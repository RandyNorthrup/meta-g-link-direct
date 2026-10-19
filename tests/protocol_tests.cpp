// SPDX-License-Identifier: AGPL-3.0-only
// Independent vectors for the portable DataX/protobuf/SDU/encoding layers.
#include "mgl/datax.hpp"
#include "mgl/encoding.hpp"
#include "mgl/protobuf.hpp"
#include "mgl/sdu.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
int g_cases = 0;

void check(bool ok, const char* name) {
    ++g_cases;
    if (!ok) {
        throw std::runtime_error(std::string(name) + ": failed");
    }
}

mgl::ByteView view(const mgl::Bytes& bytes) { return {bytes.data(), bytes.size()}; }

void testSduPhases() {
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

void testProtobuf() {
    mgl::Bytes out;
    mgl::proto::appendVarintField(out, 1, 300);
    mgl::proto::appendStringField(out, 2, "hi");
    const std::array<std::uint8_t, 3> raw{0xDE, 0xAD, 0xBE};
    mgl::proto::appendBytesField(out, 3, {raw.data(), raw.size()});
    const auto message = mgl::proto::Message::parse(view(out));
    check(message.has_value(), "protobuf parses what it built");
    check(message->varint(1) == 300U, "varint field 1 roundtrips 300");
    const auto two = message->bytes(2);
    check(two && two->size() == 2 && (*two)[0] == 'h' && (*two)[1] == 'i', "string field 2");
    const auto three = message->bytes(3);
    check(three && three->size() == 3 && (*three)[0] == 0xDE, "bytes field 3");
    check(!message->varint(9).has_value(), "absent field is nullopt");
    // A truncated length-delimited field is rejected, not read past the end.
    const std::array<std::uint8_t, 3> truncated{0x12, 0x05, 0x01};
    check(!mgl::proto::Message::parse({truncated.data(), truncated.size()}).has_value(),
          "truncated length is rejected");
    // A truncated varint is rejected.
    const std::array<std::uint8_t, 2> badvarint{0x08, 0x80};
    check(!mgl::proto::Message::parse({badvarint.data(), badvarint.size()}).has_value(),
          "truncated varint is rejected");
}

void testDataxHeader() {
    mgl::datax::Tags tags;
    tags.service = static_cast<std::uint16_t>(28);
    tags.type = static_cast<std::uint16_t>(2000);
    const std::array<std::uint8_t, 4> payload{1, 2, 3, 4};
    const auto built = mgl::datax::buildMessage(0x8002, tags, {payload.data(), payload.size()});
    check(built.has_value(), "message builds");
    const auto header = mgl::datax::parseHeader(view(*built));
    check(header.has_value(), "header parses");
    check(header->channel == 0x8002, "channel roundtrips");
    check(header->service == 28 && header->has_service, "service tag roundtrips");
    check(header->type == 2000 && header->has_type, "type tag roundtrips");
    check(header->dataSize() == 4, "data size excludes header and tags");
    check(mgl::datax::mirror(0x8002) == 0x0002, "mirror clears the originator bit");
    // A service id of zero closes the channel.
    mgl::datax::Tags close;
    close.service = static_cast<std::uint16_t>(0);
    const auto closing = mgl::datax::buildMessage(0x0002, close, {});
    const auto closed = mgl::datax::parseHeader(view(*closing));
    check(closed && closed->closed, "service 0 marks the channel closed");
    // A header short of four bytes does not parse.
    const std::array<std::uint8_t, 3> tooShort{0x80, 0x00, 0x00};
    check(!mgl::datax::parseHeader({tooShort.data(), tooShort.size()}).has_value(),
          "a three-byte head does not parse");
}

void testDecoder() {
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
    check(out[0].header.service == 36 && out[0].payload.size() == 2, "first message intact");
    check(out[1].header.type == 0x1001 && out[1].payload.size() == 1, "second message intact");

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
    check(parts[0].payload.size() == 20, "reassembled payload has the declared length");
    check(parts[0].payload[0] == 0x10 && parts[0].payload[10] == 0x20,
          "reassembled bytes in order");
}

void testRegistry() {
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

void testEncoding() {
    const std::array<std::uint8_t, 3> abc{'A', 'B', 'C'};
    check(mgl::encoding::base64Encode({abc.data(), 1}) == "QQ==", "base64 one byte");
    check(mgl::encoding::base64Encode({abc.data(), 2}) == "QUI=", "base64 two bytes");
    check(mgl::encoding::base64Encode({abc.data(), 3}) == "QUJD", "base64 three bytes");
    const auto decoded = mgl::encoding::base64Decode("QUJD");
    check(decoded.size() == 3 && decoded[0] == 'A' && decoded[2] == 'C',
          "base64 decode roundtrips");
    check(mgl::encoding::quotePlus("a b+c/d") == "a+b%2Bc%2Fd",
          "quote_plus encodes reserved bytes");
    // A valid gzip stream begins with the magic and the DEFLATE method byte.
    const std::array<std::uint8_t, 5> hello{'h', 'e', 'l', 'l', 'o'};
    const auto gz = mgl::encoding::gzip({hello.data(), hello.size()});
    check(gz.size() > 18 && gz[0] == 0x1F && gz[1] == 0x8B && gz[2] == 0x08,
          "gzip header is well formed");
    // The uncompressed size trailer (little-endian) records the input length.
    const auto size = gz[gz.size() - 4] | (gz[gz.size() - 3] << 8);
    check(size == 5, "gzip trailer records the input size");
}
} // namespace

int main() {
    try {
        testSduPhases();
        testProtobuf();
        testDataxHeader();
        testDecoder();
        testRegistry();
        testEncoding();
        std::cout << g_cases << " protocol/datax/protobuf/encoding cases passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        constexpr int DiagnosticWriteFailed = 4;
        if (std::fputs(error.what(), stderr) == EOF || std::fputc('\n', stderr) == EOF) {
            return DiagnosticWriteFailed;
        }
        return EXIT_FAILURE;
    }
}
