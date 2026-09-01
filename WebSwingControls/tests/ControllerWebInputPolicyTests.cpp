#include "ControllerWebInputPolicy.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

using trueswing::rebuild::runtime::ControllerWebInputDecision;
using trueswing::rebuild::runtime::ControllerWebInputPolicy;
using trueswing::rebuild::runtime::ControllerWebInputSample;
using trueswing::rebuild::runtime::ControllerWebOwner;

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
    std::uint64_t sourceEpoch = 1U) noexcept {
    return {.sourceEpoch = sourceEpoch,
            .connected = true,
            .focused = true,
            .runtimeReady = true,
            .airborneProven = true,
            .leftShoulderHeld = true,
            .nativeSwingAvailable = true,
            .leftTrigger = 0.0,
            .rightTrigger = 0.0};
}

[[nodiscard]] bool HasOwnershipEffect(
    const ControllerWebInputDecision& decision) noexcept {
    return decision.leftAttach || decision.leftRelease ||
           decision.rightAttach || decision.rightRelease ||
           decision.nativeSwingPress || decision.nativeSwingRelease;
}

void ArmNeutral(ControllerWebInputPolicy& policy,
                std::uint64_t sourceEpoch = 1U) {
    const ControllerWebInputDecision decision =
        policy.Update(Eligible(sourceEpoch));
    Require(!HasOwnershipEffect(decision),
            "neutral arming sample changed ownership");
}

void TestSchmittThresholdsAndJitter() {
    ControllerWebInputPolicy policy;
    ArmNeutral(policy);

    ControllerWebInputSample sample = Eligible();
    sample.leftTrigger = 0.499;
    auto decision = policy.Update(sample);
    Require(!decision.consumeLeftTrigger && !decision.leftAttach,
            "value below press threshold attached");

    sample.leftTrigger = 0.500;
    decision = policy.Update(sample);
    Require(decision.consumeLeftTrigger && decision.leftAttach &&
                decision.nativeSwingPress,
            "inclusive press threshold did not attach");

    for (const double value : {0.49, 0.30, 0.251, 0.40}) {
        sample.leftTrigger = value;
        decision = policy.Update(sample);
        Require(decision.consumeLeftTrigger &&
                    !HasOwnershipEffect(decision),
                "hysteresis-band jitter changed held ownership");
    }

    sample.leftTrigger = 0.250;
    decision = policy.Update(sample);
    Require(decision.consumeLeftTrigger && decision.leftRelease &&
                decision.nativeSwingRelease,
            "inclusive release threshold did not release");

    sample.leftTrigger = 0.499;
    decision = policy.Update(sample);
    Require(!decision.consumeLeftTrigger && !decision.leftAttach,
            "released trigger reattached inside hysteresis band");

    sample.leftTrigger = 2.0;
    decision = policy.Update(sample);
    Require(decision.leftAttach && decision.nativeSwingPress,
            "finite high value was not clamped to pressed");
}

void TestFreshEdgeRequiredAfterStartupAndGateEntry() {
    ControllerWebInputPolicy startupPolicy;
    ControllerWebInputSample sample = Eligible();
    sample.leftTrigger = 1.0;
    auto decision = startupPolicy.Update(sample);
    Require(!decision.consumeLeftTrigger && !decision.leftAttach,
            "startup-held trigger was captured");
    sample.leftTrigger = 0.0;
    (void)startupPolicy.Update(sample);
    sample.leftTrigger = 0.5;
    decision = startupPolicy.Update(sample);
    Require(decision.leftAttach,
            "neutral then fresh startup edge did not attach");

    ControllerWebInputPolicy gatePolicy;
    sample = Eligible();
    sample.leftShoulderHeld = false;
    (void)gatePolicy.Update(sample);
    sample.leftTrigger = 0.8;
    decision = gatePolicy.Update(sample);
    Require(!decision.consumeLeftTrigger && !decision.leftAttach,
            "non-gated trigger press was captured");
    sample.leftShoulderHeld = true;
    decision = gatePolicy.Update(sample);
    Require(!decision.consumeLeftTrigger && !decision.leftAttach,
            "L1 gate stole a trigger already held");
    sample.leftTrigger = 0.0;
    (void)gatePolicy.Update(sample);
    sample.leftTrigger = 0.5;
    decision = gatePolicy.Update(sample);
    Require(decision.leftAttach,
            "fresh edge after L1 gate entry did not attach");
}

void TestExactLeftAndRightMapping() {
    ControllerWebInputPolicy leftPolicy;
    ArmNeutral(leftPolicy);
    ControllerWebInputSample sample = Eligible();
    sample.leftTrigger = 0.5;
    auto decision = leftPolicy.Update(sample);
    Require(decision.consumeLeftTrigger && !decision.consumeRightTrigger &&
                decision.leftAttach && !decision.rightAttach &&
                decision.nativeSwingPress &&
                leftPolicy.CurrentOwner() == ControllerWebOwner::Left,
            "L2 did not own exact left web");
    sample.leftTrigger = 0.0;
    decision = leftPolicy.Update(sample);
    Require(decision.leftRelease && !decision.rightRelease &&
                decision.nativeSwingRelease,
            "L2 did not release exact left web");

    ControllerWebInputPolicy rightPolicy;
    ArmNeutral(rightPolicy);
    sample = Eligible();
    sample.rightTrigger = 0.5;
    decision = rightPolicy.Update(sample);
    Require(!decision.consumeLeftTrigger && decision.consumeRightTrigger &&
                !decision.leftAttach && decision.rightAttach &&
                decision.nativeSwingPress &&
                rightPolicy.CurrentOwner() == ControllerWebOwner::Right,
            "R2 did not own exact right web");
}

void TestGroundAndNonGatedInputRemainVanilla() {
    for (int gateCase = 0; gateCase < 6; ++gateCase) {
        ControllerWebInputPolicy policy;
        ControllerWebInputSample sample = Eligible();
        switch (gateCase) {
        case 0:
            sample.airborneProven = false;
            break;
        case 1:
            sample.leftShoulderHeld = false;
            break;
        case 2:
            sample.runtimeReady = false;
            break;
        case 3:
            sample.nativeSwingAvailable = false;
            break;
        case 4:
            sample.focused = false;
            break;
        case 5:
            sample.connected = false;
            break;
        default:
            break;
        }
        (void)policy.Update(sample);
        sample.leftTrigger = 0.8;
        sample.rightTrigger = 0.9;
        const ControllerWebInputDecision decision = policy.Update(sample);
        Require(!decision.consumeLeftTrigger &&
                    !decision.consumeRightTrigger &&
                    !HasOwnershipEffect(decision),
                "non-gated controller triggers were not vanilla");
    }
}

void TestFirstTriggerOwnsSingleRopeBothOrders() {
    ControllerWebInputPolicy leftFirst;
    ArmNeutral(leftFirst);
    ControllerWebInputSample sample = Eligible();
    sample.leftTrigger = 0.6;
    auto decision = leftFirst.Update(sample);
    Require(decision.leftAttach, "left-first owner did not attach");
    sample.rightTrigger = 0.7;
    decision = leftFirst.Update(sample);
    Require(decision.consumeRightTrigger && !decision.rightAttach &&
                !decision.nativeSwingPress &&
                leftFirst.CurrentOwner() == ControllerWebOwner::Left,
            "second right trigger stole left-owned rope");
    sample.rightTrigger = 0.0;
    decision = leftFirst.Update(sample);
    Require(decision.consumeRightTrigger && !decision.nativeSwingRelease &&
                leftFirst.CurrentOwner() == ControllerWebOwner::Left,
            "suppressed right release dropped left-owned rope");
    sample.leftTrigger = 0.0;
    decision = leftFirst.Update(sample);
    Require(decision.leftRelease && decision.nativeSwingRelease,
            "left-first owner did not release");

    ControllerWebInputPolicy rightFirst;
    ArmNeutral(rightFirst);
    sample = Eligible();
    sample.rightTrigger = 0.6;
    decision = rightFirst.Update(sample);
    Require(decision.rightAttach, "right-first owner did not attach");
    sample.leftTrigger = 0.7;
    decision = rightFirst.Update(sample);
    Require(decision.consumeLeftTrigger && !decision.leftAttach &&
                rightFirst.CurrentOwner() == ControllerWebOwner::Right,
            "second left trigger stole right-owned rope");
    sample.rightTrigger = 0.0;
    decision = rightFirst.Update(sample);
    Require(decision.rightRelease && decision.nativeSwingRelease &&
                !decision.leftAttach,
            "right release transferred ownership to held left trigger");
}

void TestSimultaneousPressFailsClosed() {
    ControllerWebInputPolicy policy;
    ArmNeutral(policy);
    ControllerWebInputSample sample = Eligible();
    sample.leftTrigger = 0.8;
    sample.rightTrigger = 0.8;
    auto decision = policy.Update(sample);
    Require(decision.consumeLeftTrigger && decision.consumeRightTrigger &&
                !HasOwnershipEffect(decision) &&
                policy.CurrentOwner() == ControllerWebOwner::None,
            "simultaneous press chose an arbitrary web side");

    sample.leftTrigger = 0.0;
    decision = policy.Update(sample);
    Require(decision.consumeLeftTrigger && !decision.rightAttach &&
                policy.CurrentOwner() == ControllerWebOwner::None,
            "ambiguous hold transferred after one trigger released");
    sample.rightTrigger = 0.0;
    decision = policy.Update(sample);
    Require(decision.consumeRightTrigger && !HasOwnershipEffect(decision),
            "ambiguous second release changed ownership");
    sample.rightTrigger = 0.5;
    decision = policy.Update(sample);
    Require(decision.rightAttach,
            "fresh edge after ambiguous chord did not recover");
}

void TestOwnerReleaseQuarantinesFreshOppositePress() {
    ControllerWebInputPolicy policy;
    ArmNeutral(policy);
    ControllerWebInputSample sample = Eligible();
    sample.leftTrigger = 0.6;
    (void)policy.Update(sample);
    sample.leftTrigger = 0.0;
    sample.rightTrigger = 0.6;
    auto decision = policy.Update(sample);
    Require(decision.leftRelease && decision.nativeSwingRelease &&
                decision.consumeRightTrigger && !decision.rightAttach &&
                !decision.nativeSwingPress &&
                policy.CurrentOwner() == ControllerWebOwner::None,
            "same-sample opposite press was not safely quarantined");
    decision = policy.Update(sample);
    Require(decision.consumeRightTrigger && !HasOwnershipEffect(decision),
            "held quarantined trigger stole ownership");
    sample.rightTrigger = 0.0;
    decision = policy.Update(sample);
    Require(decision.consumeRightTrigger && !policy.RightCaptured(),
            "opposite-trigger quarantine did not clear on release");
    sample.rightTrigger = 0.6;
    decision = policy.Update(sample);
    Require(decision.rightAttach && decision.nativeSwingPress &&
                policy.CurrentOwner() == ControllerWebOwner::Right,
            "fresh opposite press did not recover after neutral");
}

void TestGateLossReleasesAndQuarantinesUntilNeutral() {
    for (int lossCase = 0; lossCase < 4; ++lossCase) {
        ControllerWebInputPolicy policy;
        ArmNeutral(policy);
        ControllerWebInputSample sample = Eligible();
        sample.leftTrigger = 0.7;
        (void)policy.Update(sample);
        switch (lossCase) {
        case 0:
            sample.leftShoulderHeld = false;
            break;
        case 1:
            sample.airborneProven = false;
            break;
        case 2:
            sample.runtimeReady = false;
            break;
        case 3:
            sample.nativeSwingAvailable = false;
            break;
        default:
            break;
        }
        auto decision = policy.Update(sample);
        Require(decision.consumeLeftTrigger && decision.leftRelease &&
                    decision.nativeSwingRelease &&
                    !policy.NativeSwingHeld(),
                "gate loss left native Swing held");
        decision = policy.Update(sample);
        Require(decision.consumeLeftTrigger &&
                    !decision.nativeSwingRelease,
                "repeated gate loss emitted duplicate native up");
        sample.leftTrigger = 0.0;
        decision = policy.Update(sample);
        Require(decision.consumeLeftTrigger &&
                    !decision.nativeSwingRelease &&
                    !policy.LeftCaptured(),
                "gate-loss quarantine did not end at physical release");
    }
}

void TestFocusLossCannotLeaveSwingHeld() {
    ControllerWebInputPolicy policy;
    ArmNeutral(policy);
    ControllerWebInputSample sample = Eligible();
    sample.rightTrigger = 0.7;
    (void)policy.Update(sample);

    sample.focused = false;
    auto decision = policy.Update(sample);
    Require(decision.consumeRightTrigger && decision.rightRelease &&
                decision.nativeSwingRelease &&
                !policy.NativeSwingHeld(),
            "focus loss left native Swing held");
    decision = policy.Update(sample);
    Require(!decision.nativeSwingRelease,
            "repeated focus loss emitted duplicate native up");

    sample.focused = true;
    decision = policy.Update(sample);
    Require(decision.consumeRightTrigger && !decision.rightAttach,
            "focus regain recaptured a held trigger");
    sample.rightTrigger = 0.0;
    decision = policy.Update(sample);
    Require(decision.consumeRightTrigger && !policy.RightCaptured(),
            "focus-loss capture did not clear at neutral");
    sample.rightTrigger = 0.5;
    decision = policy.Update(sample);
    Require(decision.rightAttach,
            "fresh trigger after focus neutral did not attach");
}

void TestDisconnectAndSourceEpochCannotTransferOwnership() {
    ControllerWebInputPolicy disconnectPolicy;
    ArmNeutral(disconnectPolicy);
    ControllerWebInputSample sample = Eligible();
    sample.leftTrigger = 0.8;
    (void)disconnectPolicy.Update(sample);
    sample.connected = false;
    auto decision = disconnectPolicy.Update(sample);
    Require(decision.leftRelease && decision.nativeSwingRelease &&
                !disconnectPolicy.NativeSwingHeld(),
            "disconnect left native Swing held");
    decision = disconnectPolicy.Update(sample);
    Require(!decision.nativeSwingRelease,
            "repeated disconnect emitted duplicate native up");

    sample = Eligible(2U);
    sample.leftTrigger = 0.8;
    decision = disconnectPolicy.Update(sample);
    Require(!decision.consumeLeftTrigger && !decision.leftAttach,
            "new source inherited old held trigger");
    sample.leftTrigger = 0.0;
    (void)disconnectPolicy.Update(sample);
    sample.leftTrigger = 0.5;
    decision = disconnectPolicy.Update(sample);
    Require(decision.leftAttach,
            "new source did not arm after neutral");

    ControllerWebInputPolicy epochPolicy;
    ArmNeutral(epochPolicy, 10U);
    sample = Eligible(10U);
    sample.rightTrigger = 0.8;
    (void)epochPolicy.Update(sample);
    sample.sourceEpoch = 11U;
    decision = epochPolicy.Update(sample);
    Require(decision.rightRelease && decision.nativeSwingRelease &&
                !decision.rightAttach && !decision.consumeRightTrigger &&
                !epochPolicy.NativeSwingHeld(),
            "source epoch change transferred held ownership");
}

void TestNonFiniteInputFailsSafeAndRearms() {
    ControllerWebInputPolicy policy;
    ArmNeutral(policy);
    ControllerWebInputSample sample = Eligible();
    sample.leftTrigger = 0.7;
    (void)policy.Update(sample);
    sample.rightTrigger = std::numeric_limits<double>::quiet_NaN();
    auto decision = policy.Update(sample);
    Require(decision.consumeLeftTrigger && !decision.consumeRightTrigger &&
                decision.leftRelease && decision.nativeSwingRelease &&
                !policy.NativeSwingHeld(),
            "non-finite trigger altered an unowned axis or left Swing held");

    sample.rightTrigger = 0.0;
    decision = policy.Update(sample);
    Require(decision.consumeLeftTrigger && !decision.leftAttach,
            "valid high sample after fault recaptured without neutral");
    sample.leftTrigger = 0.0;
    (void)policy.Update(sample);
    sample.leftTrigger = 0.5;
    decision = policy.Update(sample);
    Require(decision.leftAttach,
            "policy did not recover after finite neutral sample");

    ControllerWebInputPolicy mirroredPolicy;
    ArmNeutral(mirroredPolicy);
    sample = Eligible();
    sample.rightTrigger = 0.7;
    (void)mirroredPolicy.Update(sample);
    sample.leftTrigger = std::numeric_limits<double>::infinity();
    decision = mirroredPolicy.Update(sample);
    Require(!decision.consumeLeftTrigger && decision.consumeRightTrigger &&
                decision.rightRelease && decision.nativeSwingRelease &&
                !mirroredPolicy.NativeSwingHeld(),
            "mirrored non-finite input altered the unowned axis");
}

struct Balances final {
    int native{};
    int left{};
    int right{};
};

void ApplyDecisionToBalances(const ControllerWebInputDecision& decision,
                             Balances& balances) {
    balances.native += decision.nativeSwingPress ? 1 : 0;
    balances.native -= decision.nativeSwingRelease ? 1 : 0;
    balances.left += decision.leftAttach ? 1 : 0;
    balances.left -= decision.leftRelease ? 1 : 0;
    balances.right += decision.rightAttach ? 1 : 0;
    balances.right -= decision.rightRelease ? 1 : 0;
}

void RequirePolicyInvariants(const ControllerWebInputPolicy& policy,
                             const ControllerWebInputDecision& decision,
                             const ControllerWebInputSample& sample,
                             const Balances& balances) {
    Require(balances.native >= 0 && balances.native <= 1,
            "native down/up balance left valid range");
    Require(balances.left >= 0 && balances.left <= 1 &&
                balances.right >= 0 && balances.right <= 1 &&
                balances.left + balances.right <= 1,
            "more than one web owner became active");
    Require((balances.native == 1) == policy.NativeSwingHeld(),
            "native edge balance disagreed with held state");
    Require(!decision.leftAttach || !decision.rightAttach,
            "one sample attached both sides");
    Require(!decision.nativeSwingPress ||
                decision.leftAttach || decision.rightAttach,
            "native down had no side attachment");
    Require(!(decision.nativeSwingPress && decision.nativeSwingRelease),
            "one sample requested an unsafe native Shift restart");
    if (policy.CurrentOwner() == ControllerWebOwner::Left) {
        Require(policy.LeftCaptured() && balances.left == 1 &&
                    balances.right == 0,
                "left owner lacked exact capture/balance");
    } else if (policy.CurrentOwner() == ControllerWebOwner::Right) {
        Require(policy.RightCaptured() && balances.right == 1 &&
                    balances.left == 0,
                "right owner lacked exact capture/balance");
    } else {
        Require(balances.left == 0 && balances.right == 0,
                "no-owner state retained side balance");
    }
    const bool validGate = sample.connected && sample.focused &&
                           sample.runtimeReady && sample.airborneProven &&
                           sample.leftShoulderHeld &&
                           sample.nativeSwingAvailable &&
                           std::isfinite(sample.leftTrigger) &&
                           std::isfinite(sample.rightTrigger);
    if (!validGate) {
        Require(!decision.leftAttach && !decision.rightAttach &&
                    !decision.nativeSwingPress,
                "invalid gate produced new ownership");
    }
}

void TestExhaustiveShortSequenceInvariants() {
    std::array<ControllerWebInputSample, 12> alphabet{};
    alphabet.fill(Eligible());
    alphabet[0].leftTrigger = 0.0;
    alphabet[0].rightTrigger = 0.0;
    alphabet[1].leftTrigger = 0.5;
    alphabet[2].rightTrigger = 0.5;
    alphabet[3].leftTrigger = 0.8;
    alphabet[3].rightTrigger = 0.8;
    alphabet[4].leftTrigger = 0.4;
    alphabet[5].leftShoulderHeld = false;
    alphabet[5].leftTrigger = 0.8;
    alphabet[6].airborneProven = false;
    alphabet[6].rightTrigger = 0.8;
    alphabet[7].runtimeReady = false;
    alphabet[8].focused = false;
    alphabet[9].connected = false;
    alphabet[10].sourceEpoch = 2U;
    alphabet[10].leftTrigger = 0.8;
    alphabet[11].rightTrigger =
        std::numeric_limits<double>::infinity();

    for (std::size_t first = 0; first < alphabet.size(); ++first) {
        for (std::size_t second = 0; second < alphabet.size(); ++second) {
            for (std::size_t third = 0; third < alphabet.size(); ++third) {
                for (std::size_t fourth = 0; fourth < alphabet.size();
                     ++fourth) {
                    ControllerWebInputPolicy policy;
                    ArmNeutral(policy);
                    Balances balances{};
                    const std::array<std::size_t, 4> sequence{
                        first, second, third, fourth};
                    for (const std::size_t index : sequence) {
                        const ControllerWebInputDecision decision =
                            policy.Update(alphabet[index]);
                        ApplyDecisionToBalances(decision, balances);
                        RequirePolicyInvariants(policy, decision,
                                                alphabet[index], balances);
                    }
                    const ControllerWebInputDecision stop =
                        policy.CancelAndReset();
                    ApplyDecisionToBalances(stop, balances);
                    Require(balances.native == 0 && balances.left == 0 &&
                                balances.right == 0 &&
                                !policy.NativeSwingHeld() &&
                                policy.CurrentOwner() ==
                                    ControllerWebOwner::None,
                            "terminal cancel left ownership active");
                    const ControllerWebInputDecision repeatedStop =
                        policy.CancelAndReset();
                    Require(!HasOwnershipEffect(repeatedStop),
                            "repeated terminal cancel emitted another edge");
                }
            }
        }
    }
}

} // namespace

int main() {
    const std::vector<std::pair<const char*, void (*)()>> tests{
        {"Schmitt thresholds and jitter", TestSchmittThresholdsAndJitter},
        {"fresh edge after startup and gate",
         TestFreshEdgeRequiredAfterStartupAndGateEntry},
        {"exact L2/R2 side mapping", TestExactLeftAndRightMapping},
        {"non-gated input remains vanilla",
         TestGroundAndNonGatedInputRemainVanilla},
        {"first trigger owns both orders",
         TestFirstTriggerOwnsSingleRopeBothOrders},
        {"simultaneous press fails closed",
         TestSimultaneousPressFailsClosed},
        {"release quarantines fresh opposite press",
         TestOwnerReleaseQuarantinesFreshOppositePress},
        {"gate loss releases and quarantines",
         TestGateLossReleasesAndQuarantinesUntilNeutral},
        {"focus loss releases", TestFocusLossCannotLeaveSwingHeld},
        {"disconnect and source epoch release",
         TestDisconnectAndSourceEpochCannotTransferOwnership},
        {"non-finite input fails safe", TestNonFiniteInputFailsSafeAndRearms},
        {"exhaustive short-sequence invariants",
         TestExhaustiveShortSequenceInvariants},
    };

    std::size_t passed = 0U;
    for (const auto& [name, test] : tests) {
        try {
            test();
            ++passed;
            std::cout << "PASS: " << name << '\n';
        } catch (const std::exception& error) {
            std::cerr << "FAIL: " << name << ": " << error.what() << '\n';
            return 1;
        }
    }
    std::cout << "Controller web input policy tests passed: " << passed << "/"
              << tests.size() << '\n';
    return 0;
}
