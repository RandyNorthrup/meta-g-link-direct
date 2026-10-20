// SPDX-License-Identifier: AGPL-3.0-only
// Independent vectors for the portable DataX/protobuf/SDU/encoding layers.
#include "mgl/bytes.hpp"
#include "mgl/datax.hpp"
#include "mgl/encoding.hpp"
#include "mgl/playout.hpp"
#include "mgl/protobuf.hpp"
#include "mgl/sdu.hpp"
#include "mgl/stream.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
// Fails the run on the first false case and counts every case checked.
class Checks {
  public:
    void operator()(bool ok, const char* name) {
        ++count_;
        if (!ok) {
            fail(name);
        }
    }
    // A case that holds when the optional has a value; returns that value.
    template <typename T>
    [[nodiscard]] const T& present(const std::optional<T>& value, const char* name) {
        ++count_;
        if (!value) {
            fail(name);
        }
        return *value;
    }
    [[nodiscard]] int count() const noexcept { return count_; }

  private:
    int count_ = 0;

    [[noreturn]] static void fail(const char* name) {
        throw std::runtime_error(std::string(name) + ": failed");
    }
};

mgl::ByteView view(const mgl::Bytes& bytes) { return {bytes.data(), bytes.size()}; }

void testSduPhases(Checks& check) {
    using enum mgl::PrefixStatus;
    // Encrypted phase: 0x80 is a two-byte COMMAND frame, not a DataX message.
    const std::array<std::uint8_t, 2> command{0x80, 0x01};
    auto c = mgl::inspectFramePrefix(command, mgl::LinkPhase::Encrypted);
    check(c.status == Ready && c.frame_size == 2, "encrypted COMMAND is two bytes");
    // Plaintext phase: the same first byte is a DataX message.
    auto p = mgl::inspectFramePrefix(command, mgl::LinkPhase::Plaintext);
    check(p.status == Ready && p.frame_size == 5, "plaintext 0x80 is DataX length");
    // A DATA frame measures the same in both phases.
    const std::array<std::uint8_t, 10> data{0x40, 0, 0, 0, 0, 0, 0, 0, 0, 0x00};
    check(mgl::inspectFramePrefix(data, mgl::LinkPhase::Encrypted).frame_size == 26,
          "encrypted DATA one block");
    // Long-form ADVERTISEMENT command: 0x80 code | 0x80, then big-endian length.
    const std::array<std::uint8_t, 4> advert{0x80, 0x80, 0x00, 0x05};
    auto a = mgl::inspectFramePrefix(advert, mgl::LinkPhase::Encrypted);
    check(a.status == Ready && a.frame_size == 9, "encrypted long COMMAND carries its length");
    // An unframeable first byte in the encrypted phase is rejected, not skipped.
    const std::array<std::uint8_t, 2> bad{0x00, 0x00};
    check(mgl::inspectFramePrefix(bad, mgl::LinkPhase::Encrypted).status == Invalid,
          "encrypted unframeable is invalid");
}

void testProtobuf(Checks& check) {
    mgl::Bytes out;
    mgl::proto::appendVarintField(out, 1, 300);
    mgl::proto::appendStringField(out, 2, "hi");
    const std::array<std::uint8_t, 3> raw{0xDE, 0xAD, 0xBE};
    mgl::proto::appendBytesField(out, 3, {raw.data(), raw.size()});
    const auto message = mgl::proto::Message::parse(view(out));
    const auto& parsed = check.present(message, "protobuf parses what it built");
    check(parsed.varint(1) == 300U, "varint field 1 roundtrips 300");
    const auto two = parsed.bytes(2);
    check(two && two->size() == 2 && two->front() == 'h' && two->back() == 'i', "string field 2");
    const auto three = parsed.bytes(3);
    check(three && three->size() == 3 && three->front() == 0xDE, "bytes field 3");
    check(!parsed.varint(9).has_value(), "absent field is nullopt");
    // A truncated length-delimited field is rejected, not read past the end.
    const std::array<std::uint8_t, 3> truncated{0x12, 0x05, 0x01};
    check(!mgl::proto::Message::parse({truncated.data(), truncated.size()}).has_value(),
          "truncated length is rejected");
    // A truncated varint is rejected.
    const std::array<std::uint8_t, 2> badvarint{0x08, 0x80};
    check(!mgl::proto::Message::parse({badvarint.data(), badvarint.size()}).has_value(),
          "truncated varint is rejected");
}

void testDataxHeader(Checks& check) {
    mgl::datax::Tags tags;
    tags.service = static_cast<std::uint16_t>(28);
    tags.type = static_cast<std::uint16_t>(2000);
    const std::array<std::uint8_t, 4> payload{1, 2, 3, 4};
    const auto built = mgl::datax::buildMessage(0x8002, tags, {payload.data(), payload.size()});
    const auto header = mgl::datax::parseHeader(view(check.present(built, "message builds")));
    const auto& parsed = check.present(header, "header parses");
    check(parsed.channel == 0x8002, "channel roundtrips");
    check(parsed.service == 28 && parsed.has_service, "service tag roundtrips");
    check(parsed.type == 2000 && parsed.has_type, "type tag roundtrips");
    check(parsed.dataSize() == 4, "data size excludes header and tags");
    check(mgl::datax::mirror(parsed.channel) == 0x0002, "mirror clears the originator bit");
    // A service id of zero closes the channel.
    mgl::datax::Tags close;
    close.service = static_cast<std::uint16_t>(0);
    const auto closing = mgl::datax::buildMessage(0x0002, close, {});
    const auto closed = mgl::datax::parseHeader(view(*closing));
    check(closed && closed->closed, "service 0 marks the channel closed");
    // A header short of four bytes does not parse.
    const std::array<std::uint8_t, 3> too_short{0x80, 0x00, 0x00};
    check(!mgl::datax::parseHeader({too_short.data(), too_short.size()}).has_value(),
          "a three-byte head does not parse");
}

void testDecoder(Checks& check) {
    // Two whole messages in one plaintext buffer split into two.
    mgl::datax::Tags first;
    first.service = static_cast<std::uint16_t>(36);
    first.type = static_cast<std::uint16_t>(0x1000);
    const std::array<std::uint8_t, 2> p1{0xAA, 0xBB};
    auto buffer = *mgl::datax::buildMessage(0x8002, first, {p1.data(), p1.size()});
    mgl::datax::Tags second;
    second.type = static_cast<std::uint16_t>(0x1001);
    const std::array<std::uint8_t, 1> p2{0xCC};
    mgl::append(buffer, *mgl::datax::buildMessage(0x8002, second, {p2.data(), p2.size()}));
    mgl::datax::Decoder decoder;
    std::vector<mgl::datax::Message> out;
    check(decoder.decode(view(buffer), out), "decode succeeds");
    check(out.size() == 2, "two messages come out");
    check(out.at(0).header.service == 36 && out.at(0).payload.size() == 2, "first message intact");
    check(out.at(1).header.type == 0x1001 && out.at(1).payload.size() == 1,
          "second message intact");

    // A message split across two frames reassembles to the right bytes.
    // Frame 1: header declaring 20 payload bytes, but only 10 present, then modified PKCS#7 pad.
    mgl::Bytes frame1{0x00, 0x14, 0x00, 0x02}; // no extension, payload_len 20, channel 2
    for (std::uint8_t i = 0; i < 10; ++i) {
        frame1.push_back(static_cast<std::uint8_t>(0x10 + i));
    }
    frame1.push_back(0xC2); // pad 2 (0xC0 + 2)
    frame1.push_back(0xC2);
    // Frame 2: the remaining 10 payload bytes, then a full-block pad.
    mgl::Bytes frame2;
    for (std::uint8_t i = 0; i < 10; ++i) {
        frame2.push_back(static_cast<std::uint8_t>(0x20 + i));
    }
    mgl::datax::Decoder split;
    std::vector<mgl::datax::Message> parts;
    check(split.decode(view(frame1), parts), "fragment frame 1 decodes");
    check(parts.empty(), "no message until the fragment completes");
    check(split.decode(view(frame2), parts), "fragment frame 2 decodes");
    check(parts.size() == 1, "the reassembled message emerges");
    check(parts.at(0).payload.size() == 20, "reassembled payload has the declared length");
    check(parts.at(0).payload.at(0) == 0x10 && parts.at(0).payload.at(10) == 0x20,
          "reassembled bytes in order");
}

void testRegistry(Checks& check) {
    mgl::datax::ChannelRegistry registry;
    const auto channel = registry.openLocal(36);
    check(channel.has_value() && (*channel & mgl::datax::OriginatorBit) != 0,
          "local channels carry the originator bit");
    check(registry.serviceForInbound(mgl::datax::mirror(*channel)) == 36,
          "a reply on the mirror resolves to the service");
    registry.bindInbound(0x8005, 51350);
    check(registry.serviceForInbound(0x8005) == 51350, "a peer channel binds its service");
    registry.closeInbound(0x8005);
    check(registry.serviceForInbound(0x8005) == 0, "a closed peer channel forgets its service");
}

void testEncoding(Checks& check) {
    const std::array<std::uint8_t, 3> abc{'A', 'B', 'C'};
    check(mgl::encoding::base64Encode({abc.data(), 1}) == "QQ==", "base64 one byte");
    check(mgl::encoding::base64Encode({abc.data(), 2}) == "QUI=", "base64 two bytes");
    check(mgl::encoding::base64Encode({abc.data(), 3}) == "QUJD", "base64 three bytes");
    const auto decoded = mgl::encoding::base64Decode("QUJD");
    check(decoded.size() == 3 && decoded.at(0) == 'A' && decoded.at(2) == 'C',
          "base64 decode roundtrips");
    check(mgl::encoding::quotePlus("a b+c/d") == "a+b%2Bc%2Fd",
          "quote_plus encodes reserved bytes");
    using mgl::encoding::queryParameter;
    check(queryParameter("https://h/p?state=x&code=AbC%2D1#_=_\r\n", "code") == "AbC-1",
          "the code is percent-decoded after another parameter, before a fragment");
    check(queryParameter("code=plain", "code") == "plain", "a bare query works");
    check(queryParameter("https://h/p?code=a+b", "code") == "a+b", "plus stays a plus");
    check(!queryParameter("https://h/p?state=x", "code").has_value(), "an absent code is none");
    check(!queryParameter("https://h/p?code=", "code").has_value(), "an empty code is none");
    check(!queryParameter("https://h/p?code=%ZZ", "code").has_value(),
          "malformed percent-encoding is none");
    check(!queryParameter("https://h/p?xcode=1", "code").has_value(),
          "a longer parameter name does not match");
    check(!queryParameter("https://h/p#code=1", "code").has_value(),
          "a code only in the fragment does not count");
    // A valid gzip stream begins with the magic and the DEFLATE method byte.
    const std::array<std::uint8_t, 5> hello{'h', 'e', 'l', 'l', 'o'};
    const auto gz = mgl::encoding::gzip({hello.data(), hello.size()});
    check(gz.size() > 18 && gz.at(0) == 0x1F && gz.at(1) == 0x8B && gz.at(2) == 0x08,
          "gzip header is well formed");
    // The uncompressed size trailer (little-endian) records the input length.
    const auto size = static_cast<unsigned>(gz.at(gz.size() - 4)) |
                      (static_cast<unsigned>(gz.at(gz.size() - 3)) << 8U);
    check(size == 5U, "gzip trailer records the input size");
}

std::string text(mgl::ByteView bytes) { return {bytes.begin(), bytes.end()}; }

void testCreateSoftAp(Checks& check) {
    std::array<std::uint8_t, 16> uuid{};
    for (std::size_t i = 0; i < uuid.size(); ++i) {
        uuid.at(i) = static_cast<std::uint8_t>(i + 1);
    }
    const auto request = mgl::stream::buildCreateSoftApNetwork("mgl-abc234", "secretpassphrase2345",
                                                               mgl::stream::Band24GHz,
                                                               mgl::stream::Bw20MHz, 6, "GB", uuid);
    const auto parsed = check.present(mgl::proto::Message::parse(view(request)),
                                      "CreateSoftApNetwork parses as protobuf");
    check(parsed.varint(3) == mgl::stream::Band24GHz, "band is field 3");
    check(parsed.varint(4) == 6U, "channel is field 4");
    check(parsed.varint(9) == mgl::stream::Bw20MHz, "bandwidth is field 9");
    check(!parsed.varint(1).has_value() && !parsed.bytes(2).has_value(),
          "fields 1 and 2 stay unused");
    check(text(check.present(parsed.bytes(6), "country code present")) == "GB",
          "country code is field 6");
    const auto id = check.present(parsed.bytes(7), "uuid present");
    check(id.size() == 16 && id.front() == 1 && id.back() == 16, "uuid is field 7, 16 bytes");
    const auto credentials = check.present(
        mgl::proto::Message::parse(check.present(parsed.bytes(8), "credentials present")),
        "credentials parse");
    check(text(check.present(credentials.bytes(1), "ssid present")) == "mgl-abc234",
          "credentials ssid is field 1");
    check(text(check.present(credentials.bytes(2), "password present")) == "secretpassphrase2345",
          "credentials password is field 2");
}

mgl::Bytes networkAddress(const std::string& address) {
    mgl::Bytes out;
    mgl::proto::appendVarintField(out, 1, 1);
    mgl::proto::appendVarintField(out, 2, 24);
    mgl::proto::appendStringField(out, 3, address);
    return out;
}

void testSoftApInfo(Checks& check) {
    mgl::Bytes credentials;
    mgl::proto::appendStringField(credentials, 1, "mgl-abc234");
    mgl::proto::appendStringField(credentials, 2, "secretpassphrase2345");
    mgl::Bytes reply;
    mgl::proto::appendVarintField(reply, 1, 1);
    const auto empty = networkAddress("");
    const auto real = networkAddress("192.168.56.53");
    mgl::proto::appendBytesField(reply, 2, view(empty));
    mgl::proto::appendBytesField(reply, 2, view(real));
    mgl::proto::appendBytesField(reply, 3, view(credentials));
    mgl::proto::appendVarintField(reply, 4, 20203);
    mgl::proto::appendVarintField(reply, 5, 6);
    const auto info =
        check.present(mgl::stream::parseSoftApNetworkInfo(view(reply)), "SoftApNetworkInfo parses");
    check(info.ok(), "a success with address and port is usable");
    check(info.address == "192.168.56.53", "the first non-empty address is taken");
    check(info.port == 20203U, "port is field 4");
    check(info.channel == 6U, "channel is field 5");
    check(info.ssid == "mgl-abc234" && info.passphrase == "secretpassphrase2345",
          "echoed credentials are read");

    mgl::Bytes failed;
    mgl::proto::appendVarintField(failed, 1, 3); // START_AP_FAILED
    mgl::proto::appendBytesField(failed, 2, view(real));
    mgl::proto::appendVarintField(failed, 4, 20203);
    check(!check.present(mgl::stream::parseSoftApNetworkInfo(view(failed)), "failure parses").ok(),
          "a failure status is not usable");

    mgl::Bytes no_port;
    mgl::proto::appendVarintField(no_port, 1, 1);
    mgl::proto::appendBytesField(no_port, 2, view(real));
    check(!check.present(mgl::stream::parseSoftApNetworkInfo(view(no_port)), "no port parses").ok(),
          "a success without a port is not usable");

    const mgl::Bytes truncated{0x22, 0x05, 0x01};
    check(!mgl::stream::parseSoftApNetworkInfo(view(truncated)).has_value(),
          "a truncated reply is rejected");
}
void testTimedUnits(Checks& check) {
    // 8-byte little-endian presentation time, then the payload.
    const mgl::Bytes unit{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xAA, 0xBB};
    const auto timed = check.present(mgl::stream::splitTimedUnit(view(unit)), "unit splits");
    check(timed.pts_us == 0x0807060504030201ULL, "presentation time is little-endian");
    check(timed.payload.size() == 2 && timed.payload.front() == 0xAA, "payload follows prefix");
    const mgl::Bytes prefix_only{1, 2, 3, 4, 5, 6, 7, 8};
    check(!mgl::stream::splitTimedUnit(view(prefix_only)).has_value(),
          "a unit with no payload is rejected");
    check(!mgl::stream::splitTimedUnit({}).has_value(), "an empty unit is rejected");
}

void testConfigureAudio(Checks& check) {
    for (const bool enabled : {false, true}) {
        const auto request = mgl::stream::buildConfigureAudio(7, enabled);
        const auto parsed =
            check.present(mgl::proto::Message::parse(view(request)), "ConfigureAudio parses");
        check(parsed.varint(1) == 7U, "nonce is field 1");
        check(parsed.varint(3) == (enabled ? 1U : 0U), "enabled flag is field 3");
        const auto config = check.present(
            mgl::proto::Message::parse(check.present(parsed.bytes(2), "AudioConfig present")),
            "AudioConfig parses");
        check(text(check.present(config.bytes(1), "mime present")) == "audio/mp4a-latm",
              "AAC mime");
        check(config.varint(2) == 48'000U && config.varint(4) == 2U, "48 kHz stereo");
    }
    // AAC-LC (2), 48 kHz (index 3), 2 channels: 00010 0011 0010 000.
    const auto asc = mgl::stream::requestedAudioSpecificConfig();
    check(asc.size() == 2 && asc.at(0) == 0x11 && asc.at(1) == 0x90,
          "AudioSpecificConfig describes the requested AAC-LC 48 kHz stereo");
}

void testConfigureVideo(Checks& check) {
    const mgl::stream::VideoConfig low{
        .width = 504, .height = 896, .bitrate = 1'500'000, .frame_rate = 30, .iframe_interval = 1};
    for (const bool enabled : {false, true}) {
        const auto request = mgl::stream::buildConfigureVideo(9, low, enabled);
        const auto parsed =
            check.present(mgl::proto::Message::parse(view(request)), "ConfigureVideo parses");
        check(parsed.varint(1) == 9U, "video nonce is field 1");
        check(parsed.varint(3) == (enabled ? 1U : 0U), "video enabled flag is field 3");
        const auto config = check.present(
            mgl::proto::Message::parse(check.present(parsed.bytes(2), "VideoConfig present")),
            "VideoConfig parses");
        check(text(check.present(config.bytes(1), "video mime present")) == "video/hevc",
              "HEVC mime");
        check(config.varint(2) == 504U && config.varint(3) == 896U,
              "profile size is fields 2 and 3");
        check(config.varint(4) == 1'500'000U && config.varint(5) == 30U,
              "bitrate and frame rate are fields 4 and 5");
    }
    check(low == mgl::stream::VideoConfig{.width = 504,
                                          .height = 896,
                                          .bitrate = 1'500'000,
                                          .frame_rate = 30,
                                          .iframe_interval = 1},
          "equal profiles compare equal");
    check(!(low == mgl::stream::VideoConfig{}), "a different profile compares unequal");
}

mgl::Bytes deviceStateWithBattery(std::uint64_t charge_state, std::uint64_t percent) {
    mgl::Bytes battery;
    mgl::proto::appendVarintField(battery, 2, charge_state);
    mgl::proto::appendVarintField(battery, 3, percent);
    mgl::proto::appendVarintField(battery, 4, 31'200);
    mgl::proto::appendVarintField(battery, 5, 3'850);
    mgl::Bytes state;
    mgl::proto::appendBytesField(state, 1, battery);
    mgl::proto::appendVarintField(state, 9, 1); // an unrelated DeviceState field
    return state;
}

void testBatteryState(Checks& check) {
    const auto state = deviceStateWithBattery(4, 64);
    const auto battery = check.present(mgl::stream::parseBatteryState(view(state)),
                                       "a DeviceState with a battery parses");
    check(battery.percent == 64U, "battery percent is field 1.3");
    check(!battery.charging(), "charge state 4 is not charging");
    check(battery.millivolts == 3'850U && battery.millicelsius == 31'200,
          "voltage and temperature are fields 1.5 and 1.4");
    const auto charging = deviceStateWithBattery(1, 80);
    check(
        check.present(mgl::stream::parseBatteryState(view(charging)), "charging parses").charging(),
        "charge state 1 is charging");
    const auto impossible = deviceStateWithBattery(4, 101);
    check(!mgl::stream::parseBatteryState(view(impossible)).has_value(),
          "a percentage above 100 is rejected");
    mgl::Bytes without;
    mgl::proto::appendVarintField(without, 9, 1);
    check(!mgl::stream::parseBatteryState(view(without)).has_value(),
          "a DeviceState without a battery gives none");
}

// 30 fps capture times, as the glasses stamp them.
constexpr std::int64_t FramesPerSecond = 30;
constexpr std::int64_t FrameUs = 33'333;
constexpr std::int64_t CaptureStartUs = 916'000'000;

std::uint64_t captureOf(std::int64_t index) {
    return static_cast<std::uint64_t>(CaptureStartUs + (index * FrameUs));
}

// The smallest and largest gap between consecutive presentations from index first on.
struct Spacing {
    std::int64_t smallest = INT64_MAX;
    std::int64_t largest = 0;
};

Spacing spacingFrom(const std::vector<std::int64_t>& presented, std::size_t first) {
    Spacing spacing;
    for (std::size_t i = first + 1; i < presented.size(); ++i) {
        const std::int64_t gap = presented.at(i) - presented.at(i - 1);
        spacing.smallest = std::min(spacing.smallest, gap);
        spacing.largest = std::max(spacing.largest, gap);
    }
    return spacing;
}

void testPlayoutSteady(Checks& check) {
    // Every frame arrives 5 ms after capture: the delay falls from its start to the minimum,
    // slowly enough that the cadence barely changes.
    constexpr std::int64_t TransitUs = 5'000;
    constexpr std::int64_t Frames = FramesPerSecond * 40;
    mgl::playout::PlayoutClock clock;
    std::vector<std::int64_t> presented;
    bool never_early = true;
    for (std::int64_t i = 0; i < Frames; ++i) {
        const std::int64_t arrival = CaptureStartUs + (i * FrameUs) + TransitUs;
        const std::int64_t at = clock.presentAt(captureOf(i), arrival);
        never_early = never_early && at >= arrival;
        presented.push_back(at);
    }
    const auto spacing = spacingFrom(presented, 0);
    check(never_early, "a unit is never presented before it arrives");
    check(spacing.smallest >= FrameUs - (FrameUs / 100) - 1 && spacing.largest <= FrameUs + 1,
          "a falling delay plays at most 1% fast");
    check(clock.delayUs() <= 25'000, "a steady link settles at the minimum delay");
    check(clock.lateUnits() == 0, "a steady link has no late units");
}

void testPlayoutBursts(Checks& check) {
    // The Gen 2 pattern: frames leave in bursts of five, all arriving within about a millisecond
    // just after the last of them was captured.
    constexpr std::int64_t Burst = 5;
    constexpr std::int64_t Frames = FramesPerSecond * 60;
    constexpr std::int64_t WarmUpFrames = FramesPerSecond * 20;
    mgl::playout::PlayoutClock clock;
    std::vector<std::int64_t> presented;
    std::vector<std::int64_t> arrived;
    for (std::int64_t i = 0; i < Frames; ++i) {
        const std::int64_t last_in_burst = ((i / Burst) * Burst) + (Burst - 1);
        const std::int64_t arrival =
            CaptureStartUs + (last_in_burst * FrameUs) + 2'000 + ((i % Burst) * 200);
        arrived.push_back(arrival);
        presented.push_back(clock.presentAt(captureOf(i), arrival));
    }
    const auto arrivals = spacingFrom(arrived, WarmUpFrames);
    check(arrivals.smallest < 1'000 && arrivals.largest > 4 * FrameUs,
          "the test feeds bursts: sub-millisecond gaps and gaps of four frames");
    const auto spacing = spacingFrom(presented, WarmUpFrames);
    check(spacing.smallest >= FrameUs - (FrameUs / 100) - 1 && spacing.largest <= FrameUs + 1,
          "burst presentations follow the capture cadence");
    check(clock.lateUnits() == 0, "the starting delay already covers a burst gap");
    const std::int64_t delay = clock.delayUs();
    check(delay >= (Burst - 1) * FrameUs && delay <= ((Burst - 1) * FrameUs) + 30'000,
          "the delay settles just above the burst gap");
}

void testPlayoutLateUnit(Checks& check) {
    // One frame 300 ms late raises the delay at once; the frames after it are on time again.
    constexpr std::int64_t Frames = FramesPerSecond * 20;
    constexpr std::int64_t LateIndex = FramesPerSecond * 10;
    constexpr std::int64_t LateUs = 300'000;
    mgl::playout::PlayoutClock clock;
    std::uint64_t late_after_spike = 0;
    for (std::int64_t i = 0; i < Frames; ++i) {
        const std::int64_t arrival =
            CaptureStartUs + (i * FrameUs) + 5'000 + (i == LateIndex ? LateUs : 0);
        (void)clock.presentAt(captureOf(i), arrival);
        if (i == LateIndex) {
            late_after_spike = clock.lateUnits();
            check(clock.delayUs() >= LateUs, "a late unit raises the delay past its lateness");
        }
    }
    check(late_after_spike == 1 && clock.lateUnits() == 1,
          "only the late unit counts as late; the raised delay covers the rest");
}

void testPlayoutStall(Checks& check) {
    // A two-second stall is longer than the largest delay: those frames are presented on arrival
    // and the delay stops at its maximum.
    const mgl::playout::Settings settings;
    mgl::playout::PlayoutClock clock(settings);
    std::int64_t at_stall = 0;
    std::int64_t arrival_at_stall = 0;
    for (std::int64_t i = 0; i < FramesPerSecond * 5; ++i) {
        const std::int64_t stall = i >= 60 && i < 120 ? 2'000'000 - ((i - 60) * FrameUs) : 0;
        const std::int64_t arrival = CaptureStartUs + (i * FrameUs) + 5'000 + stall;
        const std::int64_t at = clock.presentAt(captureOf(i), arrival);
        if (i == 60) {
            at_stall = at;
            arrival_at_stall = arrival;
        }
    }
    check(at_stall == arrival_at_stall, "a unit later than the largest delay plays on arrival");
    check(clock.delayUs() <= settings.max_delay_us, "the delay never passes its maximum");
    check(clock.lateUnits() >= 1, "units past the largest delay count as late");
}

void testPlayoutRestart(Checks& check) {
    // A new stream with a new time base: the clock restarts instead of presenting 100 s late.
    mgl::playout::PlayoutClock clock;
    std::int64_t arrival = CaptureStartUs + 5'000;
    for (std::int64_t i = 0; i < 90; ++i) {
        arrival = CaptureStartUs + (i * FrameUs) + 5'000;
        (void)clock.presentAt(captureOf(i), arrival);
    }
    const std::int64_t delay_before = clock.delayUs();
    const std::int64_t next_arrival = arrival + FrameUs;
    const std::uint64_t jumped = captureOf(90) + 100'000'000ULL;
    const std::int64_t at = clock.presentAt(jumped, next_arrival);
    check(clock.restarts() == 1, "a capture time 100 s off restarts the clock");
    check(at - next_arrival == delay_before && clock.lateUnits() == 0,
          "a restart keeps the delay it had");
}

void testPlayoutSharedClock(Checks& check) {
    // Video and audio share the glasses' clock, so units captured together are presented
    // together.
    constexpr std::int64_t AudioUs = 21'333;
    mgl::playout::PlayoutClock clock;
    std::int64_t video_at = 0;
    std::int64_t audio_at = 0;
    for (std::int64_t i = 0; i < 300; ++i) {
        const std::int64_t capture = CaptureStartUs + (i * AudioUs);
        audio_at = clock.presentAt(static_cast<std::uint64_t>(capture), capture + 3'000);
        video_at = clock.presentAt(static_cast<std::uint64_t>(capture), capture + 9'000);
    }
    // Within a millisecond: between their arrivals the delay may shrink by 1% of the gap.
    check(std::abs(video_at - audio_at) <= 1'000,
          "audio and video captured together are presented together");
}
} // namespace

int main() {
    try {
        Checks check;
        testSduPhases(check);
        testProtobuf(check);
        testDataxHeader(check);
        testDecoder(check);
        testRegistry(check);
        testEncoding(check);
        testCreateSoftAp(check);
        testSoftApInfo(check);
        testTimedUnits(check);
        testConfigureAudio(check);
        testConfigureVideo(check);
        testBatteryState(check);
        testPlayoutSteady(check);
        testPlayoutBursts(check);
        testPlayoutLateUnit(check);
        testPlayoutStall(check);
        testPlayoutRestart(check);
        testPlayoutSharedClock(check);
        std::cout << check.count() << " protocol/datax/protobuf/encoding/stream cases passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        constexpr int DiagnosticWriteFailed = 4;
        if (std::fputs(error.what(), stderr) == EOF || std::fputc('\n', stderr) == EOF) {
            return DiagnosticWriteFailed;
        }
        return EXIT_FAILURE;
    }
}
