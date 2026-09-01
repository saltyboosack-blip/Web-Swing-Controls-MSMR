#include "ManualWebRuntime.h"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <intrin.h>

#include "AirborneWebInputPolicy.h"
#if defined(TRUESWING_CONTROLS_ONLY)
#include "ControllerSourceRoute.h"
#include "ControllerWebInputPolicy.h"
#endif
#include "MemoryAccess.h"
#include "MoverBinding.h"
#include "NativeAnchorSidePolicy.h"
#if !defined(TRUESWING_CONTROLS_ONLY)
#include "SwingSteeringPolicy.h"
#endif

namespace trueswing::rebuild::runtime {
namespace {

// Exact Steam build 23986256. GameBuild verifies the complete executable and
// .text hashes before these local signatures are accepted.
constexpr std::uintptr_t kSwingPointHunterVtableRva = 0x038AC210;
constexpr std::uintptr_t kNativeInputVtableRva = 0x04F3C1B8;
constexpr std::uintptr_t kSwingPointHunterUpdateRva = 0x0086C4A0;
constexpr std::uintptr_t kSwingPointSubmitRva = 0x00871F60;
constexpr std::uintptr_t kSwingPointAggregateRva = 0x00874F60;
constexpr std::uintptr_t kNativeInputDispatchRva = 0x01CE3920;
constexpr std::uintptr_t kNativeKeyboardParserRva = 0x01CE30F0;
#if defined(TRUESWING_CONTROLS_ONLY)
constexpr std::uintptr_t kNativeInputDeviceUpdateRva = 0x01CE2AB0;
constexpr std::uintptr_t kNativeMainGamepadGetterEvidenceRva = 0x01CE0910;
constexpr std::uintptr_t kNativeInputOrderingEvidenceRva = 0x01CE214A;
constexpr std::uintptr_t kNativeL1MappingEvidenceRva = 0x01D16941;
constexpr std::uintptr_t kNativeTriggerMappingEvidenceRva = 0x01D16A04;
constexpr std::uintptr_t kNativeL2AggregateEvidenceRva = 0x01CE263F;
constexpr std::uintptr_t kNativeR2AggregateEvidenceRva = 0x01CE26B9;
#endif
constexpr std::uintptr_t kNativeIntSettingGetterRva = 0x0072D7E0;
constexpr std::uintptr_t kAccessibilitySettingsRva = 0x05D96DD0;
constexpr std::uintptr_t kSwingToggleEvidenceRva = 0x007F019C;
constexpr std::uintptr_t kAirborneEvidenceRva = 0x01FBED75;
constexpr std::uintptr_t kAirborneStoreEvidenceRva = 0x01FBEE93;
constexpr std::uintptr_t kActionRecordBaseRva = 0x06E067C0;
constexpr std::uintptr_t kActionRecordCountRva = 0x06E06DC0;
constexpr std::uintptr_t kSwingActionNameRva = 0x038DF588;
constexpr std::uintptr_t kSwingActionRegistrationEvidenceRva = 0x00B9D562;
constexpr std::uintptr_t kActionBindingGetterEvidenceRva = 0x01C9CDA0;
constexpr std::uintptr_t kNativeHandDerivationEvidenceRva = 0x00AB2F38;
constexpr std::uintptr_t kNativeRightHandPredicateEvidenceRva = 0x00AB960C;
constexpr std::uintptr_t kNativeRightHandSetEvidenceRva = 0x00AB9640;
constexpr std::uintptr_t kNativeLeftHandSetEvidenceRva = 0x00AB352F;
constexpr std::uintptr_t kNativeHandMirrorFlagRva = 0x06DF2615;
#if !defined(TRUESWING_CONTROLS_ONLY)
constexpr std::uintptr_t kSwingTransitionPayloadSizeEvidenceRva = 0x00AB385C;
constexpr std::uintptr_t kSwingTransitionPayloadCopyEvidenceRva = 0x00AB5DA0;
constexpr std::uintptr_t kSwingTransitionPayloadReadEvidenceRva = 0x00AB2B41;
constexpr std::uintptr_t kSwingTransitionQueueCopyEvidenceRva = 0x020DF988;
constexpr std::uintptr_t kRequestTransitionRva = 0x020DF4A0;
constexpr std::uintptr_t kExecuteTransitionRva = 0x020DCF10;
constexpr std::uintptr_t kSwingStateKeyRva = 0x06DF2690;
constexpr std::uintptr_t kPrimarySwingRequestReturnRva = 0x0097C987;
constexpr std::uintptr_t kAlternateSwingRequestReturnRva = 0x00993651;
#endif

constexpr std::uintptr_t kHunterStateHostOffset = 0x8;
constexpr std::uintptr_t kHunterActorRightOffset = 0xE880;
constexpr std::uintptr_t kHunterOriginOffset = 0xE8B0;
constexpr std::uintptr_t kHunterSelectedRecordOffset = 0xE9BC;
constexpr std::uintptr_t kHunterSelectedValidOffset = 0xEA18;
constexpr std::uintptr_t kHunterCandidateBankOffset = 0xEA1C;
constexpr std::uintptr_t kHunterCandidateCountOffset = 0xF01C;
constexpr std::uintptr_t kHunterCandidatesPendingOffset = 0xF020;
constexpr std::size_t kHunterCandidateStride = 0x60;
constexpr std::size_t kHunterMaximumCandidates = 16U;
constexpr std::uintptr_t kCandidatePivotOffset = 0x24;
constexpr std::uintptr_t kCandidateAlternateHandPointOffset = 0x3C;
constexpr std::uintptr_t kCandidateScoreOffset = 0x58;
constexpr std::uintptr_t kCandidateValidOffset = 0x5C;
constexpr std::uintptr_t kCandidateUsesAlternateHandPointOffset = 0x5D;
constexpr std::uintptr_t kCandidateRejectedOffset = 0x5E;

constexpr std::uintptr_t kMoverAirborneOffset = 0x6EE;
constexpr std::uintptr_t kKeyboardStateBaseOffset = 0xF8;
constexpr std::uintptr_t kMouseInputEnabledOffset = 0x55C;
#if defined(TRUESWING_CONTROLS_ONLY)
constexpr std::uintptr_t kInputDeviceArrayOffset = 0x590;
constexpr std::uintptr_t kPrimaryPlayerDeviceIndexOffset = 0x690;
constexpr std::size_t kInputDeviceCapacity = 20U;
constexpr std::uintptr_t kGamepadConnectedOffset = 0x9;
constexpr std::uintptr_t kGamepadTypeOffset = 0x90;
constexpr std::uintptr_t kGamepadLeftShoulderStateOffset = 0x30;
constexpr std::uintptr_t kGamepadLeftTriggerOffset = 0x48;
constexpr std::uintptr_t kGamepadRightTriggerOffset = 0x4C;
constexpr std::uint32_t kNativeButtonHeldMask = 4U;
constexpr std::uint32_t kLastNativeGamepadType = 11U;
constexpr std::uintptr_t kWindowsGamingInputGamepadVtableRva = 0x04F3C148;
constexpr std::uintptr_t kSceGamepadVtableRva = 0x04F435F0;
constexpr std::uintptr_t kSteamInputGamepadVtableRva = 0x04F439A0;
constexpr std::uintptr_t kXInputGamepadVtableRva = 0x04F44100;
#endif
constexpr std::uint32_t kLeftShiftScanCode = 0x2A;
constexpr std::uint32_t kSwingUseToggleSettingId = 0x2D;
constexpr std::size_t kActionRecordStride = 0x30;
constexpr std::size_t kSwingActionRecordIndex = 5U;
constexpr std::uintptr_t kActionKeyboardBindingBaseOffset = 0xB4;
#if !defined(TRUESWING_CONTROLS_ONLY)
constexpr std::uintptr_t kSwingTransitionFirstPointOffset = 0x24;
constexpr std::uintptr_t kSwingTransitionSecondPointOffset = 0x30;
constexpr std::size_t kSwingTransitionPayloadSize = 0x70U;
constexpr std::uint32_t kSwingTransitionType = 0x01378FEFU;
#endif
constexpr ULONGLONG kHeroPublicationMaximumAgeMs = 500ULL;
#if !defined(TRUESWING_CONTROLS_ONLY)
constexpr std::size_t kQueuedSubmitEvidenceCapacity = 64U;
#endif

constexpr std::array<std::uint8_t, 20> kHunterUpdateSignature{
    0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x10, 0x48, 0x89, 0x70,
    0x18, 0x48, 0x89, 0x78, 0x20, 0x55, 0x41, 0x56, 0x41, 0x57};
constexpr std::array<std::uint8_t, 20> kSwingPointSubmitSignature{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10,
    0x48, 0x89, 0x74, 0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x30};
constexpr std::array<std::uint8_t, 16> kSwingPointAggregateSignature{
    0x48, 0x89, 0x74, 0x24, 0x18, 0x57, 0x48, 0x83,
    0xEC, 0x20, 0x0F, 0x57, 0xC0, 0x48, 0x89, 0x6C};
constexpr std::array<std::uint8_t, 16> kInputDispatchSignature{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74,
    0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x49};
#if defined(TRUESWING_CONTROLS_ONLY)
constexpr std::array<std::uint8_t, 22> kInputDeviceUpdateSignature{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x60, 0x48, 0x8B, 0xD9, 0x84,
    0xD2, 0x0F, 0x84, 0x90, 0x03, 0x00, 0x00, 0xE8, 0xCA, 0xF2,
    0xFB, 0xFF};
constexpr std::array<std::uint8_t, 32> kNativeMainGamepadGetterEvidence{
    0x48, 0x63, 0xC2, 0x48, 0x83, 0xC0, 0x54, 0x48, 0x8D, 0x04,
    0x80, 0x48, 0x63, 0x14, 0x81, 0x83, 0xFA, 0xFF, 0x75, 0x03,
    0x33, 0xC0, 0xC3, 0x48, 0x8B, 0x84, 0xD1, 0x90, 0x05, 0x00,
    0x00, 0xC3};
constexpr std::array<std::uint8_t, 25> kNativeInputOrderingEvidence{
    0x41, 0x0F, 0xB6, 0xD6, 0x48, 0x8B, 0xCB, 0xE8, 0x5A,
    0x09, 0x00, 0x00, 0xE8, 0x65, 0x14, 0x03, 0x00, 0x48,
    0x8B, 0xCB, 0xE8, 0x8D, 0x19, 0x00, 0x00};
constexpr std::array<std::uint8_t, 33> kNativeL1MappingEvidence{
    0x0F, 0xB7, 0x54, 0x24, 0x34, 0x4C, 0x8D, 0x43, 0x10, 0x08,
    0x43, 0x08, 0x41, 0xB9, 0x98, 0x01, 0x00, 0x00, 0x66, 0xC1,
    0xEA, 0x08, 0x48, 0x8B, 0xCB, 0x80, 0xE2, 0x01, 0xE8, 0x7E,
    0x6C, 0xFF, 0xFF};
constexpr std::array<std::uint8_t, 46> kNativeTriggerMappingEvidence{
    0x44, 0x0F, 0xB6, 0x44, 0x24, 0x36, 0x45, 0x33, 0xC9, 0x08,
    0x43, 0x08, 0x33, 0xD2, 0x48, 0x8B, 0xCB, 0xE8, 0x16, 0xF8,
    0xFF, 0xFF, 0x44, 0x0F, 0xB6, 0x44, 0x24, 0x37, 0x45, 0x33,
    0xC9, 0x08, 0x43, 0x08, 0x48, 0x8B, 0xCB, 0x41, 0x8D, 0x51,
    0x01, 0xE8, 0xFE, 0xF7, 0xFF, 0xFF};
constexpr std::array<std::uint8_t, 34> kNativeL2AggregateEvidence{
    0x83, 0x7D, 0x00, 0x01, 0x74, 0x61, 0x48, 0x85, 0xC0, 0x74,
    0x5C, 0xF3, 0x0F, 0x10, 0x40, 0x48, 0x44, 0x38, 0x78, 0x6C,
    0x74, 0x0D, 0x41, 0x0F, 0x54, 0xC1, 0x41, 0x0F, 0x2F, 0xC2,
    0x0F, 0x97, 0xC0, 0xEB};
constexpr std::array<std::uint8_t, 34> kNativeR2AggregateEvidence{
    0x83, 0x7D, 0x00, 0x01, 0x74, 0x51, 0x48, 0x85, 0xC0, 0x74,
    0x4C, 0xF3, 0x0F, 0x10, 0x40, 0x4C, 0x44, 0x38, 0x78, 0x6D,
    0x74, 0x0D, 0x41, 0x0F, 0x54, 0xC1, 0x41, 0x0F, 0x2F, 0xC2,
    0x0F, 0x97, 0xC0, 0xEB};
#endif
constexpr std::array<std::uint8_t, 16> kKeyboardParserSignature{
    0x48, 0x89, 0x5C, 0x24, 0x18, 0x56, 0x48, 0x83,
    0xEC, 0x30, 0x48, 0x8B, 0xF2, 0x48, 0x8B, 0xD9};
constexpr std::array<std::uint8_t, 16> kIntSettingGetterSignature{
    0x48, 0x83, 0xEC, 0x28, 0xE8, 0x67, 0xFE, 0xFF,
    0xFF, 0x0F, 0x57, 0xC9, 0xF3, 0x0F, 0x5F, 0xC1};
constexpr std::array<std::uint8_t, 38> kSwingToggleEvidenceSignature{
    0xBA, 0x2D, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x0D, 0x28, 0x6C,
    0x5A, 0x05, 0xE8, 0x33, 0xD6, 0xF3, 0xFF, 0x48, 0x8B, 0x4B,
    0x58, 0x84, 0xC0, 0xBA, 0x2D, 0x7A, 0x0D, 0x5E, 0x41, 0xB8,
    0x04, 0xD6, 0x39, 0xC5, 0x41, 0x0F, 0x45, 0xD0};
constexpr std::array<std::uint8_t, 15> kAirborneEvidenceSignature{
    0x44, 0x38, 0xB7, 0xEE, 0x06, 0x00, 0x00, 0x0F,
    0x84, 0x88, 0x00, 0x00, 0x00, 0x85, 0xF6};
constexpr std::array<std::uint8_t, 23> kAirborneStoreEvidenceSignature{
    0xF3, 0x0F, 0x10, 0x8F, 0xB4, 0x06, 0x00, 0x00,
    0x48, 0x8B, 0x9C, 0x24, 0x88, 0x01, 0x00, 0x00,
    0x40, 0x88, 0xB7, 0xEE, 0x06, 0x00, 0x00};
constexpr std::array<std::uint8_t, 25> kSwingActionRegistrationEvidence{
    0x48, 0x8D, 0x0D, 0xC7, 0x1F, 0xD4, 0x02, 0xC7, 0x44,
    0x24, 0x28, 0xE8, 0x03, 0x00, 0x00, 0x33, 0xD2, 0xC7,
    0x44, 0x24, 0x20, 0x2A, 0x00, 0x00, 0x00};
constexpr std::array<std::uint8_t, 8> kActionBindingGetterEvidence{
    0x8B, 0x84, 0x91, 0xB4, 0x00, 0x00, 0x00, 0xC3};
constexpr std::array<std::uint8_t, 36> kNativeHandDerivationEvidence{
    0xF3, 0x0F, 0x5C, 0x4D, 0x1F, 0xF3, 0x0F, 0x5C, 0x45,
    0x17, 0xF3, 0x0F, 0x59, 0x4D, 0xEF, 0xF3, 0x0F, 0x59,
    0x45, 0xE7, 0xF3, 0x0F, 0x58, 0xC8, 0x0F, 0x2F, 0xCE,
    0x0F, 0x97, 0xC0, 0x88, 0x87, 0x81, 0x02, 0x00, 0x00};
constexpr std::array<std::uint8_t, 18> kNativeRightHandPredicateEvidence{
    0x80, 0xBB, 0x81, 0x02, 0x00, 0x00, 0x00, 0x74, 0x46,
    0x80, 0xBB, 0xF0, 0x00, 0x00, 0x00, 0x01, 0x74, 0x3D};
constexpr std::array<std::uint8_t, 27> kNativeRightHandSetEvidence{
    0x8B, 0x93, 0xFC, 0x00, 0x00, 0x00, 0x41, 0xB8, 0x01,
    0x00, 0x00, 0x00, 0x48, 0x8B, 0xCF, 0xE8, 0x0C, 0x41,
    0xBC, 0xFF, 0xC6, 0x83, 0x81, 0x02, 0x00, 0x00, 0x00};
constexpr std::array<std::uint8_t, 17> kNativeLeftHandSetEvidence{
    0x8B, 0x93, 0xFC, 0x00, 0x00, 0x00, 0x45, 0x33, 0xC0,
    0x48, 0x8B, 0xC8, 0xE8, 0x20, 0xA2, 0xBC, 0xFF};
#if !defined(TRUESWING_CONTROLS_ONLY)
constexpr std::array<std::uint8_t, 15> kSwingTransitionPayloadSizeEvidence{
    0x8B, 0x05, 0xAA, 0xEE, 0x33, 0x06, 0x89, 0x03,
    0x48, 0x8B, 0xC3, 0xC6, 0x43, 0x04, 0x70};
constexpr std::array<std::uint8_t, 33> kSwingTransitionPayloadCopyEvidence{
    0xF2, 0x0F, 0x10, 0x02, 0xF2, 0x0F, 0x11, 0x41, 0x24, 0x8B, 0x42,
    0x08, 0x89, 0x41, 0x2C, 0xF2, 0x41, 0x0F, 0x10, 0x00, 0xF2, 0x0F,
    0x11, 0x41, 0x30, 0x41, 0x8B, 0x40, 0x08, 0x89, 0x41, 0x38, 0xC3};
constexpr std::array<std::uint8_t, 48> kSwingTransitionPayloadReadEvidence{
    0xF2, 0x41, 0x0F, 0x10, 0x45, 0x24, 0xF2, 0x0F, 0x11, 0x87,
    0x5C, 0x01, 0x00, 0x00, 0x41, 0x8B, 0x45, 0x2C, 0x89, 0x87,
    0x64, 0x01, 0x00, 0x00, 0xF2, 0x41, 0x0F, 0x10, 0x45, 0x30,
    0xF2, 0x0F, 0x11, 0x87, 0x68, 0x01, 0x00, 0x00, 0x41, 0x8B,
    0x45, 0x38, 0xF2, 0x0F, 0x10, 0x87, 0x5C, 0x01};
constexpr std::array<std::uint8_t, 40> kSwingTransitionQueueCopyEvidence{
    0x0F, 0x10, 0x07, 0x48, 0x8B, 0xD0, 0x0F, 0x11, 0x00, 0x48,
    0x8D, 0x88, 0x80, 0x00, 0x00, 0x00, 0x0F, 0x10, 0x4F, 0x10,
    0x0F, 0x11, 0x48, 0x10, 0x0F, 0x10, 0x47, 0x20, 0x0F, 0x11,
    0x40, 0x20, 0x0F, 0x10, 0x4F, 0x30, 0x0F, 0x11, 0x48, 0x30};
constexpr std::array<std::uint8_t, 20> kRequestTransitionSignature{
    0x40, 0x55, 0x56, 0x57, 0x48, 0x81, 0xEC, 0x70, 0x02, 0x00,
    0x00, 0x49, 0x8B, 0xF0, 0x48, 0x8B, 0xEA, 0x48, 0x8B, 0xF9};
constexpr std::array<std::uint8_t, 25> kExecuteTransitionSignature{
    0x48, 0x89, 0x6C, 0x24, 0x18, 0x48, 0x89, 0x74,
    0x24, 0x20, 0x41, 0x56, 0x48, 0x83, 0xEC, 0x20,
    0x49, 0x8B, 0xE8, 0x4C, 0x8B, 0xF2, 0x48, 0x8B, 0xF1};
#endif

using SwingPointHunterUpdate = void(__fastcall*)(void* hunter,
                                                  float elapsedTime);
using SwingPointSubmit = bool(__fastcall*)(void* hunter, void* inputOrigin,
                                           void* outputPivot,
                                           void* outputVisibleEndpoint);
using SwingPointAggregate = void(__fastcall*)(void* hunter);
using NativeInputDispatch = bool(__fastcall*)(void* input, std::uint32_t message,
                                               std::uintptr_t wParam,
                                               std::intptr_t lParam,
                                               void* rawInputCopy);
#if defined(TRUESWING_CONTROLS_ONLY)
using NativeInputDeviceUpdate = void(__fastcall*)(void* input,
                                                   bool updateDevices);
#endif
using NativeKeyboardParser = void(__fastcall*)(void* input,
                                               const RAWKEYBOARD* keyboard);
using NativeIntSettingGetter = int(__fastcall*)(void* settings,
                                                 std::uint32_t settingId);
#if !defined(TRUESWING_CONTROLS_ONLY)
using SwingTransitionPayloadCopy = std::uint32_t(__fastcall*)(
    void* payload, const GameVec3* firstTransitionPoint,
    const GameVec3* secondTransitionPoint);
using RequestTransition = bool(__fastcall*)(void* machine,
                                            const void* stateKey,
                                            void* callerPayload);
using ExecuteTransition = bool(__fastcall*)(void* machine,
                                            const void* resolvedStateKey,
                                            const void* queuedPayload);
#endif

SwingPointHunterUpdate g_originalHunterUpdate = nullptr;
SwingPointSubmit g_originalSwingPointSubmit = nullptr;
NativeInputDispatch g_originalInputDispatch = nullptr;
#if defined(TRUESWING_CONTROLS_ONLY)
NativeInputDeviceUpdate g_originalInputDeviceUpdate = nullptr;
#endif
SwingPointAggregate g_swingPointAggregate = nullptr;
NativeKeyboardParser g_nativeKeyboardParser = nullptr;
NativeIntSettingGetter g_nativeIntSettingGetter = nullptr;
#if !defined(TRUESWING_CONTROLS_ONLY)
SwingTransitionPayloadCopy g_originalSwingTransitionPayloadCopy = nullptr;
RequestTransition g_originalRequestTransition = nullptr;
ExecuteTransition g_originalExecuteTransition = nullptr;
#endif
std::uintptr_t g_moduleBase = 0;
std::atomic_bool g_ready{false};
#if !defined(TRUESWING_CONTROLS_ONLY)
std::atomic_uint32_t g_movementKeyMask{0U};
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
#endif

struct HeroPublication final {
    std::atomic_uint64_t sequence{0};
    std::atomic_uintptr_t hunter{0};
    std::atomic_uintptr_t stateHost{0};
    std::atomic_uintptr_t manager{0};
    std::atomic_uintptr_t mover{0};
    std::atomic_uint32_t handle{0};
    std::atomic_uint16_t registryIndex{0};
    std::atomic_uint64_t observedTick{0};
};

struct HeroSnapshot final {
    std::uint64_t sequence{};
    std::uintptr_t hunter{};
    MoverBinding binding{};
    ULONGLONG observedTick{};
};

struct SideRequestPublication final {
    std::atomic_uint64_t sequence{0};
    std::atomic_uint64_t token{0};
    std::atomic_uint8_t side{0}; // 0 none, 1 left, 2 right
    std::atomic_uintptr_t hunter{0};
    std::atomic_uintptr_t stateHost{0};
    std::atomic_uintptr_t manager{0};
    std::atomic_uintptr_t mover{0};
    std::atomic_uint32_t handle{0};
    std::atomic_uint16_t registryIndex{0};
};

struct SideRequestSnapshot final {
    std::uint64_t sequence{};
    std::uint64_t token{};
    WebSide side{WebSide::Left};
    bool canceling{};
    std::uintptr_t hunter{};
    MoverBinding binding{};
};

#if !defined(TRUESWING_CONTROLS_ONLY)
struct QueuedSubmitEvidence final {
    bool occupied{};
    std::uint64_t token{};
    std::uintptr_t machine{};
    std::array<std::uint8_t, kSwingTransitionPayloadSize> payload{};
    bool canceled{};
    bool consumed{};
};

struct PendingSubmitPublication final {
    std::uint64_t token{};
    GameVec3 firstTransitionPoint{};
    GameVec3 secondTransitionPoint{};
    std::uintptr_t payload{};
    bool valid{};
};

struct QueuedSubmitMatch final {
    std::uint64_t token{};
    bool canceled{};
    bool ambiguous{};
};

struct ExecutorEnterScope final {
    const void* queuedPayload{};
    ManualWebEnterClaim claim{};
    bool valid{};
};
#endif

HeroPublication g_hero;
std::atomic_flag g_heroWriter = ATOMIC_FLAG_INIT;
SideRequestPublication g_sideRequest;
std::atomic_uint64_t g_nextSideRequestToken{0};
SRWLOCK g_sideRequestLock = SRWLOCK_INIT;
AirborneWebInputPolicy g_inputPolicy;
#if defined(TRUESWING_CONTROLS_ONLY)
ControllerWebInputPolicy g_controllerInputPolicy;
ControllerSourceRoute g_controllerSourceRoute;
#endif
bool g_syntheticShiftDown = false;
bool g_physicalShiftDown = false;
bool g_nativeBridgeShiftDown = false;
#if !defined(TRUESWING_CONTROLS_ONLY)
std::array<QueuedSubmitEvidence, kQueuedSubmitEvidenceCapacity>
    g_queuedSubmitEvidence{};
std::size_t g_nextQueuedSubmitEvidence = 0U;
std::uint64_t g_consumedSubmitToken = 0U;
thread_local PendingSubmitPublication g_pendingSubmitPublication{};
thread_local ExecutorEnterScope g_executorEnterScope{};
#endif

class ExclusiveSrwGuard final {
public:
    explicit ExclusiveSrwGuard(SRWLOCK& lock) noexcept : lock_(&lock) {
        AcquireSRWLockExclusive(lock_);
    }
    ~ExclusiveSrwGuard() { Release(); }

    ExclusiveSrwGuard(const ExclusiveSrwGuard&) = delete;
    ExclusiveSrwGuard& operator=(const ExclusiveSrwGuard&) = delete;

    void Release() noexcept {
        if (lock_ != nullptr) {
            ReleaseSRWLockExclusive(lock_);
            lock_ = nullptr;
        }
    }

private:
    SRWLOCK* lock_{};
};

template <std::size_t Size>
[[nodiscard]] bool Matches(std::uintptr_t address,
                           const std::array<std::uint8_t, Size>& expected) {
    for (std::size_t index = 0; index < expected.size(); ++index) {
        std::uint8_t actual = 0;
        if (!TryReadByte(address + index, actual) ||
            actual != expected[index]) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool TryCopyBytes(std::uintptr_t destination,
                                std::uintptr_t source,
                                std::size_t size) noexcept {
    __try {
        std::memcpy(reinterpret_cast<void*>(destination),
                    reinterpret_cast<const void*>(source), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

#if !defined(TRUESWING_CONTROLS_ONLY)
[[nodiscard]] bool TryReadSwingPayload(
    std::uintptr_t source,
    std::array<std::uint8_t, kSwingTransitionPayloadSize>& output) noexcept {
    if (source == 0U) {
        return false;
    }
    __try {
        std::memcpy(output.data(), reinterpret_cast<const void*>(source),
                    output.size());
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
#endif

[[nodiscard]] bool TryWriteByte(std::uintptr_t address,
                                std::uint8_t value) noexcept {
    __try {
        *reinterpret_cast<std::uint8_t*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

#if defined(TRUESWING_CONTROLS_ONLY)
[[nodiscard]] bool TryWriteFloat(std::uintptr_t address,
                                 float value) noexcept {
    __try {
        *reinterpret_cast<float*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
#endif

void PublishHero(std::uintptr_t hunter, const MoverBinding& binding) noexcept {
    if (g_heroWriter.test_and_set(std::memory_order_acquire)) {
        return;
    }
    g_hero.sequence.fetch_add(1U, std::memory_order_acq_rel);
    g_hero.hunter.store(hunter, std::memory_order_relaxed);
    g_hero.stateHost.store(binding.stateHost, std::memory_order_relaxed);
    g_hero.manager.store(binding.manager, std::memory_order_relaxed);
    g_hero.mover.store(binding.mover, std::memory_order_relaxed);
    g_hero.handle.store(binding.handle, std::memory_order_relaxed);
    g_hero.registryIndex.store(binding.registryIndex,
                               std::memory_order_relaxed);
    g_hero.observedTick.store(GetTickCount64(), std::memory_order_relaxed);
    g_hero.sequence.fetch_add(1U, std::memory_order_release);
    g_heroWriter.clear(std::memory_order_release);
}

[[nodiscard]] bool ReadHero(HeroSnapshot& output) noexcept {
    for (int attempt = 0; attempt < 8; ++attempt) {
        const std::uint64_t before =
            g_hero.sequence.load(std::memory_order_acquire);
        if ((before & 1U) != 0U) {
            continue;
        }
        HeroSnapshot candidate{};
        candidate.sequence = before;
        candidate.hunter = g_hero.hunter.load(std::memory_order_relaxed);
        candidate.binding.stateHost =
            g_hero.stateHost.load(std::memory_order_relaxed);
        candidate.binding.manager =
            g_hero.manager.load(std::memory_order_relaxed);
        candidate.binding.mover =
            g_hero.mover.load(std::memory_order_relaxed);
        candidate.binding.handle =
            g_hero.handle.load(std::memory_order_relaxed);
        candidate.binding.registryIndex =
            g_hero.registryIndex.load(std::memory_order_relaxed);
        candidate.observedTick =
            g_hero.observedTick.load(std::memory_order_relaxed);
        const std::uint64_t after =
            g_hero.sequence.load(std::memory_order_acquire);
        if (before == after && (after & 1U) == 0U) {
            output = candidate;
            return candidate.hunter != 0U && candidate.binding.mover != 0U;
        }
    }
    return false;
}

void PublishSideRequest(WebSide side,
                        const HeroSnapshot& hero) noexcept {
    ExclusiveSrwGuard guard(g_sideRequestLock);
    const std::uint64_t token =
        g_nextSideRequestToken.fetch_add(1U, std::memory_order_relaxed) + 1U;
    g_sideRequest.sequence.fetch_add(1U, std::memory_order_acq_rel);
    g_sideRequest.token.store(token, std::memory_order_relaxed);
    g_sideRequest.side.store(side == WebSide::Left ? 1U : 2U,
                             std::memory_order_relaxed);
    g_sideRequest.hunter.store(hero.hunter, std::memory_order_relaxed);
    g_sideRequest.stateHost.store(hero.binding.stateHost,
                                  std::memory_order_relaxed);
    g_sideRequest.manager.store(hero.binding.manager,
                                std::memory_order_relaxed);
    g_sideRequest.mover.store(hero.binding.mover, std::memory_order_relaxed);
    g_sideRequest.handle.store(hero.binding.handle, std::memory_order_relaxed);
    g_sideRequest.registryIndex.store(hero.binding.registryIndex,
                                      std::memory_order_relaxed);
    g_sideRequest.sequence.fetch_add(1U, std::memory_order_release);
}

#if !defined(TRUESWING_CONTROLS_ONLY)
void CancelQueuedSubmitEvidenceLocked(std::uint64_t token) noexcept;
#endif

void ClearSideRequest() noexcept {
    ExclusiveSrwGuard guard(g_sideRequestLock);
    const std::uint8_t side =
        g_sideRequest.side.load(std::memory_order_relaxed);
#if !defined(TRUESWING_CONTROLS_ONLY)
    if (side == 1U || side == 2U) {
        CancelQueuedSubmitEvidenceLocked(
            g_sideRequest.token.load(std::memory_order_relaxed));
    }
#else
    (void)side;
#endif
    g_sideRequest.sequence.fetch_add(1U, std::memory_order_acq_rel);
    g_sideRequest.side.store(0U, std::memory_order_relaxed);
    g_sideRequest.hunter.store(0U, std::memory_order_relaxed);
    g_sideRequest.stateHost.store(0U, std::memory_order_relaxed);
    g_sideRequest.manager.store(0U, std::memory_order_relaxed);
    g_sideRequest.mover.store(0U, std::memory_order_relaxed);
    g_sideRequest.handle.store(0U, std::memory_order_relaxed);
    g_sideRequest.registryIndex.store(0U, std::memory_order_relaxed);
    g_sideRequest.sequence.fetch_add(1U, std::memory_order_release);
}

#if !defined(TRUESWING_CONTROLS_ONLY)
[[nodiscard]] bool SameGameVecBits(const GameVec3& left,
                                   const GameVec3& right) noexcept {
    return std::memcmp(&left, &right, sizeof(GameVec3)) == 0;
}

void PublishQueuedSubmitEvidenceLocked(
    std::uint64_t token, std::uintptr_t machine,
    const std::array<std::uint8_t, kSwingTransitionPayloadSize>& payload,
    bool canceled) noexcept {
    if (token == 0U || machine == 0U || token <= g_consumedSubmitToken) {
        return;
    }
    g_queuedSubmitEvidence[g_nextQueuedSubmitEvidence] = QueuedSubmitEvidence{
        true, token, machine, payload, canceled, false};
    g_nextQueuedSubmitEvidence =
        (g_nextQueuedSubmitEvidence + 1U) % kQueuedSubmitEvidenceCapacity;
}

void CancelQueuedSubmitEvidenceLocked(std::uint64_t token) noexcept {
    if (token == 0U) {
        return;
    }
    for (QueuedSubmitEvidence& evidence : g_queuedSubmitEvidence) {
        if (evidence.occupied && evidence.token == token &&
            !evidence.consumed) {
            evidence.canceled = true;
        }
    }
}

[[nodiscard]] std::optional<QueuedSubmitMatch>
ConsumeQueuedSubmitEvidenceLocked(std::uintptr_t machine,
                                  const void* payload) noexcept {
    std::array<std::uint8_t, kSwingTransitionPayloadSize> bytes{};
    if (machine == 0U ||
        !TryReadSwingPayload(reinterpret_cast<std::uintptr_t>(payload),
                             bytes)) {
        return std::nullopt;
    }

    QueuedSubmitEvidence* match = nullptr;
    bool ambiguous = false;
    for (QueuedSubmitEvidence& candidate : g_queuedSubmitEvidence) {
        if (!candidate.occupied || candidate.token == 0U ||
            candidate.consumed || candidate.machine != machine ||
            candidate.payload != bytes) {
            continue;
        }
        if (match == nullptr) {
            match = &candidate;
        } else if (candidate.token != match->token) {
            ambiguous = true;
        }
    }
    if (match == nullptr) {
        return std::nullopt;
    }

    const std::uint64_t matchedToken = match->token;
    bool canceled = match->canceled;
    std::uint64_t greatestConsumedToken = matchedToken;
    for (QueuedSubmitEvidence& evidence : g_queuedSubmitEvidence) {
        if (!evidence.occupied || evidence.consumed ||
            evidence.machine != machine || evidence.payload != bytes) {
            continue;
        }
        if (ambiguous || evidence.token == matchedToken) {
            evidence.consumed = true;
            canceled = canceled || evidence.canceled;
            greatestConsumedToken =
                std::max(greatestConsumedToken, evidence.token);
        }
    }
    g_consumedSubmitToken =
        std::max(g_consumedSubmitToken, greatestConsumedToken);
    return QueuedSubmitMatch{ambiguous ? 0U : matchedToken,
                             canceled || ambiguous, ambiguous};
}
#endif

void MarkSideRequestCancelingLocked() noexcept {
    const std::uint8_t side =
        g_sideRequest.side.load(std::memory_order_relaxed);
    if (side != 1U && side != 2U) {
        return;
    }
#if !defined(TRUESWING_CONTROLS_ONLY)
    CancelQueuedSubmitEvidenceLocked(
        g_sideRequest.token.load(std::memory_order_relaxed));
#endif

    // Submit holds this same lock through its native call. Reaching this store
    // therefore proves that any already-authorized Submit finished first.
    // New submissions see the canceling state and fail closed while the native
    // Swing-up edge is delivered outside the non-recursive SRW lock.
    g_sideRequest.sequence.fetch_add(1U, std::memory_order_acq_rel);
    g_sideRequest.side.store(3U, std::memory_order_relaxed);
    g_sideRequest.sequence.fetch_add(1U, std::memory_order_release);
}

void BeginSideRequestCancellation() noexcept {
    ExclusiveSrwGuard guard(g_sideRequestLock);
    MarkSideRequestCancelingLocked();
}

[[nodiscard]] bool ReadSideRequest(SideRequestSnapshot& output) noexcept {
    for (int attempt = 0; attempt < 8; ++attempt) {
        const std::uint64_t before =
            g_sideRequest.sequence.load(std::memory_order_acquire);
        if ((before & 1U) != 0U) {
            continue;
        }
        SideRequestSnapshot candidate{};
        candidate.sequence = before;
        candidate.token =
            g_sideRequest.token.load(std::memory_order_relaxed);
        const std::uint8_t side =
            g_sideRequest.side.load(std::memory_order_relaxed);
        candidate.side = side == 2U ? WebSide::Right : WebSide::Left;
        candidate.canceling = side == 3U;
        candidate.hunter =
            g_sideRequest.hunter.load(std::memory_order_relaxed);
        candidate.binding.stateHost =
            g_sideRequest.stateHost.load(std::memory_order_relaxed);
        candidate.binding.manager =
            g_sideRequest.manager.load(std::memory_order_relaxed);
        candidate.binding.mover =
            g_sideRequest.mover.load(std::memory_order_relaxed);
        candidate.binding.handle =
            g_sideRequest.handle.load(std::memory_order_relaxed);
        candidate.binding.registryIndex =
            g_sideRequest.registryIndex.load(std::memory_order_relaxed);
        const std::uint64_t after =
            g_sideRequest.sequence.load(std::memory_order_acquire);
        if (before == after && (after & 1U) == 0U) {
            output = candidate;
            return (side == 1U || side == 2U || side == 3U) &&
                   candidate.token != 0U &&
                   candidate.hunter != 0U && candidate.binding.mover != 0U;
        }
    }
    return false;
}

[[nodiscard]] bool SameBindingIdentity(const MoverBinding& left,
                                       const MoverBinding& right) noexcept {
    return left.stateHost == right.stateHost && left.manager == right.manager &&
           left.mover == right.mover && left.handle == right.handle &&
           left.registryIndex == right.registryIndex;
}

[[nodiscard]] bool IsFresh(const HeroSnapshot& snapshot) noexcept {
    const ULONGLONG now = GetTickCount64();
    return snapshot.observedTick != 0ULL && now >= snapshot.observedTick &&
           now - snapshot.observedTick <= kHeroPublicationMaximumAgeMs;
}

[[nodiscard]] bool RequestMatchesCurrentAirborneHero(
    const SideRequestSnapshot& request) noexcept {
    HeroSnapshot hero{};
    std::uint8_t airborne = 0;
    return !request.canceling && ReadHero(hero) && IsFresh(hero) &&
           hero.hunter == request.hunter &&
           SameBindingIdentity(hero.binding, request.binding) &&
           RevalidateHeroMover(request.binding, g_moduleBase) &&
           TryReadByte(request.binding.mover + kMoverAirborneOffset,
                       airborne) &&
           airborne != 0U;
}

[[nodiscard]] bool ExactVtable(std::uintptr_t object,
                               std::uintptr_t expectedRva) noexcept {
    std::uintptr_t vtable = 0;
    return object != 0U && TryReadPointer(object, vtable) &&
           vtable == g_moduleBase + expectedRva;
}

#if defined(TRUESWING_CONTROLS_ONLY)
struct NativeGamepadSample final {
    std::uintptr_t device{};
    bool connected{};
    bool leftShoulderHeld{};
    float leftTrigger{};
    float rightTrigger{};
};

[[nodiscard]] bool MouseOwnsNativeHold() noexcept {
    return g_inputPolicy.LeftOwnsNativeHold() ||
           g_inputPolicy.RightOwnsNativeHold();
}

[[nodiscard]] bool KnownNativeGamepadVtable(
    std::uintptr_t vtable) noexcept {
    return vtable == g_moduleBase + kWindowsGamingInputGamepadVtableRva ||
           vtable == g_moduleBase + kSceGamepadVtableRva ||
           vtable == g_moduleBase + kSteamInputGamepadVtableRva ||
           vtable == g_moduleBase + kXInputGamepadVtableRva;
}

[[nodiscard]] bool TryReadNativeGamepad(
    std::uintptr_t device, NativeGamepadSample& output) noexcept {
    std::uintptr_t vtable = 0U;
    std::uint32_t type = 0U;
    std::uint32_t leftShoulder = 0U;
    std::uint8_t connected = 0U;
    float leftTrigger = 0.0F;
    float rightTrigger = 0.0F;
    if (device == 0U || !TryReadPointer(device, vtable) ||
        !KnownNativeGamepadVtable(vtable) ||
        !TryReadU32(device + kGamepadTypeOffset, type) ||
        type > kLastNativeGamepadType ||
        !TryReadByte(device + kGamepadConnectedOffset, connected) ||
        !TryReadU32(device + kGamepadLeftShoulderStateOffset,
                    leftShoulder) ||
        !TryReadFloat(device + kGamepadLeftTriggerOffset, leftTrigger) ||
        !TryReadFloat(device + kGamepadRightTriggerOffset, rightTrigger)) {
        return false;
    }
    output = NativeGamepadSample{
        device, connected != 0U,
        (leftShoulder & kNativeButtonHeldMask) != 0U, leftTrigger,
        rightTrigger};
    return true;
}

[[nodiscard]] bool TryReadInputDevice(std::uintptr_t input,
                                      std::uint32_t index,
                                      std::uintptr_t& device) noexcept {
    return index < kInputDeviceCapacity &&
           TryReadPointer(input + kInputDeviceArrayOffset +
                              static_cast<std::uintptr_t>(index) *
                                  sizeof(std::uintptr_t),
                          device);
}

[[nodiscard]] bool TrySelectNativeGamepad(
    std::uintptr_t input, NativeGamepadSample& output) noexcept {
    std::uint32_t primaryIndex = std::numeric_limits<std::uint32_t>::max();
    std::uintptr_t primaryDevice = 0U;
    NativeGamepadSample primary{};
    if (TryReadU32(input + kPrimaryPlayerDeviceIndexOffset, primaryIndex) &&
        TryReadInputDevice(input, primaryIndex, primaryDevice) &&
        TryReadNativeGamepad(primaryDevice, primary)) {
        output = primary;
        return true;
    }
    // Logical player 0 is authoritative. A missing, reassigned, or unknown
    // main-pad mapping is a source boundary; never borrow another device.
    return false;
}
#endif

[[nodiscard]] bool ForegroundProcess() noexcept {
    const HWND window = GetForegroundWindow();
    if (window == nullptr) {
        return false;
    }
    DWORD processId = 0;
    (void)GetWindowThreadProcessId(window, &processId);
    return processId == GetCurrentProcessId();
}

[[nodiscard]] bool TryNativeShiftState(std::uintptr_t input,
                                       bool& down) noexcept {
    std::uint32_t state = 0;
    if (!TryReadU32(input + kKeyboardStateBaseOffset +
                        kLeftShiftScanCode * sizeof(std::uint32_t),
                    state)) {
        return false;
    }
    down = (state & 4U) != 0U;
    return true;
}

[[nodiscard]] bool SwingBindingIsExclusiveLeftShift() noexcept {
    std::uintptr_t count = 0;
    const std::uintptr_t record =
        g_moduleBase + kActionRecordBaseRva +
        kSwingActionRecordIndex * kActionRecordStride;
    std::uintptr_t action = 0;
    std::uintptr_t name = 0;
    std::uint32_t defaultPrimary = 0;
    std::uint32_t defaultSecondary = 0;
    std::uint32_t group = 0;
    std::uint32_t livePrimary = 0;
    std::uint32_t liveSecondary = 0;
    if (!TryReadPointer(g_moduleBase + kActionRecordCountRva, count) ||
        count <= kSwingActionRecordIndex || count > 512U ||
        !TryReadPointer(record, action) || action == 0U ||
        !TryReadU32(record + 8U, defaultPrimary) ||
        defaultPrimary != kLeftShiftScanCode ||
        !TryReadU32(record + 0xCU, defaultSecondary) ||
        defaultSecondary != 1000U || !TryReadU32(record + 0x14U, group) ||
        group != 0U || !TryReadPointer(record + 0x18U, name) ||
        name != g_moduleBase + kSwingActionNameRva ||
        !TryReadU32(action + kActionKeyboardBindingBaseOffset, livePrimary) ||
        !TryReadU32(action + kActionKeyboardBindingBaseOffset +
                        sizeof(std::uint32_t),
                    liveSecondary) ||
        (livePrimary != kLeftShiftScanCode &&
         liveSecondary != kLeftShiftScanCode)) {
        return false;
    }

    // Injecting the game's native Shift edge is safe only while Swing is the
    // sole live action using that scan code. Otherwise one mouse click could
    // activate an unrelated rebound action as well.
    for (std::size_t index = 0; index < count; ++index) {
        if (index == kSwingActionRecordIndex) {
            continue;
        }
        const std::uintptr_t otherRecord =
            g_moduleBase + kActionRecordBaseRva +
            index * kActionRecordStride;
        std::uintptr_t otherAction = 0;
        std::uint32_t otherPrimary = 0;
        std::uint32_t otherSecondary = 0;
        if (!TryReadPointer(otherRecord, otherAction) || otherAction == 0U ||
            !TryReadU32(otherAction + kActionKeyboardBindingBaseOffset,
                        otherPrimary) ||
            !TryReadU32(otherAction + kActionKeyboardBindingBaseOffset +
                            sizeof(std::uint32_t),
                        otherSecondary)) {
            return false;
        }
        if (otherPrimary == kLeftShiftScanCode ||
            otherSecondary == kLeftShiftScanCode) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool SwingUsesHold() noexcept {
    if (g_nativeIntSettingGetter == nullptr || g_moduleBase == 0U) {
        return false;
    }
    int toggle = 1;
    __try {
        toggle = g_nativeIntSettingGetter(
            reinterpret_cast<void*>(g_moduleBase + kAccessibilitySettingsRva),
            kSwingUseToggleSettingId);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    return toggle == 0;
}

[[nodiscard]] WebInputEligibility EligibilityForDown(
    std::uintptr_t input, bool foreground,
    HeroSnapshot* provenHero) noexcept {
    WebInputEligibility result{};
    result.foreground = foreground;
    std::uint8_t mouseInputEnabled = 0;
    std::uint8_t handMirror = 1;
    bool nativeShiftDown = false;
#if defined(TRUESWING_CONTROLS_ONLY)
    const bool controllerBridgeAvailable =
        !g_controllerInputPolicy.NativeSwingHeld();
#else
    constexpr bool controllerBridgeAvailable = true;
#endif
    result.runtimeReady = g_ready.load(std::memory_order_acquire) &&
                          ExactVtable(input, kNativeInputVtableRva) &&
                          GetSystemMetrics(SM_SWAPBUTTON) == 0 &&
                          TryReadByte(input + kMouseInputEnabledOffset,
                                      mouseInputEnabled) &&
                          mouseInputEnabled != 0U &&
                          TryReadByte(g_moduleBase + kNativeHandMirrorFlagRva,
                                      handMirror) &&
                          handMirror == 0U &&
                          TryNativeShiftState(input, nativeShiftDown) &&
                           SwingBindingIsExclusiveLeftShift() &&
                           SwingUsesHold() &&
                           controllerBridgeAvailable &&
                           (g_syntheticShiftDown || !nativeShiftDown);
    if (!result.foreground || !result.runtimeReady) {
        return result;
    }

    HeroSnapshot snapshot{};
    std::uint8_t airborne = 0;
    if (!ReadHero(snapshot) || !IsFresh(snapshot) ||
        !RevalidateHeroMover(snapshot.binding, g_moduleBase) ||
        !TryReadByte(snapshot.binding.mover + kMoverAirborneOffset,
                     airborne) ||
        g_hero.sequence.load(std::memory_order_acquire) != snapshot.sequence) {
        return result;
    }
    result.airborneProven = airborne != 0U;
    if (result.airborneProven && provenHero != nullptr) {
        *provenHero = snapshot;
    }
    return result;
}

#if defined(TRUESWING_CONTROLS_ONLY)
[[nodiscard]] WebInputEligibility ControllerEligibilityForDown(
    std::uintptr_t input, bool foreground,
    HeroSnapshot* provenHero) noexcept {
    WebInputEligibility result{};
    result.foreground = foreground;
    std::uint8_t handMirror = 1U;
    bool nativeShiftDown = false;
    const bool controllerOwns =
        g_controllerInputPolicy.NativeSwingHeld();
    const bool bridgeConfigurationReady =
        controllerOwns ||
        (SwingBindingIsExclusiveLeftShift() && SwingUsesHold());
    result.runtimeReady =
        g_ready.load(std::memory_order_acquire) &&
        ExactVtable(input, kNativeInputVtableRva) &&
        TryReadByte(g_moduleBase + kNativeHandMirrorFlagRva, handMirror) &&
        handMirror == 0U && TryNativeShiftState(input, nativeShiftDown) &&
        bridgeConfigurationReady && !MouseOwnsNativeHold() &&
        (controllerOwns
             ? (g_syntheticShiftDown && g_nativeBridgeShiftDown &&
                nativeShiftDown)
             : (!g_syntheticShiftDown && !g_nativeBridgeShiftDown &&
                !nativeShiftDown));
    if (!result.foreground || !result.runtimeReady) {
        return result;
    }

    HeroSnapshot snapshot{};
    std::uint8_t airborne = 0U;
    if (!ReadHero(snapshot) || !IsFresh(snapshot) ||
        !RevalidateHeroMover(snapshot.binding, g_moduleBase) ||
        !TryReadByte(snapshot.binding.mover + kMoverAirborneOffset,
                     airborne) ||
        g_hero.sequence.load(std::memory_order_acquire) != snapshot.sequence) {
        return result;
    }
    result.airborneProven = airborne != 0U;
    if (result.airborneProven && provenHero != nullptr) {
        *provenHero = snapshot;
    }
    return result;
}
#endif

void InjectNativeShift(std::uintptr_t input, bool down) noexcept {
    if (g_nativeKeyboardParser == nullptr) {
        return;
    }
    RAWKEYBOARD keyboard{};
    keyboard.MakeCode = static_cast<USHORT>(kLeftShiftScanCode);
    keyboard.Flags = down ? 0U : RI_KEY_BREAK;
    keyboard.VKey = VK_SHIFT;
    keyboard.Message = down ? WM_KEYDOWN : WM_KEYUP;
    g_nativeKeyboardParser(reinterpret_cast<void*>(input), &keyboard);
    g_nativeBridgeShiftDown = down;
}

[[nodiscard]] bool ApplyDecision(
    std::uintptr_t input, const WebInputDecision& decision,
    const HeroSnapshot* provenHero = nullptr) noexcept {
    if (decision.leftAttach) {
        if (provenHero == nullptr) {
            return false;
        }
        PublishSideRequest(WebSide::Left, *provenHero);
    } else if (decision.rightAttach) {
        if (provenHero == nullptr) {
            return false;
        }
        PublishSideRequest(WebSide::Right, *provenHero);
    }
    if (decision.nativeSwingPress) {
        // Eligibility just proved the native key state is up. Start this
        // ownership epoch from that authoritative state rather than any stale
        // cached physical observation.
        g_physicalShiftDown = false;
        InjectNativeShift(input, true);
        g_syntheticShiftDown = true;
    }
    const bool releasesSide = decision.leftRelease || decision.rightRelease;
    if (releasesSide) {
        BeginSideRequestCancellation();
    }
    if (decision.nativeSwingRelease) {
        if (!g_physicalShiftDown) {
            InjectNativeShift(input, false);
        }
        g_syntheticShiftDown = false;
    }
    if (releasesSide) {
        // State 3 blocks Submit during a reentrant native up. The canceled
        // queued-payload evidence rejects any already-returned transition
        // after the request itself is cleared.
        ClearSideRequest();
    }
    return true;
}

#if defined(TRUESWING_CONTROLS_ONLY)
[[nodiscard]] bool ApplyControllerDecision(
    std::uintptr_t input, const ControllerWebInputDecision& decision,
    const HeroSnapshot* provenHero = nullptr) noexcept {
    // The controller policy quarantines same-sample side transfers. Reject a
    // broken decision before it can restart native Shift or lose a physical
    // Shift handoff.
    if (decision.nativeSwingPress && decision.nativeSwingRelease) {
        return false;
    }
    const WebInputDecision release{
        .leftRelease = decision.leftRelease,
        .rightRelease = decision.rightRelease,
        .nativeSwingRelease = decision.nativeSwingRelease,
    };
    if ((release.leftRelease || release.rightRelease ||
         release.nativeSwingRelease) &&
        !ApplyDecision(input, release)) {
        return false;
    }
    const WebInputDecision press{
        .leftAttach = decision.leftAttach,
        .rightAttach = decision.rightAttach,
        .nativeSwingPress = decision.nativeSwingPress,
    };
    if (press.leftAttach || press.rightAttach || press.nativeSwingPress) {
        return ApplyDecision(input, press, provenHero);
    }
    return true;
}

void CancelControllerForFocusLoss(std::uintptr_t input) noexcept {
    ControllerWebInputDecision decision{};
    if (g_controllerSourceRoute.CurrentDevice().has_value()) {
        ControllerWebInputSample sample{};
        sample.sourceEpoch = g_controllerSourceRoute.SourceEpoch();
        sample.connected = true;
        sample.focused = false;
        decision = g_controllerInputPolicy.Update(sample);
    } else {
        decision = g_controllerInputPolicy.CancelAndReset();
    }
    (void)ApplyControllerDecision(input, decision);
}
#endif

void CancelManualHold(std::uintptr_t input) noexcept {
#if defined(TRUESWING_CONTROLS_ONLY)
    CancelControllerForFocusLoss(input);
#endif
    const WebInputDecision mouseCancel =
        g_inputPolicy.CancelNativeHold();
    (void)ApplyDecision(input, mouseCancel);
    g_inputPolicy.Reset();
    BeginSideRequestCancellation();
    // A focus boundary invalidates our physical-key observation. Force the
    // native action up even if the last physical Shift edge was a down.
    if (g_nativeBridgeShiftDown) {
        InjectNativeShift(input, false);
    }
    g_syntheticShiftDown = false;
    g_physicalShiftDown = false;
    ClearSideRequest();
}

[[nodiscard]] bool IsLeftShift(const RAWKEYBOARD& keyboard) noexcept {
    return (keyboard.MakeCode & 0x7FU) == kLeftShiftScanCode &&
           (keyboard.Flags & RI_KEY_E0) == 0U;
}

#if !defined(TRUESWING_CONTROLS_ONLY)
[[nodiscard]] bool ObserveSwingMovementKey(
    const RAWKEYBOARD& keyboard) noexcept {
    const bool extended =
        (keyboard.Flags & (RI_KEY_E0 | RI_KEY_E1)) != 0U;
    const std::uint32_t bit = SwingMovementKeyBit(keyboard.MakeCode, extended);
    if (bit == 0U) {
        return false;
    }

    const bool down = (keyboard.Flags & RI_KEY_BREAK) == 0U;
    if (down) {
        g_movementKeyMask.fetch_or(bit, std::memory_order_release);
    } else {
        g_movementKeyMask.fetch_and(~bit, std::memory_order_release);
    }
    return true;
}
#endif

void ObserveHunter(void* hunterObject) noexcept {
    const std::uintptr_t hunter =
        reinterpret_cast<std::uintptr_t>(hunterObject);
    if (!g_ready.load(std::memory_order_acquire) ||
        !ExactVtable(hunter, kSwingPointHunterVtableRva)) {
        return;
    }
    std::uintptr_t stateHost = 0;
    if (!TryReadPointer(hunter + kHunterStateHostOffset, stateHost) ||
        stateHost == 0U) {
        return;
    }

    // The registry walk is only needed for first discovery or a generation /
    // checkpoint change. Normal frames revalidate the already-proven binding
    // in constant time before refreshing its publication lease.
    HeroSnapshot current{};
    if (ReadHero(current) && current.hunter == hunter &&
        current.binding.stateHost == stateHost &&
        RevalidateHeroMover(current.binding, g_moduleBase)) {
        PublishHero(hunter, current.binding);
        return;
    }
    const auto binding = ResolveHeroMover(stateHost, g_moduleBase);
    if (!binding.has_value()) {
        return;
    }
    PublishHero(hunter, *binding);
}

void __fastcall HookHunterUpdate(void* hunter, float elapsedTime) {
    g_originalHunterUpdate(hunter, elapsedTime);
    ObserveHunter(hunter);
}

[[nodiscard]] bool ApplyRequestedSide(std::uintptr_t hunter,
                                      WebSide side) noexcept {
    std::uint8_t pending = 0;
    if (!TryReadByte(hunter + kHunterCandidatesPendingOffset, pending)) {
        return false;
    }
    if (pending != 0U) {
        g_swingPointAggregate(reinterpret_cast<void*>(hunter));
    }

    std::uint32_t count = 0;
    GameVec3 originGame{};
    GameVec3 actorRightGame{};
    if (!TryReadU32(hunter + kHunterCandidateCountOffset, count) ||
        count > kHunterMaximumCandidates ||
        !TryReadGameVec3(hunter + kHunterOriginOffset, originGame) ||
        !TryReadGameVec3(hunter + kHunterActorRightOffset, actorRightGame) ||
        !IsFinite(originGame) || !IsFinite(actorRightGame)) {
        return false;
    }
    // HeroStateSwing derives the native left/right hand from the horizontal
    // actor-transform row copied into Hunter+0xE880. Runtime validation proves
    // its positive native branch presents the character's left side, while the
    // selector's canonical basis is positive-right. Negate the horizontal row
    // once here so LMB remains semantic left and RMB semantic right. Use that
    // same basis for pivot and hand-point gating. Vertical contribution is
    // explicitly absent in the native dot.
    actorRightGame.x = -actorRightGame.x;
    actorRightGame.y = 0.0F;
    actorRightGame.z = -actorRightGame.z;

    std::array<NativeAnchorCandidate, kHunterMaximumCandidates> candidates{};
    for (std::uint32_t index = 0; index < count; ++index) {
        const std::uintptr_t record =
            hunter + kHunterCandidateBankOffset +
            static_cast<std::uintptr_t>(index) * kHunterCandidateStride;
        GameVec3 pivot{};
        GameVec3 effectiveHandPoint{};
        float score = 0.0F;
        std::uint8_t valid = 0;
        std::uint8_t usesAlternateHandPoint = 0;
        std::uint8_t rejected = 0;
        if (!TryReadGameVec3(record + kCandidatePivotOffset, pivot) ||
            !TryReadFloat(record + kCandidateScoreOffset, score) ||
            !TryReadByte(record + kCandidateValidOffset, valid) ||
            !TryReadByte(record + kCandidateUsesAlternateHandPointOffset,
                         usesAlternateHandPoint) ||
            !TryReadByte(record + kCandidateRejectedOffset, rejected)) {
            return false;
        }
        effectiveHandPoint = pivot;
        if (usesAlternateHandPoint != 0U &&
            !TryReadGameVec3(record + kCandidateAlternateHandPointOffset,
                             effectiveHandPoint)) {
            return false;
        }
        candidates[index] = NativeAnchorCandidate{
            ToPhysics(pivot), ToPhysics(effectiveHandPoint),
            static_cast<double>(score), index, valid != 0U, rejected != 0U};
    }

    const auto selected = TrySelectNativeSideAnchor(
        std::span<const NativeAnchorCandidate>(candidates.data(), count),
        ToPhysics(originGame), ToPhysics(actorRightGame), side);
    if (!selected.has_value()) {
        return TryWriteByte(hunter + kHunterSelectedValidOffset, 0U);
    }

    const std::uintptr_t source =
        hunter + kHunterCandidateBankOffset +
        selected->nativeIndex * kHunterCandidateStride;
    return TryCopyBytes(hunter + kHunterSelectedRecordOffset, source,
                        kHunterCandidateStride) &&
           TryWriteByte(hunter + kHunterSelectedValidOffset, 1U);
}

bool __fastcall HookSwingPointSubmit(void* hunterObject, void* inputOrigin,
                                     void* outputPivot,
                                     void* outputVisibleEndpoint) {
#if !defined(TRUESWING_CONTROLS_ONLY)
    // Submit and the exact HeroSwing payload copy are synchronous on each
    // verified caller. Reset the thread-local bridge before every attempt so a
    // successful Submit that later fails a caller-side gate cannot authorize a
    // different transition.
    g_pendingSubmitPublication = {};
#endif
    const std::uintptr_t hunter =
        reinterpret_cast<std::uintptr_t>(hunterObject);
    SideRequestSnapshot request{};
    if (!g_ready.load(std::memory_order_acquire)) {
        return g_originalSwingPointSubmit(hunterObject, inputOrigin,
                                          outputPivot,
                                          outputVisibleEndpoint);
    }

    // Serialize the pending request with mouse release/replacement through the
    // complete native Submit. This closes the check/use window even if input
    // dispatch and traversal submission run on different threads.
    ExclusiveSrwGuard requestGuard(g_sideRequestLock);
    if (!ReadSideRequest(request)) {
        return g_originalSwingPointSubmit(hunterObject, inputOrigin,
                                          outputPivot,
                                          outputVisibleEndpoint);
    }

    if (request.canceling) {
        if (ExactVtable(hunter, kSwingPointHunterVtableRva)) {
            (void)TryWriteByte(hunter + kHunterSelectedValidOffset, 0U);
        }
        return false;
    }

    // A captured mouse request must never silently attach a stock/opposite
    // anchor after a checkpoint, identity change, or failed side proof.
    std::uint8_t handMirror = 1;
    if (!ExactVtable(hunter, kSwingPointHunterVtableRva) ||
        hunter != request.hunter ||
        !RequestMatchesCurrentAirborneHero(request) ||
        !TryReadByte(g_moduleBase + kNativeHandMirrorFlagRva, handMirror) ||
        handMirror != 0U ||
        !ApplyRequestedSide(hunter, request.side)) {
        if (ExactVtable(hunter, kSwingPointHunterVtableRva)) {
            (void)TryWriteByte(hunter + kHunterSelectedValidOffset, 0U);
        }
        return false;
    }

    const bool submitted = g_originalSwingPointSubmit(
        hunterObject, inputOrigin, outputPivot, outputVisibleEndpoint);
#if defined(TRUESWING_CONTROLS_ONLY)
    return submitted;
#else
    if (submitted) {
        GameVec3 firstTransitionPoint{};
        GameVec3 secondTransitionPoint{};
        if (!TryReadGameVec3(reinterpret_cast<std::uintptr_t>(inputOrigin),
                             firstTransitionPoint) ||
            !TryReadGameVec3(reinterpret_cast<std::uintptr_t>(outputPivot),
                             secondTransitionPoint) ||
            !IsFinite(firstTransitionPoint) ||
            !IsFinite(secondTransitionPoint)) {
            (void)TryWriteByte(hunter + kHunterSelectedValidOffset, 0U);
            return false;
        }
        // Stage only. The exact payload-copy and RequestTransition hooks below
        // must prove this Submit became the decorated queued Swing request
        // before any evidence can authorize custom movement at Enter.
        g_pendingSubmitPublication = PendingSubmitPublication{
            request.token, firstTransitionPoint, secondTransitionPoint, 0U,
            true};
    }
    return submitted;
#endif
}

#if !defined(TRUESWING_CONTROLS_ONLY)
std::uint32_t __fastcall HookSwingTransitionPayloadCopy(
    void* payload, const GameVec3* firstTransitionPoint,
    const GameVec3* secondTransitionPoint) {
    // The two verified callers semantically ignore RAX, but the exact native
    // leaf leaves the second vector's final 32 bits there. Preserve that
    // incidental ABI value instead of allowing this post-hook work to clobber
    // it.
    const std::uint32_t nativeResult = g_originalSwingTransitionPayloadCopy(
        payload, firstTransitionPoint, secondTransitionPoint);
    PendingSubmitPublication& pending = g_pendingSubmitPublication;
    if (!pending.valid || firstTransitionPoint == nullptr ||
        secondTransitionPoint == nullptr || payload == nullptr) {
        pending = {};
        return nativeResult;
    }

    GameVec3 first{};
    GameVec3 second{};
    if (!TryReadGameVec3(
            reinterpret_cast<std::uintptr_t>(firstTransitionPoint), first) ||
        !TryReadGameVec3(
            reinterpret_cast<std::uintptr_t>(secondTransitionPoint), second) ||
        !SameGameVecBits(first, pending.firstTransitionPoint) ||
        !SameGameVecBits(second, pending.secondTransitionPoint)) {
        pending = {};
        return nativeResult;
    }

    GameVec3 copiedFirst{};
    GameVec3 copiedSecond{};
    const std::uintptr_t payloadAddress =
        reinterpret_cast<std::uintptr_t>(payload);
    if (!TryReadGameVec3(payloadAddress + kSwingTransitionFirstPointOffset,
                         copiedFirst) ||
        !TryReadGameVec3(payloadAddress + kSwingTransitionSecondPointOffset,
                         copiedSecond) ||
        !SameGameVecBits(copiedFirst, pending.firstTransitionPoint) ||
        !SameGameVecBits(copiedSecond, pending.secondTransitionPoint)) {
        pending = {};
        return nativeResult;
    }
    pending.payload = payloadAddress;
    return nativeResult;
}

bool __fastcall HookRequestTransition(void* machine, const void* stateKey,
                                      void* callerPayload) {
    const std::uintptr_t returnAddress =
        reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const bool verifiedCaller =
        returnAddress == g_moduleBase + kPrimarySwingRequestReturnRva ||
        returnAddress == g_moduleBase + kAlternateSwingRequestReturnRva;
    const bool staged =
        g_pendingSubmitPublication.valid &&
        g_pendingSubmitPublication.payload != 0U &&
        verifiedCaller &&
        reinterpret_cast<std::uintptr_t>(stateKey) ==
            g_moduleBase + kSwingStateKeyRva &&
        reinterpret_cast<std::uintptr_t>(callerPayload) ==
            g_pendingSubmitPublication.payload;
    const PendingSubmitPublication pending =
        staged ? g_pendingSubmitPublication : PendingSubmitPublication{};
    if (staged) {
        // One exact request call may consume one staged Submit publication.
        g_pendingSubmitPublication = {};
    }

    struct QueueBefore final {
        std::uintptr_t base{};
        std::uint32_t stride{};
        std::uint32_t capacity{};
        std::uint32_t writeIndex{};
        std::uint8_t sequence{};
        bool valid{};
    } before;
    const std::uintptr_t machineAddress =
        reinterpret_cast<std::uintptr_t>(machine);
    const std::uintptr_t payloadAddress =
        reinterpret_cast<std::uintptr_t>(callerPayload);
    if (staged && machineAddress != 0U && payloadAddress != 0U) {
        std::uint32_t type = 0;
        std::uint8_t size = 0;
        std::uint8_t specialRoute = 1;
        GameVec3 first{};
        GameVec3 second{};
        before.valid =
            TryReadU32(payloadAddress, type) &&
            TryReadByte(payloadAddress + 4U, size) &&
            type == kSwingTransitionType &&
            size == kSwingTransitionPayloadSize &&
            TryReadGameVec3(
                payloadAddress + kSwingTransitionFirstPointOffset, first) &&
            TryReadGameVec3(
                payloadAddress + kSwingTransitionSecondPointOffset, second) &&
            SameGameVecBits(first, pending.firstTransitionPoint) &&
            SameGameVecBits(second, pending.secondTransitionPoint) &&
            TryReadByte(machineAddress + 0xE00U, specialRoute) &&
            specialRoute == 0U &&
            TryReadPointer(machineAddress + 0x98U, before.base) &&
            TryReadU32(machineAddress + 0xA0U, before.stride) &&
            TryReadU32(machineAddress + 0xA4U, before.capacity) &&
            TryReadU32(machineAddress + 0xACU, before.writeIndex) &&
            TryReadByte(machineAddress + 0xDDFU, before.sequence) &&
            before.base != 0U && before.stride >= 0x104U &&
            before.stride <= 0x10000U && before.capacity > 0U &&
            before.capacity <= 4096U &&
            before.writeIndex < before.capacity;
    }

    const bool nativeResult =
        g_originalRequestTransition(machine, stateKey, callerPayload);
    if (!staged || !before.valid || !nativeResult) {
        return nativeResult;
    }

    std::uint32_t type = 0;
    std::uint32_t decoratedType = 0;
    std::uint32_t keyType = 0;
    std::uint8_t size = 0;
    std::uint8_t sequence = 0;
    std::uint8_t specialRoute = 1;
    std::uintptr_t ringBase = 0;
    std::uint32_t stride = 0;
    std::uint32_t capacity = 0;
    std::uint32_t writeIndex = 0;
    GameVec3 first{};
    GameVec3 second{};
    std::array<std::uint8_t, kSwingTransitionPayloadSize> payloadBytes{};
    const std::uint32_t expectedWriteIndex =
        before.writeIndex + 1U == before.capacity
            ? 0U
            : before.writeIndex + 1U;
    const std::uint8_t expectedSequence =
        static_cast<std::uint8_t>(before.sequence + 1U);
    if (!TryReadU32(payloadAddress, type) ||
        !TryReadByte(payloadAddress + 4U, size) ||
        !TryReadU32(payloadAddress + 0x1DU, decoratedType) ||
        !TryReadByte(payloadAddress + 0x21U, sequence) ||
        !TryReadU32(g_moduleBase + kSwingStateKeyRva + 0x7CU, keyType) ||
        !TryReadByte(machineAddress + 0xE00U, specialRoute) ||
        !TryReadPointer(machineAddress + 0x98U, ringBase) ||
        !TryReadU32(machineAddress + 0xA0U, stride) ||
        !TryReadU32(machineAddress + 0xA4U, capacity) ||
        !TryReadU32(machineAddress + 0xACU, writeIndex) ||
        type != kSwingTransitionType ||
        decoratedType != kSwingTransitionType ||
        keyType != kSwingTransitionType ||
        size != kSwingTransitionPayloadSize ||
        sequence != expectedSequence || specialRoute != 0U ||
        ringBase != before.base || stride != before.stride ||
        capacity != before.capacity || writeIndex != expectedWriteIndex ||
        !TryReadGameVec3(payloadAddress + kSwingTransitionFirstPointOffset,
                         first) ||
        !TryReadGameVec3(payloadAddress + kSwingTransitionSecondPointOffset,
                         second) ||
        !SameGameVecBits(first, pending.firstTransitionPoint) ||
        !SameGameVecBits(second, pending.secondTransitionPoint) ||
        !TryReadSwingPayload(payloadAddress, payloadBytes)) {
        return nativeResult;
    }

    if (before.writeIndex != 0U &&
        before.stride >
            (std::numeric_limits<std::uintptr_t>::max() - before.base) /
                before.writeIndex) {
        return nativeResult;
    }
    const std::uintptr_t queuedSlot =
        before.base + static_cast<std::uintptr_t>(before.stride) *
                          before.writeIndex;
    std::array<std::uint8_t, kSwingTransitionPayloadSize> queuedBytes{};
    if (queuedSlot < before.base ||
        !TryReadSwingPayload(queuedSlot, queuedBytes) ||
        queuedBytes != payloadBytes) {
        return nativeResult;
    }

    ExclusiveSrwGuard guard(g_sideRequestLock);
    SideRequestSnapshot request{};
    const bool hasRequest = ReadSideRequest(request);
    const bool canceled = !hasRequest || request.canceling ||
                          request.token != pending.token ||
                          !RequestMatchesCurrentAirborneHero(request);
    PublishQueuedSubmitEvidenceLocked(
        pending.token, machineAddress, queuedBytes, canceled);
    return nativeResult;
}

[[nodiscard]] ManualWebEnterClaim BuildExecutorEnterClaimLocked(
    std::uintptr_t machine, const void* queuedPayload) noexcept {
    SideRequestSnapshot request{};
    const bool hasRequest = ReadSideRequest(request);
    const auto queuedEvidence =
        ConsumeQueuedSubmitEvidenceLocked(machine, queuedPayload);
    const std::uint64_t queuedToken =
        queuedEvidence.has_value() ? queuedEvidence->token : 0U;
    const bool queuedCanceled =
        queuedEvidence.has_value() && queuedEvidence->canceled;
    const bool requestEligible =
        hasRequest && !request.canceling &&
        RequestMatchesCurrentAirborneHero(request);
    const ManualWebEnterKind kind = ClassifyManualWebEnter(
        queuedToken, queuedCanceled, hasRequest, request.canceling,
        request.token, requestEligible,
        queuedEvidence.has_value() && queuedEvidence->ambiguous);
    if (kind == ManualWebEnterKind::CanceledRequest && hasRequest &&
        (queuedToken == 0U || queuedToken == request.token)) {
        MarkSideRequestCancelingLocked();
    }
    return {kind, queuedToken != 0U ? queuedToken : request.token};
}

bool __fastcall HookExecuteTransition(void* machine,
                                      const void* resolvedStateKey,
                                      const void* queuedPayload) {
    if (reinterpret_cast<std::uintptr_t>(resolvedStateKey) !=
        g_moduleBase + kSwingStateKeyRva) {
        return g_originalExecuteTransition(machine, resolvedStateKey,
                                           queuedPayload);
    }

    ManualWebEnterClaim claim{};
    {
        ExclusiveSrwGuard guard(g_sideRequestLock);
        claim = BuildExecutorEnterClaimLocked(
            reinterpret_cast<std::uintptr_t>(machine), queuedPayload);
    }
    const ExecutorEnterScope previous = g_executorEnterScope;
    g_executorEnterScope = ExecutorEnterScope{queuedPayload, claim, true};
    const bool nativeResult = g_originalExecuteTransition(
        machine, resolvedStateKey, queuedPayload);
    g_executorEnterScope = previous;
    return nativeResult;
}
#endif

#if defined(TRUESWING_CONTROLS_ONLY)
void __fastcall HookInputDeviceUpdate(void* inputObject,
                                      bool updateDevices) {
    g_originalInputDeviceUpdate(inputObject, updateDevices);

    const std::uintptr_t input =
        reinterpret_cast<std::uintptr_t>(inputObject);
    if (!g_ready.load(std::memory_order_acquire) ||
        !ExactVtable(input, kNativeInputVtableRva) || !updateDevices) {
        return;
    }

    NativeGamepadSample gamepad{};
    if (!TrySelectNativeGamepad(input, gamepad)) {
        const ControllerSourceRouteState route =
            g_controllerSourceRoute.Update(std::nullopt);
        ControllerWebInputSample disconnected{};
        disconnected.sourceEpoch = route.sourceEpoch;
        disconnected.connected = false;
        disconnected.focused = ForegroundProcess();
        const ControllerWebInputDecision decision =
            g_controllerInputPolicy.Update(disconnected);
        (void)ApplyControllerDecision(input, decision);
        return;
    }

    const ControllerSourceRouteState route = g_controllerSourceRoute.Update(
        gamepad.connected
            ? std::optional<ControllerSourceRoute::Device>{gamepad.device}
            : std::nullopt);
    if (!gamepad.connected) {
        ControllerWebInputSample disconnected{};
        disconnected.sourceEpoch = route.sourceEpoch;
        disconnected.connected = false;
        disconnected.focused = ForegroundProcess();
        const ControllerWebInputDecision decision =
            g_controllerInputPolicy.Update(disconnected);
        (void)ApplyControllerDecision(input, decision);
        return;
    }

    HeroSnapshot provenHero{};
    const bool foreground = ForegroundProcess();
    const bool freshLeftCandidate =
        std::isfinite(gamepad.leftTrigger) &&
        !g_controllerInputPolicy.LeftTriggerDown() &&
        static_cast<double>(gamepad.leftTrigger) >=
            ControllerWebInputPolicy::kTriggerPressThreshold;
    const bool freshRightCandidate =
        std::isfinite(gamepad.rightTrigger) &&
        !g_controllerInputPolicy.RightTriggerDown() &&
        static_cast<double>(gamepad.rightTrigger) >=
            ControllerWebInputPolicy::kTriggerPressThreshold;
    WebInputEligibility eligibility{};
    eligibility.foreground = foreground;
    if (g_controllerInputPolicy.NativeSwingHeld() ||
        (gamepad.leftShoulderHeld &&
         (freshLeftCandidate || freshRightCandidate))) {
        eligibility = ControllerEligibilityForDown(input, foreground,
                                                   &provenHero);
    }
    const ControllerWebInputSample sample{
        .sourceEpoch = route.sourceEpoch,
        .connected = true,
        .focused = foreground,
        .runtimeReady = eligibility.runtimeReady,
        .airborneProven = eligibility.airborneProven,
        .leftShoulderHeld = gamepad.leftShoulderHeld,
        .nativeSwingAvailable = eligibility.runtimeReady,
        .leftTrigger = static_cast<double>(gamepad.leftTrigger),
        .rightTrigger = static_cast<double>(gamepad.rightTrigger),
    };
    const ControllerWebInputDecision decision =
        g_controllerInputPolicy.Update(sample);

    bool neutralized = true;
    if (decision.consumeLeftTrigger) {
        neutralized = TryWriteFloat(
                          gamepad.device + kGamepadLeftTriggerOffset, 0.0F) &&
                      neutralized;
    }
    if (decision.consumeRightTrigger) {
        neutralized = TryWriteFloat(
                          gamepad.device + kGamepadRightTriggerOffset, 0.0F) &&
                      neutralized;
    }
    if (!neutralized ||
        !ApplyControllerDecision(
            input, decision,
            eligibility.airborneProven ? &provenHero : nullptr)) {
        const ControllerWebInputDecision cancel =
            g_controllerInputPolicy.CancelAndReset();
        (void)ApplyControllerDecision(input, cancel);
        (void)g_controllerSourceRoute.Update(std::nullopt);
    }
}
#endif

bool __fastcall HookInputDispatch(void* inputObject, std::uint32_t message,
                                   std::uintptr_t wParam, std::intptr_t lParam,
                                  void* rawInputCopy) {
    const std::uintptr_t input =
        reinterpret_cast<std::uintptr_t>(inputObject);
    if (!g_ready.load(std::memory_order_acquire) ||
        !ExactVtable(input, kNativeInputVtableRva)) {
        return g_originalInputDispatch(inputObject, message, wParam, lParam,
                                       rawInputCopy);
    }

    const bool focusLost =
        message == WM_KILLFOCUS ||
        message == WM_CANCELMODE ||
        (message == WM_ACTIVATEAPP && wParam == FALSE) ||
        (message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE);
    if (focusLost) {
#if !defined(TRUESWING_CONTROLS_ONLY)
        g_movementKeyMask.store(0U, std::memory_order_release);
#endif
    }
    if (focusLost) {
        // Keyboard/mouse ups may be lost without RIDEV_INPUTSINK. Controller
        // capture remains quarantined until its polled trigger reaches neutral.
        CancelManualHold(input);
    }

    if (message == WM_LBUTTONDBLCLK && g_inputPolicy.LeftCaptured()) {
        return true;
    }
    if (message == WM_RBUTTONDBLCLK && g_inputPolicy.RightCaptured()) {
        return true;
    }
    if (message != WM_INPUT || rawInputCopy == nullptr) {
        return g_originalInputDispatch(inputObject, message, wParam, lParam,
                                       rawInputCopy);
    }

    RAWINPUTHEADER header{};
    __try {
        header = *reinterpret_cast<const RAWINPUTHEADER*>(rawInputCopy);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return g_originalInputDispatch(inputObject, message, wParam, lParam,
                                       rawInputCopy);
    }

    constexpr std::size_t kRawDataOffset = offsetof(RAWINPUT, data);
    const std::size_t requiredSize =
        header.dwType == RIM_TYPEKEYBOARD
            ? kRawDataOffset + sizeof(RAWKEYBOARD)
            : header.dwType == RIM_TYPEMOUSE
                  ? kRawDataOffset + sizeof(RAWMOUSE)
                  : 0U;
    if (requiredSize == 0U || header.dwSize < requiredSize ||
        header.dwSize > 4096U) {
        return g_originalInputDispatch(inputObject, message, wParam, lParam,
                                       rawInputCopy);
    }

    RAWINPUT local{};
    __try {
        std::memcpy(&local, rawInputCopy,
                    std::min<std::size_t>(sizeof(local), header.dwSize));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return g_originalInputDispatch(inputObject, message, wParam, lParam,
                                       rawInputCopy);
    }

#if !defined(TRUESWING_CONTROLS_ONLY)
    if (local.header.dwType == RIM_TYPEKEYBOARD &&
        ObserveSwingMovementKey(local.data.keyboard)) {
        // Steering observes the edge but never consumes or rewrites it. The
        // game's native movement input receives the original Raw Input bytes.
        return g_originalInputDispatch(inputObject, message, wParam, lParam,
                                       rawInputCopy);
    }
#endif
    if (local.header.dwType == RIM_TYPEKEYBOARD &&
        IsLeftShift(local.data.keyboard)) {
        const bool down =
            (local.data.keyboard.Flags & RI_KEY_BREAK) == 0U;
        g_physicalShiftDown = down;
        if (g_syntheticShiftDown) {
            return true;
        }
        const bool result = g_originalInputDispatch(
            inputObject, message, wParam, lParam, rawInputCopy);
        if (!down && g_nativeBridgeShiftDown) {
            // A physical press may inherit the native down edge after the
            // mouse owner releases. Its real up completes that handoff. If
            // focus is lost first, the focus path above forces this edge.
            g_nativeBridgeShiftDown = false;
        }
        return result;
    }
    if (local.header.dwType != RIM_TYPEMOUSE) {
        return g_originalInputDispatch(inputObject, message, wParam, lParam,
                                       rawInputCopy);
    }

    USHORT& flags = local.data.mouse.usButtonFlags;
    const auto handleAmbiguousPair = [&](USHORT downFlag, USHORT upFlag,
                                         WebSide side) noexcept {
        const USHORT pair = static_cast<USHORT>(downFlag | upFlag);
        if ((flags & pair) != pair) {
            return false;
        }
        const bool captured = side == WebSide::Left
                                  ? g_inputPolicy.LeftCaptured()
                                  : g_inputPolicy.RightCaptured();
        if (captured) {
            const WebInputDecision release = g_inputPolicy.Update(
                side == WebSide::Left ? MouseButtonTransition::LeftUp
                                      : MouseButtonTransition::RightUp,
                {});
            flags = static_cast<USHORT>(flags & ~pair);
            (void)ApplyDecision(input, release);
        }
        return true;
    };
    const bool skipLeft = handleAmbiguousPair(
        RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_LEFT_BUTTON_UP, WebSide::Left);
    const bool skipRight = handleAmbiguousPair(
        RI_MOUSE_RIGHT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_UP, WebSide::Right);

    const auto process = [&](USHORT rawFlag,
                             MouseButtonTransition transition) noexcept {
        if ((flags & rawFlag) == 0U) {
            return;
        }
        WebInputEligibility eligibility{};
        HeroSnapshot provenHero{};
        if (transition == MouseButtonTransition::LeftDown ||
            transition == MouseButtonTransition::RightDown) {
            eligibility = EligibilityForDown(input, ForegroundProcess(),
                                             &provenHero);
        }
        const WebInputDecision decision =
            g_inputPolicy.Update(transition, eligibility);
        if (!decision.consume) {
            return;
        }
        flags = static_cast<USHORT>(flags & ~rawFlag);
        if (!ApplyDecision(input, decision,
                           eligibility.airborneProven ? &provenHero
                                                     : nullptr)) {
            // Fail closed: remove internal ownership and do not inject the
            // native Swing edge if its exact hero proof was unavailable.
            (void)g_inputPolicy.CancelNativeHold();
            g_inputPolicy.Reset();
            ClearSideRequest();
        }
    };

    if (!skipLeft) {
        process(RI_MOUSE_LEFT_BUTTON_DOWN, MouseButtonTransition::LeftDown);
        process(RI_MOUSE_LEFT_BUTTON_UP, MouseButtonTransition::LeftUp);
    }
    if (!skipRight) {
        process(RI_MOUSE_RIGHT_BUTTON_DOWN, MouseButtonTransition::RightDown);
        process(RI_MOUSE_RIGHT_BUTTON_UP, MouseButtonTransition::RightUp);
    }

    return g_originalInputDispatch(inputObject, message, wParam, lParam,
                                   &local);
}

} // namespace

bool PrepareManualWebRuntime(std::uintptr_t moduleBase, std::string& reason) {
    if (moduleBase == 0U) {
        reason = "manual-web module base is null";
        return false;
    }
    if (!Matches(moduleBase + kSwingPointHunterUpdateRva,
                 kHunterUpdateSignature) ||
        !Matches(moduleBase + kSwingPointSubmitRva,
                 kSwingPointSubmitSignature) ||
        !Matches(moduleBase + kSwingPointAggregateRva,
                 kSwingPointAggregateSignature) ||
        !Matches(moduleBase + kNativeInputDispatchRva,
                  kInputDispatchSignature) ||
#if defined(TRUESWING_CONTROLS_ONLY)
        !Matches(moduleBase + kNativeInputDeviceUpdateRva,
                 kInputDeviceUpdateSignature) ||
        !Matches(moduleBase + kNativeMainGamepadGetterEvidenceRva,
                 kNativeMainGamepadGetterEvidence) ||
        !Matches(moduleBase + kNativeInputOrderingEvidenceRva,
                 kNativeInputOrderingEvidence) ||
        !Matches(moduleBase + kNativeL1MappingEvidenceRva,
                 kNativeL1MappingEvidence) ||
        !Matches(moduleBase + kNativeTriggerMappingEvidenceRva,
                 kNativeTriggerMappingEvidence) ||
        !Matches(moduleBase + kNativeL2AggregateEvidenceRva,
                 kNativeL2AggregateEvidence) ||
        !Matches(moduleBase + kNativeR2AggregateEvidenceRva,
                 kNativeR2AggregateEvidence) ||
#endif
        !Matches(moduleBase + kNativeKeyboardParserRva,
                  kKeyboardParserSignature) ||
        !Matches(moduleBase + kNativeIntSettingGetterRva,
                 kIntSettingGetterSignature) ||
        !Matches(moduleBase + kSwingToggleEvidenceRva,
                 kSwingToggleEvidenceSignature) ||
        !Matches(moduleBase + kAirborneEvidenceRva,
                 kAirborneEvidenceSignature) ||
        !Matches(moduleBase + kAirborneStoreEvidenceRva,
                 kAirborneStoreEvidenceSignature) ||
        !Matches(moduleBase + kSwingActionRegistrationEvidenceRva,
                 kSwingActionRegistrationEvidence) ||
        !Matches(moduleBase + kActionBindingGetterEvidenceRva,
                 kActionBindingGetterEvidence) ||
        !Matches(moduleBase + kNativeHandDerivationEvidenceRva,
                 kNativeHandDerivationEvidence) ||
        !Matches(moduleBase + kNativeRightHandPredicateEvidenceRva,
                 kNativeRightHandPredicateEvidence) ||
        !Matches(moduleBase + kNativeRightHandSetEvidenceRva,
                 kNativeRightHandSetEvidence) ||
        !Matches(moduleBase + kNativeLeftHandSetEvidenceRva,
                 kNativeLeftHandSetEvidence)
#if !defined(TRUESWING_CONTROLS_ONLY)
        ||
        !Matches(moduleBase + kSwingTransitionPayloadSizeEvidenceRva,
                 kSwingTransitionPayloadSizeEvidence) ||
        !Matches(moduleBase + kSwingTransitionPayloadCopyEvidenceRva,
                 kSwingTransitionPayloadCopyEvidence) ||
        !Matches(moduleBase + kSwingTransitionPayloadReadEvidenceRva,
                 kSwingTransitionPayloadReadEvidence) ||
        !Matches(moduleBase + kSwingTransitionQueueCopyEvidenceRva,
                 kSwingTransitionQueueCopyEvidence) ||
        !Matches(moduleBase + kRequestTransitionRva,
                 kRequestTransitionSignature) ||
        !Matches(moduleBase + kExecuteTransitionRva,
                 kExecuteTransitionSignature)
#endif
    ) {
        reason = "manual-web exact native signature changed";
        return false;
    }
    g_moduleBase = moduleBase;
    g_swingPointAggregate = reinterpret_cast<SwingPointAggregate>(
        moduleBase + kSwingPointAggregateRva);
    g_nativeKeyboardParser = reinterpret_cast<NativeKeyboardParser>(
        moduleBase + kNativeKeyboardParserRva);
    g_nativeIntSettingGetter = reinterpret_cast<NativeIntSettingGetter>(
        moduleBase + kNativeIntSettingGetterRva);
    reason = "manual-web exact routes verified";
    return true;
}

std::array<ManualWebHookSpec, kManualWebHookCount>
GetManualWebHookSpecs() noexcept {
#if defined(TRUESWING_CONTROLS_ONLY)
    return {
        ManualWebHookSpec{kSwingPointHunterUpdateRva,
                          reinterpret_cast<void*>(&HookHunterUpdate),
                          reinterpret_cast<void**>(&g_originalHunterUpdate),
                          "swing-point-hunter-update"},
        ManualWebHookSpec{kSwingPointSubmitRva,
                          reinterpret_cast<void*>(&HookSwingPointSubmit),
                          reinterpret_cast<void**>(&g_originalSwingPointSubmit),
                          "swing-point-side-submit"},
        ManualWebHookSpec{kNativeInputDispatchRva,
                          reinterpret_cast<void*>(&HookInputDispatch),
                          reinterpret_cast<void**>(&g_originalInputDispatch),
                          "native-input-dispatch"},
        ManualWebHookSpec{
            kNativeInputDeviceUpdateRva,
            reinterpret_cast<void*>(&HookInputDeviceUpdate),
            reinterpret_cast<void**>(&g_originalInputDeviceUpdate),
            "native-controller-device-update"},
    };
#else
    return {
        ManualWebHookSpec{kSwingPointHunterUpdateRva,
                          reinterpret_cast<void*>(&HookHunterUpdate),
                          reinterpret_cast<void**>(&g_originalHunterUpdate),
                          "swing-point-hunter-update"},
        ManualWebHookSpec{kSwingPointSubmitRva,
                          reinterpret_cast<void*>(&HookSwingPointSubmit),
                          reinterpret_cast<void**>(&g_originalSwingPointSubmit),
                          "swing-point-side-submit"},
        ManualWebHookSpec{kNativeInputDispatchRva,
                          reinterpret_cast<void*>(&HookInputDispatch),
                          reinterpret_cast<void**>(&g_originalInputDispatch),
                          "native-input-dispatch"},
        ManualWebHookSpec{
            kSwingTransitionPayloadCopyEvidenceRva,
            reinterpret_cast<void*>(&HookSwingTransitionPayloadCopy),
            reinterpret_cast<void**>(&g_originalSwingTransitionPayloadCopy),
            "swing-transition-payload-copy"},
        ManualWebHookSpec{kRequestTransitionRva,
                          reinterpret_cast<void*>(&HookRequestTransition),
                          reinterpret_cast<void**>(&g_originalRequestTransition),
                          "swing-request-transition"},
        ManualWebHookSpec{kExecuteTransitionRva,
                          reinterpret_cast<void*>(&HookExecuteTransition),
                          reinterpret_cast<void**>(&g_originalExecuteTransition),
                          "swing-execute-transition"},
    };
#endif
}

#if !defined(TRUESWING_CONTROLS_ONLY)
ManualWebEnterClaim ClaimManualWebSwingEnter(const void* payload) noexcept {
    if (g_executorEnterScope.valid &&
        g_executorEnterScope.queuedPayload == payload) {
        const ManualWebEnterClaim claim = g_executorEnterScope.claim;
        // A queued payload may reach more than one native +0x58 callsite in
        // the generic executor. Only its first exact HeroStateSwing Enter may
        // consume manual ownership; any repeat falls through and fails closed.
        g_executorEnterScope.valid = false;
        return claim;
    }

    // A manual request may authorize custom movement only while the exact
    // queued Swing transition executor scopes that same payload on this
    // thread. Any other Enter stays native and invalidates a request that was
    // concurrently waiting, rather than borrowing it.
    ExclusiveSrwGuard guard(g_sideRequestLock);
    SideRequestSnapshot request{};
    const bool hasRequest = ReadSideRequest(request);
    if (hasRequest) {
        MarkSideRequestCancelingLocked();
        return {ManualWebEnterKind::CanceledRequest, request.token};
    }
    return {};
}

bool CompleteManualWebSwingEnter(
    const ManualWebEnterClaim& claim) noexcept {
    ExclusiveSrwGuard guard(g_sideRequestLock);
    SideRequestSnapshot request{};
    const bool hasRequest = ReadSideRequest(request);
    if (claim.kind == ManualWebEnterKind::Vanilla) {
        // If a click request appeared while a previously vanilla Enter was in
        // flight, do not let that old transition borrow manual ownership.
        return !hasRequest;
    }
    if (claim.kind == ManualWebEnterKind::CanceledRequest || !hasRequest ||
        request.canceling || request.token != claim.token) {
        return false;
    }
    if (!RequestMatchesCurrentAirborneHero(request)) {
        MarkSideRequestCancelingLocked();
        return false;
    }
    return true;
}

SwingMovementInput ReadSwingMovementInput() noexcept {
    if (!g_ready.load(std::memory_order_acquire)) {
        return {};
    }
    return DecodeSwingMovementInput(
        g_movementKeyMask.load(std::memory_order_acquire));
}
#endif

void SetManualWebRuntimeReady(bool ready) noexcept {
    g_ready.store(ready, std::memory_order_release);
    if (!ready) {
#if !defined(TRUESWING_CONTROLS_ONLY)
        g_movementKeyMask.store(0U, std::memory_order_release);
#endif
        ClearSideRequest();
        g_inputPolicy.Reset();
#if defined(TRUESWING_CONTROLS_ONLY)
        (void)g_controllerInputPolicy.CancelAndReset();
        g_controllerSourceRoute.Reset();
#endif
        g_syntheticShiftDown = false;
        g_physicalShiftDown = false;
        g_nativeBridgeShiftDown = false;
    }
}

} // namespace trueswing::rebuild::runtime
