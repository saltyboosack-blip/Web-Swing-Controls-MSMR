#include "AirborneWebInputPolicy.h"
#include "ManualWebRuntime.h"
#include "NativeAnchorSidePolicy.h"
#include "SwingSteeringPolicy.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using trueswing::rebuild::runtime::AirborneWebInputPolicy;
using trueswing::rebuild::runtime::MouseButtonTransition;
using trueswing::rebuild::runtime::ClassifyManualWebEnter;
using trueswing::rebuild::runtime::ManualWebEnterKind;
using trueswing::rebuild::runtime::NativeAnchorCandidate;
using trueswing::rebuild::runtime::TrySelectNativeSideAnchor;
using trueswing::rebuild::runtime::WebInputEligibility;
using trueswing::rebuild::runtime::WebSide;
using trueswing::rebuild::runtime::ApplySwingMovementKeyTransition;
using trueswing::rebuild::runtime::ComputeSwingSteeringAcceleration;
using trueswing::rebuild::runtime::DecodeSwingMovementInput;
using trueswing::rebuild::runtime::SwingMovementInput;
using trueswing::rebuild::runtime::kSwingBackwardScanCode;
using trueswing::rebuild::runtime::kSwingForwardScanCode;
using trueswing::rebuild::runtime::kSwingLeftScanCode;
using trueswing::rebuild::runtime::kSwingRightScanCode;
using trueswing::rebuild::Vec3;

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

[[nodiscard]] WebInputEligibility Eligible() {
    return {.foreground = true,
            .runtimeReady = true,
            .airborneProven = true};
}

void TestGroundClicksRemainVanilla() {
    AirborneWebInputPolicy policy;
    WebInputEligibility gate = Eligible();
    gate.airborneProven = false;
    const auto down = policy.Update(MouseButtonTransition::LeftDown, gate);
    const auto up = policy.Update(MouseButtonTransition::LeftUp, gate);
    Require(!down.consume && !down.leftAttach && !down.nativeSwingPress,
            "ground left-down was captured");
    Require(!up.consume && !up.leftRelease && !up.nativeSwingRelease,
            "uncaptured ground left-up was captured");
}

void TestCapturedUpAlwaysReleasesAfterGateLoss() {
    AirborneWebInputPolicy policy;
    const auto down =
        policy.Update(MouseButtonTransition::LeftDown, Eligible());
    Require(down.consume && down.leftAttach && down.nativeSwingPress,
            "eligible left-down did not attach");

    WebInputEligibility lost{};
    const auto up = policy.Update(MouseButtonTransition::LeftUp, lost);
    Require(up.consume && up.leftRelease && up.nativeSwingRelease,
            "captured left-up leaked after gate loss");
}

void TestFirstButtonOwnsSingleNativeRope() {
    AirborneWebInputPolicy policy;
    const auto leftDown =
        policy.Update(MouseButtonTransition::LeftDown, Eligible());
    const auto rightDown =
        policy.Update(MouseButtonTransition::RightDown, Eligible());
    const auto rightUp =
        policy.Update(MouseButtonTransition::RightUp, Eligible());
    const auto leftUp =
        policy.Update(MouseButtonTransition::LeftUp, Eligible());

    Require(leftDown.nativeSwingPress && !rightDown.nativeSwingPress,
            "native Swing press was not edge-triggered");
    Require(leftDown.leftAttach && rightDown.consume &&
                !rightDown.rightAttach,
            "second button stole the single native rope");
    Require(rightUp.consume && !rightUp.rightRelease &&
                !rightUp.nativeSwingRelease,
            "suppressed second button released the owning rope");
    Require(leftUp.leftRelease && leftUp.nativeSwingRelease,
            "owning button did not release the native rope");
}

void TestCancelStopsWebsButConsumesFutureUps() {
    AirborneWebInputPolicy policy;
    (void)policy.Update(MouseButtonTransition::LeftDown, Eligible());
    (void)policy.Update(MouseButtonTransition::RightDown, Eligible());
    const auto cancel = policy.CancelNativeHold();
    Require(cancel.leftRelease && !cancel.rightRelease &&
                cancel.nativeSwingRelease,
            "fault cancellation did not release the owning web");

    const WebInputEligibility lost{};
    const auto leftUp = policy.Update(MouseButtonTransition::LeftUp, lost);
    const auto rightUp = policy.Update(MouseButtonTransition::RightUp, lost);
    Require(leftUp.consume && !leftUp.leftRelease &&
                !leftUp.nativeSwingRelease,
            "post-cancel left-up was not consumed exactly once");
    Require(rightUp.consume && !rightUp.rightRelease &&
                !rightUp.nativeSwingRelease,
            "post-cancel right-up was not consumed exactly once");
}

void TestDuplicateDownCannotDoubleAttach() {
    AirborneWebInputPolicy policy;
    (void)policy.Update(MouseButtonTransition::LeftDown, Eligible());
    const auto duplicate =
        policy.Update(MouseButtonTransition::LeftDown, Eligible());
    Require(duplicate.consume && !duplicate.leftAttach &&
                !duplicate.nativeSwingPress,
            "duplicate captured down retriggered attach");
}

void TestMatchingFilteredSubmitAuthorizesEnter() {
    const auto kind =
        ClassifyManualWebEnter(7U, false, true, false, 7U, true);
    Require(kind == ManualWebEnterKind::ActiveRequest,
            "matching filtered Submit did not authorize manual Enter");
}

void TestStaleOrUnfilteredEnterCannotBorrowRequest() {
    Require(ClassifyManualWebEnter(7U, true, false, false, 0U, false) ==
                ManualWebEnterKind::CanceledRequest,
            "canceled queued Submit became a vanilla Enter");
    Require(ClassifyManualWebEnter(7U, false, false, false, 0U, false) ==
                ManualWebEnterKind::CanceledRequest,
            "orphaned queued Submit became a vanilla Enter");
    Require(ClassifyManualWebEnter(0U, false, true, false, 8U, true) ==
                ManualWebEnterKind::CanceledRequest,
            "unfiltered Enter borrowed a new manual request");
    Require(ClassifyManualWebEnter(9U, false, true, false, 8U, true) ==
                ManualWebEnterKind::CanceledRequest,
            "old filtered Submit borrowed a replacement request");
    Require(ClassifyManualWebEnter(8U, false, true, true, 8U, true) ==
                ManualWebEnterKind::CanceledRequest,
            "canceling request authorized Enter");
    Require(ClassifyManualWebEnter(8U, false, true, false, 8U, false) ==
                ManualWebEnterKind::CanceledRequest,
            "landed/stale request authorized Enter");
}

void TestVanillaEnterNeedsNoManualPublication() {
    Require(ClassifyManualWebEnter(0U, false, false, false, 0U, false) ==
                ManualWebEnterKind::Vanilla,
            "ordinary vanilla Enter was suppressed");
}

void TestAmbiguousQueueEvidenceAlwaysFailsClosed() {
    Require(ClassifyManualWebEnter(0U, true, false, false, 0U, false, true) ==
                ManualWebEnterKind::CanceledRequest,
            "ambiguous queue evidence became vanilla ownership");
    Require(ClassifyManualWebEnter(12U, false, true, false, 12U, true, true) ==
                ManualWebEnterKind::CanceledRequest,
            "ambiguous queue evidence authorized custom ownership");
}

void TestCharacterSpaceSelectionIgnoresWorldAxis() {
    // Character faces +X, so its local right is world -Z. World X therefore
    // cannot be used to classify these candidates.
    const Vec3 origin{10.0, 2.0, 20.0};
    const Vec3 characterRight{0.0, 0.0, -2.0};
    const std::vector<NativeAnchorCandidate> candidates{
        {{100.0, 12.0, 15.0}, {100.0, 12.0, 15.0}, 4.0, 0U, true, false}, // local right
        {{-100.0, 15.0, 25.0}, {-100.0, 15.0, 25.0}, 8.0, 1U, true, false}, // local left
    };

    const auto left = TrySelectNativeSideAnchor(
        std::span<const NativeAnchorCandidate>(candidates), origin,
        characterRight, WebSide::Left);
    const auto right = TrySelectNativeSideAnchor(
        std::span<const NativeAnchorCandidate>(candidates), origin,
        characterRight, WebSide::Right);
    Require(left.has_value() && left->nativeIndex == 1U &&
                left->signedCharacterRightDistance < 0.0,
            "left selection used world axis instead of character right");
    Require(right.has_value() && right->nativeIndex == 0U &&
                right->signedCharacterRightDistance > 0.0,
            "right selection used world axis instead of character right");
}

void TestNativeValidityAndScoreRemainAuthoritative() {
    const std::vector<NativeAnchorCandidate> candidates{
        {{-4.0, 8.0, 0.0}, {-4.0, 8.0, 0.0}, 10.0, 0U, false, false},
        {{-5.0, 9.0, 0.0}, {-5.0, 9.0, 0.0}, 20.0, 1U, true, true},
        {{-6.0, 10.0, 0.0}, {-6.0, 10.0, 0.0}, 3.0, 2U, true, false},
        {{-7.0, 11.0, 0.0}, {-7.0, 11.0, 0.0}, 9.0, 3U, true, false},
        {{4.0, 12.0, 0.0}, {4.0, 12.0, 0.0}, 100.0, 4U, true, false},
    };
    const auto selected = TrySelectNativeSideAnchor(
        std::span<const NativeAnchorCandidate>(candidates), {}, {1.0, 0.0, 0.0},
        WebSide::Left);
    Require(selected.has_value() && selected->nativeIndex == 3U &&
                std::abs(selected->nativeScore - 9.0) < 1.0e-12,
            "selector bypassed native validity/reject/score rules");
}

void TestAnchorAndNativeHandMustShareRequestedSide() {
    const std::vector<NativeAnchorCandidate> candidates{
        // The physical pivot is left, but the game's alternate hand point is
        // right. This record must not produce a left-hand request.
        {{-8.0, 12.0, 0.0}, {6.0, 10.0, 0.0}, 100.0, 0U, true, false},
        {{-5.0, 11.0, 0.0}, {-4.0, 9.0, 0.0}, 8.0, 1U, true, false},
    };
    const auto selected = TrySelectNativeSideAnchor(
        std::span<const NativeAnchorCandidate>(candidates), {},
        {1.0, 0.0, 0.0}, WebSide::Left);
    Require(selected.has_value() && selected->nativeIndex == 1U,
            "opposite native hand point bypassed side filter");
}

void TestBoundaryInvalidAndTieRules() {
    const std::vector<NativeAnchorCandidate> candidates{
        {{0.0, 9.0, 0.0}, {0.0, 9.0, 0.0}, 100.0, 0U, true, false},
        {{3.0, 10.0, 0.0}, {3.0, 10.0, 0.0}, 5.0, 1U, true, false},
        {{4.0, 11.0, 0.0}, {4.0, 11.0, 0.0}, 5.0, 2U, true, false},
    };
    const auto selected = TrySelectNativeSideAnchor(
        std::span<const NativeAnchorCandidate>(candidates), {}, {2.0, 0.0, 0.0},
        WebSide::Right);
    Require(selected.has_value() && selected->nativeIndex == 1U,
            "selector did not preserve native first-on-tie behavior");

    const auto invalidBasis = TrySelectNativeSideAnchor(
        std::span<const NativeAnchorCandidate>(candidates), {}, {},
        WebSide::Right);
    Require(!invalidBasis.has_value(), "zero character-right basis was accepted");

    std::vector<NativeAnchorCandidate> nonFinite = candidates;
    nonFinite[1].nativeScore = std::numeric_limits<double>::infinity();
    nonFinite[2].worldAnchor.x = std::numeric_limits<double>::quiet_NaN();
    const auto none = TrySelectNativeSideAnchor(
        std::span<const NativeAnchorCandidate>(nonFinite), {}, {1.0, 0.0, 0.0},
        WebSide::Left);
    Require(!none.has_value(), "invalid candidates produced a side anchor");
}

void TestMovementKeyMaskUsesExactRawScanCodes() {
    std::uint32_t mask = 0U;
    mask = ApplySwingMovementKeyTransition(mask, kSwingForwardScanCode, true);
    const std::uint32_t afterFirstMake = mask;
    mask = ApplySwingMovementKeyTransition(mask, kSwingForwardScanCode, true);
    Require(mask == afterFirstMake, "repeated W make changed held state");
    mask = ApplySwingMovementKeyTransition(mask, kSwingLeftScanCode, true);
    mask = ApplySwingMovementKeyTransition(mask, kSwingBackwardScanCode, true);
    mask = ApplySwingMovementKeyTransition(mask, kSwingRightScanCode, true);
    const SwingMovementInput all = DecodeSwingMovementInput(mask);
    Require(all.forward && all.left && all.backward && all.right,
            "W/A/S/D scan-code mask did not decode all held keys");

    const std::uint32_t beforeIgnored = mask;
    mask = ApplySwingMovementKeyTransition(mask, 0x12U, true);
    mask = ApplySwingMovementKeyTransition(mask, kSwingForwardScanCode, false,
                                           true);
    Require(mask == beforeIgnored,
            "unrelated or extended scan code changed movement state");

    mask = ApplySwingMovementKeyTransition(mask, kSwingForwardScanCode, false);
    mask = ApplySwingMovementKeyTransition(mask, kSwingForwardScanCode, false);
    const SwingMovementInput released = DecodeSwingMovementInput(mask);
    Require(!released.forward && released.left && released.backward &&
                released.right,
            "repeated W break disturbed another held key");
}

void TestSteeringFollowsMomentumAndCorrectedRight() {
    const Vec3 correctedRight{1.0, 0.4, 0.0};
    const Vec3 momentum{0.0, -20.0, 12.0};

    const Vec3 forward = ComputeSwingSteeringAcceleration(
        SwingMovementInput{.forward = true}, momentum, correctedRight);
    const Vec3 backward = ComputeSwingSteeringAcceleration(
        SwingMovementInput{.backward = true}, momentum, correctedRight);
    const Vec3 left = ComputeSwingSteeringAcceleration(
        SwingMovementInput{.left = true}, momentum, correctedRight);
    const Vec3 right = ComputeSwingSteeringAcceleration(
        SwingMovementInput{.right = true}, momentum, correctedRight);

    Require(std::abs(forward.x) < 1.0e-12 &&
                std::abs(forward.y) < 1.0e-12 &&
                std::abs(forward.z - 8.0) < 1.0e-12,
            "W did not accelerate along horizontal momentum");
    Require(std::abs(backward.z + 8.0) < 1.0e-12,
            "S did not accelerate against horizontal momentum");
    Require(std::abs(left.x + 8.0) < 1.0e-12 &&
                std::abs(right.x - 8.0) < 1.0e-12,
            "A/D did not use corrected character-right handedness");
}

void TestSteeringNormalizesDiagonalsAndCancelsOpposites() {
    const Vec3 diagonal = ComputeSwingSteeringAcceleration(
        SwingMovementInput{.forward = true, .right = true},
        {0.0, 0.0, 10.0}, {1.0, 0.0, 0.0});
    Require(std::abs(diagonal.Length() - 8.0) < 1.0e-12 &&
                diagonal.x > 0.0 && diagonal.z > 0.0,
            "diagonal steering was faster or used a wrong quadrant");

    const Vec3 canceled = ComputeSwingSteeringAcceleration(
        SwingMovementInput{.forward = true,
                           .left = true,
                           .backward = true,
                           .right = true},
        {0.0, 0.0, 10.0}, {1.0, 0.0, 0.0});
    Require(canceled.LengthSquared() == 0.0,
            "opposing steering keys did not cancel");
}

void TestLowSpeedSteeringUsesCorrectedCharacterBasis() {
    const Vec3 forward = ComputeSwingSteeringAcceleration(
        SwingMovementInput{.forward = true}, {0.1, -2.0, 0.0},
        {2.0, 0.0, 0.0});
    const Vec3 right = ComputeSwingSteeringAcceleration(
        SwingMovementInput{.right = true}, {0.1, -2.0, 0.0},
        {2.0, 0.0, 0.0});
    Require(std::abs(forward.z - 8.0) < 1.0e-12 &&
                std::abs(right.x - 8.0) < 1.0e-12,
            "low-speed steering did not use corrected character basis");

    const Vec3 invalid = ComputeSwingSteeringAcceleration(
        SwingMovementInput{.forward = true}, {}, {});
    Require(invalid.LengthSquared() == 0.0,
            "invalid character basis produced steering acceleration");
}

} // namespace

int main() {
    const std::vector<std::pair<const char*, void (*)()>> tests{
        {"ground clicks remain vanilla", TestGroundClicksRemainVanilla},
        {"captured up survives gate loss", TestCapturedUpAlwaysReleasesAfterGateLoss},
        {"first button owns native rope", TestFirstButtonOwnsSingleNativeRope},
        {"cancel keeps raw capture latches", TestCancelStopsWebsButConsumesFutureUps},
        {"duplicate down cannot reattach", TestDuplicateDownCannotDoubleAttach},
        {"matching filtered submit authorizes enter",
         TestMatchingFilteredSubmitAuthorizesEnter},
        {"stale enter cannot borrow request",
         TestStaleOrUnfilteredEnterCannotBorrowRequest},
        {"vanilla enter remains vanilla",
         TestVanillaEnterNeedsNoManualPublication},
        {"ambiguous queue evidence fails closed",
         TestAmbiguousQueueEvidenceAlwaysFailsClosed},
        {"character-space side selection", TestCharacterSpaceSelectionIgnoresWorldAxis},
        {"native validity and score", TestNativeValidityAndScoreRemainAuthoritative},
        {"anchor and native hand side",
         TestAnchorAndNativeHandMustShareRequestedSide},
        {"boundary invalid and tie rules", TestBoundaryInvalidAndTieRules},
        {"movement raw scan-code mask",
         TestMovementKeyMaskUsesExactRawScanCodes},
        {"steering momentum frame",
         TestSteeringFollowsMomentumAndCorrectedRight},
        {"steering diagonal and cancellation",
         TestSteeringNormalizesDiagonalsAndCancelsOpposites},
        {"steering low-speed fallback",
         TestLowSpeedSteeringUsesCorrectedCharacterBasis},
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
    std::cout << "Manual web policy tests passed: " << passed << "/"
              << tests.size() << '\n';
    return 0;
}
