#include "../src/ward_policy.h"

#include <cassert>

int main()
{
	using WardUniversalPatchSKSE::EffectVisualSummary;
	using WardUniversalPatchSKSE::IsWardPulseEvent;
	using WardUniversalPatchSKSE::ShouldPlayNativePulse;
	using WardUniversalPatchSKSE::ShouldForward;

	assert(ShouldForward(EffectVisualSummary{ .wardPower = true }));
	assert(ShouldForward(EffectVisualSummary{ .hitReferencesWard = true }));
	assert(ShouldForward(EffectVisualSummary{ .castingReferencesWard = true }));
	assert(ShouldForward(EffectVisualSummary{ .enchantReferencesWard = true }));
	assert(!ShouldForward(EffectVisualSummary{}));
	assert(IsWardPulseEvent("OnWardHit"));
	assert(IsWardPulseEvent("OnHit"));
	assert(!IsWardPulseEvent("OnEffectStart"));
	assert(ShouldPlayNativePulse(true, false));
	assert(!ShouldPlayNativePulse(false, false));
	assert(!ShouldPlayNativePulse(true, true));
	return 0;
}
