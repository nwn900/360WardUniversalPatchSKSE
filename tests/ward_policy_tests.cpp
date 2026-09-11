#include "../src/ward_policy.h"

#include <cassert>

int main()
{
	using WardVisualForwarder::EffectVisualSummary;
	using WardVisualForwarder::IsWardPulseEvent;
	using WardVisualForwarder::ShouldForward;
	using WardVisualForwarder::ShouldPlayNativePulse;
	using WardVisualForwarder::ShouldReplaceArt;

	assert(ShouldForward(EffectVisualSummary{ .wardPower = true }));
	assert(ShouldForward(EffectVisualSummary{ .hitReferencesWard = true }));
	assert(ShouldForward(EffectVisualSummary{ .castingReferencesWard = true }));
	assert(ShouldForward(EffectVisualSummary{ .enchantReferencesWard = true }));
	assert(!ShouldForward(EffectVisualSummary{}));

	assert(ShouldReplaceArt(true, false, false));
	assert(ShouldReplaceArt(true, true, true));
	assert(!ShouldReplaceArt(true, true, false));
	assert(ShouldReplaceArt(false, true, true));
	assert(!ShouldReplaceArt(false, false, false));

	assert(IsWardPulseEvent("OnWardHit"));
	assert(IsWardPulseEvent("OnHit"));
	assert(!IsWardPulseEvent("OnEffectStart"));
	assert(ShouldPlayNativePulse(true, false));
	assert(!ShouldPlayNativePulse(false, false));
	assert(!ShouldPlayNativePulse(true, true));
	return 0;
}
