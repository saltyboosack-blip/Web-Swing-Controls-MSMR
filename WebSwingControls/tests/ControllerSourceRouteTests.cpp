#include "ControllerSourceRoute.h"
#include "ControllerWebInputPolicy.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

namespace {

using trueswing::rebuild::runtime::ControllerSourceRoute;
using trueswing::rebuild::runtime::ControllerSourceRouteState;
using trueswing::rebuild::runtime::ControllerWebInputDecision;
using trueswing::rebuild::runtime::ControllerWebInputPolicy;
using trueswing::rebuild::runtime::ControllerWebInputSample;
using trueswing::rebuild::runtime::ControllerWebOwner;

template <typename Candidate>
concept AcceptedRouteInput =
    requires(ControllerSourceRoute& route, Candidate&& candidate) {
        route.Update(std::forward<Candidate>(candidate));
    };

static_assert(AcceptedRouteInput<
              std::optional<ControllerSourceRoute::Device>>);
static_assert(!AcceptedRouteInput<ControllerSourceRoute::Device>);
static_assert(!AcceptedRouteInput<
              std::array<ControllerSourceRoute::Device, 2>>);

class TestFailure final : public std::exception {
public:
    explicit TestFailure(std::string message) : message_(std::move(message)) {}

    [[nodiscard]] const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};

void Require(bool condition, const char* message) {
    if (!condition) {
        throw TestFailure(message);
    }
}

[[nodiscard]] ControllerWebInputSample Eligible(
    const ControllerSourceRouteState& route, double leftTrigger = 0.0,
    double rightTrigger = 0.0) noexcept {
    return {.sourceEpoch = route.sourceEpoch,
            .connected = route.device.has_value(),
            .focused = true,
            .runtimeReady = true,
            .airborneProven = true,
            .leftShoulderHeld = true,
            .nativeSwingAvailable = true,
            .leftTrigger = leftTrigger,
            .rightTrigger = rightTrigger};
}

void TestStableDeviceDoesNotAdvanceEpoch() {
    ControllerSourceRoute route;
    constexpr ControllerSourceRoute::Device deviceA = 0x1000U;

    const ControllerSourceRouteState first =
        route.Update(std::optional{deviceA});
    Require(first.changed, "first device was not reported as a change");
    Require(first.device == deviceA, "first device was not retained");
    Require(first.sourceEpoch != 0U, "first source epoch was zero");

    const ControllerSourceRouteState stable =
        route.Update(std::optional{deviceA});
    Require(!stable.changed, "stable device reported a route change");
    Require(stable.device == deviceA, "stable device changed identity");
    Require(stable.sourceEpoch == first.sourceEpoch,
            "stable device advanced source epoch");
    Require(route.CurrentDevice() == stable.device &&
                route.SourceEpoch() == stable.sourceEpoch &&
                route.Changed() == stable.changed,
            "route accessors disagreed with returned state");
}

void TestReplacementReleasesOldOwnerWithoutHeldHighAttach() {
    ControllerSourceRoute route;
    ControllerWebInputPolicy policy;
    constexpr ControllerSourceRoute::Device deviceA = 0x1000U;
    constexpr ControllerSourceRoute::Device deviceB = 0x2000U;

    const ControllerSourceRouteState sourceA =
        route.Update(std::optional{deviceA});
    (void)policy.Update(Eligible(sourceA));
    const ControllerWebInputDecision attach =
        policy.Update(Eligible(sourceA, 0.75));
    Require(attach.leftAttach && attach.nativeSwingPress &&
                policy.CurrentOwner() == ControllerWebOwner::Left,
            "device A did not acquire the left web");

    const ControllerSourceRouteState sourceB =
        route.Update(std::optional{deviceB});
    Require(sourceB.changed && sourceB.device == deviceB &&
                sourceB.sourceEpoch > sourceA.sourceEpoch,
            "A-to-B route did not advance to a fresh epoch");

    const ControllerWebInputDecision replacement =
        policy.Update(Eligible(sourceB, 0.75));
    Require(replacement.leftRelease && replacement.nativeSwingRelease,
            "A-to-B route did not release device A ownership");
    Require(!replacement.leftAttach && !replacement.rightAttach &&
                !replacement.nativeSwingPress &&
                policy.CurrentOwner() == ControllerWebOwner::None,
            "held-high device B inherited device A ownership");
}

void TestDisconnectCancelsOwnership() {
    ControllerSourceRoute route;
    ControllerWebInputPolicy policy;
    constexpr ControllerSourceRoute::Device deviceA = 0x1000U;

    const ControllerSourceRouteState sourceA =
        route.Update(std::optional{deviceA});
    (void)policy.Update(Eligible(sourceA));
    const ControllerWebInputDecision attach =
        policy.Update(Eligible(sourceA, 0.0, 0.75));
    Require(attach.rightAttach && attach.nativeSwingPress,
            "device A did not acquire the right web");

    const ControllerSourceRouteState disconnected = route.Update(std::nullopt);
    Require(disconnected.changed && !disconnected.device.has_value() &&
                disconnected.sourceEpoch > sourceA.sourceEpoch,
            "A-to-none route did not publish a disconnect epoch");

    const ControllerWebInputDecision cancel =
        policy.Update(Eligible(disconnected));
    Require(cancel.rightRelease && cancel.nativeSwingRelease &&
                !cancel.leftAttach && !cancel.rightAttach &&
                policy.CurrentOwner() == ControllerWebOwner::None,
            "disconnect did not cancel the old controller owner");
}

void TestReconnectOfSameAddressGetsFreshEpoch() {
    ControllerSourceRoute route;
    constexpr ControllerSourceRoute::Device deviceA = 0x1000U;

    const ControllerSourceRouteState first =
        route.Update(std::optional{deviceA});
    const ControllerSourceRouteState disconnected = route.Update(std::nullopt);
    const ControllerSourceRouteState reconnected =
        route.Update(std::optional{deviceA});

    Require(disconnected.changed && !disconnected.device.has_value(),
            "disconnect was not represented as no main device");
    Require(reconnected.changed && reconnected.device == deviceA,
            "same-address reconnect was not reported as a change");
    Require(reconnected.sourceEpoch > disconnected.sourceEpoch &&
                disconnected.sourceEpoch > first.sourceEpoch,
            "same-address reconnect reused an old source epoch");

    const std::uint64_t beforeReset = reconnected.sourceEpoch;
    route.Reset();
    const ControllerSourceRouteState reset = route.State();
    Require(!reset.device.has_value() && !reset.changed &&
                reset.sourceEpoch == beforeReset,
            "reset did not clear only the current route state");
    const ControllerSourceRouteState afterReset =
        route.Update(std::optional{deviceA});
    Require(afterReset.sourceEpoch > beforeReset,
            "post-reset reconnect reused a prior epoch");
}

void TestNoMainCannotSelectUnrelatedPads() {
    ControllerSourceRoute route;

    const ControllerSourceRouteState noMain = route.Update(std::nullopt);
    Require(!noMain.device.has_value() && !noMain.changed &&
                noMain.sourceEpoch == 0U,
            "no-main input invented an unrelated controller route");

    const ControllerSourceRouteState invalidMain =
        route.Update(std::optional<ControllerSourceRoute::Device>{0U});
    Require(!invalidMain.device.has_value() && !invalidMain.changed &&
                invalidMain.sourceEpoch == 0U,
            "invalid logical-player-0 pointer became a controller route");
}

} // namespace

int main() {
    const std::array tests{
        std::pair{"stable device epoch", TestStableDeviceDoesNotAdvanceEpoch},
        std::pair{"replacement cancels owner",
                  TestReplacementReleasesOldOwnerWithoutHeldHighAttach},
        std::pair{"disconnect cancels owner", TestDisconnectCancelsOwnership},
        std::pair{"same-address reconnect epoch",
                  TestReconnectOfSameAddressGetsFreshEpoch},
        std::pair{"no unrelated fallback", TestNoMainCannotSelectUnrelatedPads},
    };

    std::size_t passed = 0U;
    for (const auto& [name, test] : tests) {
        try {
            test();
            ++passed;
        } catch (const std::exception& error) {
            std::cerr << "FAILED: " << name << ": " << error.what() << '\n';
            return 1;
        }
    }

    std::cout << "Controller source route tests passed: " << passed << "/"
              << tests.size() << '\n';
    return 0;
}
