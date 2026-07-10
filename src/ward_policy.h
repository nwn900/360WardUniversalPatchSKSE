#pragma once

#include <string_view>

namespace WardUniversalPatchSKSE
{
	struct EffectVisualSummary
	{
		bool wardPower{};
		bool castingReferencesWard{};
		bool hitReferencesWard{};
		bool enchantReferencesWard{};
	};

	[[nodiscard]] constexpr bool ShouldForward(const EffectVisualSummary& a_summary) noexcept
	{
		return a_summary.wardPower ||
			a_summary.castingReferencesWard ||
			a_summary.hitReferencesWard ||
			a_summary.enchantReferencesWard;
	}

	[[nodiscard]] constexpr bool IsWardPulseEvent(const std::string_view a_eventName) noexcept
	{
		return a_eventName == "OnWardHit" || a_eventName == "OnHit";
	}

	[[nodiscard]] constexpr bool ShouldPlayNativePulse(const bool a_hasNativeCandidate, const bool a_hasSphereWardScript) noexcept
	{
		return a_hasNativeCandidate && !a_hasSphereWardScript;
	}
}
