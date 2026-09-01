#pragma once

#include <cmath>
#include <cstddef>
#include <optional>
#include <span>

#include "AirborneWebInputPolicy.h"
#include "trueswing/Vec3.h"

namespace trueswing::rebuild::runtime {

struct NativeAnchorCandidate final {
    Vec3 worldAnchor{};
    Vec3 worldHandPoint{};
    double nativeScore{};
    std::size_t nativeIndex{};
    bool nativeValid{};
    bool nativeRejected{};
};

struct SideAnchorSelection final {
    Vec3 worldAnchor{};
    double nativeScore{};
    double signedCharacterRightDistance{};
    std::size_t nativeIndex{};
};

// Selects only candidates already accepted by SwingPointHunter. Both the
// physical pivot and the effective point used by HeroStateSwing to derive its
// native hand must be on the requested character-local side. Side is never
// measured by world X, camera X, or a mirrored/offset stock winner. Native
// scoring remains primary.
[[nodiscard]] inline std::optional<SideAnchorSelection>
TrySelectNativeSideAnchor(std::span<const NativeAnchorCandidate> candidates,
                          const Vec3& characterOrigin,
                          const Vec3& characterRight, WebSide requestedSide,
                          double minimumSideDistance = 1.0e-6) noexcept {
    if (!characterOrigin.IsFinite() || !characterRight.IsFinite() ||
        !std::isfinite(minimumSideDistance) || minimumSideDistance < 0.0) {
        return std::nullopt;
    }

    const double rightLength = characterRight.Length();
    if (!std::isfinite(rightLength) || rightLength <= 1.0e-9) {
        return std::nullopt;
    }
    const Vec3 normalizedRight = characterRight / rightLength;

    std::optional<SideAnchorSelection> best;
    for (const NativeAnchorCandidate& candidate : candidates) {
        if (!candidate.nativeValid || candidate.nativeRejected ||
            !candidate.worldAnchor.IsFinite() ||
            !candidate.worldHandPoint.IsFinite() ||
            !std::isfinite(candidate.nativeScore)) {
            continue;
        }

        const double sideDistance =
            (candidate.worldAnchor - characterOrigin).Dot(normalizedRight);
        if (!std::isfinite(sideDistance)) {
            continue;
        }
        const double handSideDistance =
            (candidate.worldHandPoint - characterOrigin).Dot(normalizedRight);
        if (!std::isfinite(handSideDistance)) {
            continue;
        }
        const bool anchorOnRequestedSide =
            requestedSide == WebSide::Left
                ? sideDistance < -minimumSideDistance
                : sideDistance > minimumSideDistance;
        const bool handOnRequestedSide =
            requestedSide == WebSide::Left
                ? handSideDistance < -minimumSideDistance
                : handSideDistance > minimumSideDistance;
        if (!anchorOnRequestedSide || !handOnRequestedSide) {
            continue;
        }

        // SwingPointHunter itself keeps the first record on equal score and
        // replaces it only for a strictly larger score. Preserve that rule.
        if (!best.has_value() || candidate.nativeScore > best->nativeScore) {
            best = SideAnchorSelection{candidate.worldAnchor,
                                       candidate.nativeScore, sideDistance,
                                       candidate.nativeIndex};
        }
    }
    return best;
}

} // namespace trueswing::rebuild::runtime
