#include "AirborneWebInputPolicy.h"
#include "NativeAnchorSidePolicy.h"

#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using trueswing::rebuild::Vec3;
using trueswing::rebuild::runtime::AirborneWebInputPolicy;
using trueswing::rebuild::runtime::MouseButtonTransition;
using trueswing::rebuild::runtime::NativeAnchorCandidate;
using trueswing::rebuild::runtime::TrySelectNativeSideAnchor;
using trueswing::rebuild::runtime::WebInputEligibility;
using trueswing::rebuild::runtime::WebSide;

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

[[nodiscard]] WebInputEligibility Eligible() noexcept {
    return {.foreground = true,
            .runtimeReady = true,
            .airborneProven = true};
}

void TestGroundClicksRemainVanilla() {
    AirborneWebInputPolicy policy;
    WebInputEligibility gate = Eligible();
    gate.airborneProven = false;
    const auto left = policy.Update(MouseButtonTransition::LeftDown, gate);
    const auto right = policy.Update(MouseButtonTransition::RightDown, gate);
    Require(!left.consume && !right.consume,
            "a ground mouse-down was captured");
}

void TestLeftButtonOwnsLeftWeb() {
    AirborneWebInputPolicy policy;
    const auto down =
        policy.Update(MouseButtonTransition::LeftDown, Eligible());
    const auto up = policy.Update(MouseButtonTransition::LeftUp, {});
    Require(down.consume && down.leftAttach && !down.rightAttach &&
                down.nativeSwingPress,
            "LMB did not request the left native web");
    Require(up.consume && up.leftRelease && up.nativeSwingRelease,
            "LMB release did not release its native web");
}

void TestRightButtonOwnsRightWeb() {
    AirborneWebInputPolicy policy;
    const auto down =
        policy.Update(MouseButtonTransition::RightDown, Eligible());
    const auto up = policy.Update(MouseButtonTransition::RightUp, {});
    Require(down.consume && down.rightAttach && !down.leftAttach &&
                down.nativeSwingPress,
            "RMB did not request the right native web");
    Require(up.consume && up.rightRelease && up.nativeSwingRelease,
            "RMB release did not release its native web");
}

void TestSpaceLeftClickRemainsNative() {
    AirborneWebInputPolicy policy;
    WebInputEligibility gate = Eligible();
    gate.nativeLeftActionChordHeld = true;
    const auto down =
        policy.Update(MouseButtonTransition::LeftDown, gate);
    const auto up = policy.Update(MouseButtonTransition::LeftUp, {});
    Require(!down.consume && !down.leftAttach && !down.nativeSwingPress,
            "Space+LMB was captured instead of reaching Ground Strike");
    Require(!up.consume,
            "uncaptured Ground Strike LMB release was consumed");
}

void TestLeftClickRemainsNativeDuringRightWeb() {
    AirborneWebInputPolicy policy;
    const auto right =
        policy.Update(MouseButtonTransition::RightDown, Eligible());
    const auto left =
        policy.Update(MouseButtonTransition::LeftDown, Eligible());
    const auto leftUp =
        policy.Update(MouseButtonTransition::LeftUp, Eligible());
    const auto rightUp =
        policy.Update(MouseButtonTransition::RightUp, Eligible());
    Require(right.rightAttach && right.nativeSwingPress,
            "RMB did not own the rope");
    Require(!left.consume && !left.leftAttach && !left.nativeSwingPress,
            "LMB was captured instead of reaching Swing Kick");
    Require(!leftUp.consume && !leftUp.nativeSwingRelease,
            "native Swing Kick LMB release was consumed");
    Require(rightUp.consume && rightUp.nativeSwingRelease,
            "RMB release did not drop its rope");
}

void TestFirstButtonKeepsSingleRope() {
    AirborneWebInputPolicy policy;
    const auto left =
        policy.Update(MouseButtonTransition::LeftDown, Eligible());
    const auto right =
        policy.Update(MouseButtonTransition::RightDown, Eligible());
    const auto rightUp =
        policy.Update(MouseButtonTransition::RightUp, Eligible());
    const auto leftUp =
        policy.Update(MouseButtonTransition::LeftUp, Eligible());
    Require(left.leftAttach && left.nativeSwingPress,
            "first button did not own the rope");
    Require(right.consume && !right.rightAttach &&
                !right.nativeSwingPress,
            "second button stole the rope");
    Require(rightUp.consume && !rightUp.nativeSwingRelease,
            "second-button release dropped the rope");
    Require(leftUp.nativeSwingRelease,
            "owning-button release did not drop the rope");
}

void TestCapturedReleaseSurvivesGateLoss() {
    AirborneWebInputPolicy policy;
    (void)policy.Update(MouseButtonTransition::LeftDown, Eligible());
    const auto up = policy.Update(MouseButtonTransition::LeftUp, {});
    Require(up.consume && up.leftRelease && up.nativeSwingRelease,
            "captured release leaked after eligibility loss");
}

void TestCancelCannotLeaveSwingHeld() {
    AirborneWebInputPolicy policy;
    (void)policy.Update(MouseButtonTransition::RightDown, Eligible());
    const auto cancel = policy.CancelNativeHold();
    const auto physicalUp =
        policy.Update(MouseButtonTransition::RightUp, {});
    Require(cancel.rightRelease && cancel.nativeSwingRelease,
            "cancel did not release the native Swing hold");
    Require(physicalUp.consume && !physicalUp.nativeSwingRelease,
            "post-cancel physical up was not consumed exactly once");
}

void TestCharacterLocalSideMapping() {
    const Vec3 origin{10.0, 2.0, 20.0};
    const Vec3 characterRight{0.0, 0.0, -2.0};
    const std::vector<NativeAnchorCandidate> candidates{
        {{100.0, 12.0, 15.0}, {100.0, 12.0, 15.0}, 4.0, 0U, true, false},
        {{-100.0, 15.0, 25.0}, {-100.0, 15.0, 25.0}, 8.0, 1U, true, false},
    };
    const auto left = TrySelectNativeSideAnchor(
        std::span<const NativeAnchorCandidate>(candidates), origin,
        characterRight, WebSide::Left);
    const auto right = TrySelectNativeSideAnchor(
        std::span<const NativeAnchorCandidate>(candidates), origin,
        characterRight, WebSide::Right);
    Require(left.has_value() && left->nativeIndex == 1U,
            "LMB side selection used a world axis");
    Require(right.has_value() && right->nativeIndex == 0U,
            "RMB side selection used a world axis");
}

void TestNativeValidityHandSideAndScoreStayAuthoritative() {
    const std::vector<NativeAnchorCandidate> candidates{
        {{-8.0, 12.0, 0.0}, {6.0, 10.0, 0.0}, 100.0, 0U, true, false},
        {{-5.0, 11.0, 0.0}, {-4.0, 9.0, 0.0}, 8.0, 1U, true, false},
        {{-7.0, 13.0, 0.0}, {-6.0, 10.0, 0.0}, 20.0, 2U, false, false},
        {{-9.0, 14.0, 0.0}, {-8.0, 12.0, 0.0}, 30.0, 3U, true, true},
    };
    const auto selected = TrySelectNativeSideAnchor(
        std::span<const NativeAnchorCandidate>(candidates), {},
        {1.0, 0.0, 0.0}, WebSide::Left);
    Require(selected.has_value() && selected->nativeIndex == 1U,
            "selector bypassed native validity, rejection, hand side, or score");
}

void TestBoundaryAndInvalidCandidatesFailClosed() {
    std::vector<NativeAnchorCandidate> candidates{
        {{0.0, 9.0, 0.0}, {0.0, 9.0, 0.0}, 100.0, 0U, true, false},
        {{3.0, 10.0, 0.0}, {3.0, 10.0, 0.0}, 5.0, 1U, true, false},
    };
    const auto boundary = TrySelectNativeSideAnchor(
        std::span<const NativeAnchorCandidate>(candidates), {},
        {1.0, 0.0, 0.0}, WebSide::Left);
    Require(!boundary.has_value(), "center/right candidates produced a left web");
    candidates[1].nativeScore = std::numeric_limits<double>::infinity();
    const auto invalid = TrySelectNativeSideAnchor(
        std::span<const NativeAnchorCandidate>(candidates), {},
        {1.0, 0.0, 0.0}, WebSide::Right);
    Require(!invalid.has_value(), "non-finite candidate was accepted");
}

} // namespace

int main() {
    const std::vector<std::pair<const char*, void (*)()>> tests{
        {"ground clicks remain vanilla", TestGroundClicksRemainVanilla},
        {"LMB owns left web", TestLeftButtonOwnsLeftWeb},
        {"RMB owns right web", TestRightButtonOwnsRightWeb},
        {"Space+LMB remains native", TestSpaceLeftClickRemainsNative},
        {"LMB remains native during right web",
         TestLeftClickRemainsNativeDuringRightWeb},
        {"first button keeps single rope", TestFirstButtonKeepsSingleRope},
        {"captured release survives gate loss",
         TestCapturedReleaseSurvivesGateLoss},
        {"cancel cannot leave Swing held", TestCancelCannotLeaveSwingHeld},
        {"character-local side mapping", TestCharacterLocalSideMapping},
        {"native candidate authority",
         TestNativeValidityHandSideAndScoreStayAuthoritative},
        {"boundary and invalid candidates fail closed",
         TestBoundaryAndInvalidCandidatesFailClosed},
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
    std::cout << "WebSwingControls policy tests passed: " << passed << "/"
              << tests.size() << '\n';
    return 0;
}
