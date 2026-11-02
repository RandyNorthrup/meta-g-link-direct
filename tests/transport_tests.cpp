// SPDX-License-Identifier: AGPL-3.0-only
// BufferedTransport cuts whatever a socket delivers into whole frames, on every platform's byte
// stream. A scripted fill stands in for the socket.
#include "mgl/buffered_transport.hpp"
#include "mgl/bytes.hpp"
#include "mgl/sdu.hpp"
#include "mgl/transport.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace std::chrono_literals;

// One scripted result of fill: bytes to deliver, or the status to report when there are none.
struct Step {
    std::vector<std::uint8_t> bytes;
    mgl::TransportStatus status = mgl::TransportStatus::Ok;
};

class ScriptedTransport final : public mgl::BufferedTransport {
  public:
    explicit ScriptedTransport(std::deque<Step> script) : script_(std::move(script)) {}
    [[nodiscard]] bool send(mgl::ByteView /*frame*/) override { return true; }
    void close() override {}
    [[nodiscard]] int fills() const noexcept { return fills_; }

  protected:
    [[nodiscard]] mgl::TransportStatus fill(std::chrono::milliseconds /*timeout*/) override {
        ++fills_;
        if (script_.empty()) {
            return mgl::TransportStatus::TimedOut;
        }
        const Step step = std::move(script_.front());
        script_.pop_front();
        keep(mgl::ByteView(step.bytes.data(), step.bytes.size()));
        return step.status;
    }

  private:
    std::deque<Step> script_;
    int fills_ = 0;
};

void check(bool ok, const char* name) {
    if (!ok) {
        throw std::runtime_error(std::string(name) + ": failed");
    }
}

// A plaintext DataX frame: two header bytes holding the payload length, two more fixed bytes, then
// the payload.
std::vector<std::uint8_t> dataxFrame(std::uint8_t payload_size, std::uint8_t fill_byte) {
    std::vector<std::uint8_t> frame{0x80, payload_size, 0x01, 0x02};
    frame.insert(frame.end(), payload_size, fill_byte);
    return frame;
}

constexpr auto Phase = mgl::LinkPhase::Plaintext;

void testSplitFrame() {
    const auto frame = dataxFrame(6, 0xAB);
    constexpr std::ptrdiff_t Split = 1; // inside the prefix, the hardest place to cut
    ScriptedTransport transport({
        {.bytes = {frame.begin(), frame.begin() + Split}},
        {.bytes = {frame.begin() + Split, frame.end() - 1}},
        {.bytes = {frame.end() - 1, frame.end()}},
    });
    const auto received = transport.receive(Phase, 1s);
    check(received.status == mgl::TransportStatus::Ok, "a split frame arrives");
    check(received.frame == frame, "a split frame arrives whole and unchanged");
    check(transport.fills() == 3, "a split frame takes as many fills as its pieces");
}

void testFramesInOneRead() {
    const auto first = dataxFrame(1, 0x11);
    const auto second = dataxFrame(3, 0x22);
    std::vector<std::uint8_t> both = first;
    both.insert(both.end(), second.begin(), second.end());
    ScriptedTransport transport({{.bytes = both}});
    check(transport.receive(Phase, 1s).frame == first, "the first of two frames comes first");
    const auto next = transport.receive(Phase, 1s);
    check(next.frame == second, "the second of two frames follows");
    check(transport.fills() == 1, "a buffered frame needs no further read");
}

void testInvalidAndOversize() {
    ScriptedTransport invalid({{.bytes = {0x3F, 0xFF}}});
    check(invalid.receive(Phase, 1s).status == mgl::TransportStatus::Unframeable,
          "an unknown frame type is refused");
    // Encrypted AirShield frames are bounded by their block count, DataX by its length field, so
    // the largest frame the prefix can describe still fits the limit.
    ScriptedTransport largest({{.bytes = {0xFF, 0xFF}}});
    check(largest.receive(Phase, 1ms).status == mgl::TransportStatus::TimedOut,
          "the largest legal frame waits for its bytes");
    check(mgl::MaximumSduSize == 16387, "the limit equals the largest DataX frame");
}

void testEnds() {
    ScriptedTransport closed({{.bytes = {}, .status = mgl::TransportStatus::Closed}});
    check(closed.receive(Phase, 1s).status == mgl::TransportStatus::Closed,
          "a closed link is reported");
    ScriptedTransport idle(std::deque<Step>{});
    check(idle.receive(Phase, 1s).status == mgl::TransportStatus::TimedOut,
          "no bytes in time is a timeout");
    const auto frame = dataxFrame(2, 0x33);
    ScriptedTransport partial({{.bytes = {frame.begin(), frame.begin() + 3}}});
    const auto cut = partial.receive(Phase, 1ms);
    check(cut.status == mgl::TransportStatus::TimedOut && cut.frame.empty(),
          "half a frame is never handed over");
}
} // namespace

int main() {
    try {
        testSplitFrame();
        testFramesInOneRead();
        testInvalidAndOversize();
        testEnds();
        std::cout << "buffered transport cases passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        constexpr int DiagnosticWriteFailed = 4;
        if (std::fputs(error.what(), stderr) == EOF || std::fputc('\n', stderr) == EOF) {
            return DiagnosticWriteFailed;
        }
        return EXIT_FAILURE;
    }
}
