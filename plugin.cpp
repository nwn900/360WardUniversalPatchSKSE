#include "PCH.h"

#include "src/ward_policy.h"

#ifdef GetObject
#undef GetObject
#endif

namespace
{
	using RE::BGSArtObject;
	using RE::BGSReferenceEffect;
	using RE::EffectSetting;
	using RE::Actor;
	using RE::TESObjectREFR;

	constexpr RE::FormID kWardInHandFormID = 0x0253F1;
	constexpr RE::FormID kWardHitFormID = 0x018124;
	constexpr RE::FormID k360WardHitFormID = 0x000802;
	constexpr RE::FormID k360WardPulseFormID = 0x000803;

	BGSArtObject* g_wardInHand = nullptr;
	BGSArtObject* g_wardHit = nullptr;
	BGSArtObject* g_ward360Hit = nullptr;
	BGSArtObject* g_wardShieldPulse = nullptr;

	using SendEventFunction = void (*)(RE::BSScript::IVirtualMachine*, RE::VMHandle, const RE::BSFixedString&, RE::BSScript::IFunctionArguments*);
	static inline REL::Relocation<SendEventFunction> g_originalSendEvent;
	static inline bool g_pulseHookInstalled = false;

	[[nodiscard]] BGSArtObject* LookupArt(const RE::FormID a_formID, const std::string_view a_plugin)
	{
		return RE::TESDataHandler::GetSingleton()->LookupForm<BGSArtObject>(a_formID, a_plugin);
	}

	[[nodiscard]] BGSArtObject* LookupShieldPulseArt()
	{
		const auto* pulseVisualEffect = RE::TESDataHandler::GetSingleton()->LookupForm<BGSReferenceEffect>(k360WardPulseFormID, "360 Ward");
		if (pulseVisualEffect && pulseVisualEffect->data.artObject) {
			return pulseVisualEffect->data.artObject;
		}

		return LookupArt(k360WardHitFormID, "360 Ward");
	}

	[[nodiscard]] bool IsWardArt(const BGSArtObject* a_art,
		const BGSArtObject* a_wardInHand,
		const BGSArtObject* a_wardHit,
		const BGSArtObject* a_360WardHit) noexcept
	{
		return a_art && (a_art == a_wardInHand || a_art == a_wardHit || a_art == a_360WardHit);
	}

	[[nodiscard]] bool IsWardEffect(const EffectSetting* a_effect) noexcept
	{
		if (!a_effect) {
			return false;
		}

		const auto& data = a_effect->data;
		return data.primaryAV == RE::ActorValue::kWardPower ||
			IsWardArt(data.castingArt, g_wardInHand, g_wardHit, g_ward360Hit) ||
			IsWardArt(data.hitEffectArt, g_wardInHand, g_wardHit, g_ward360Hit) ||
			IsWardArt(data.enchantEffectArt, g_wardInHand, g_wardHit, g_ward360Hit);
	}

	[[nodiscard]] bool HasSphereWardScript(const RE::ActiveEffect* a_effect, const RE::BSScript::IObjectHandlePolicy* a_policy)
	{
		if (!a_effect || !a_policy) {
			return false;
		}

		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm) {
			return false;
		}

		const auto handle = a_policy->GetHandleForObject(RE::ActiveEffect::VMTYPEID, a_effect);
		if (handle == a_policy->EmptyHandle()) {
			return false;
		}

		const auto it = vm->attachedScripts.find(handle);
		if (it == vm->attachedScripts.end()) {
			return false;
		}

		for (const auto& attachedScript : it->second) {
			const auto* scriptObject = attachedScript.get();
			const auto* typeInfo = scriptObject ? scriptObject->GetTypeInfo() : nullptr;
			const auto* scriptName = typeInfo ? typeInfo->GetName() : nullptr;
			if (scriptName && std::string_view(scriptName) == "SphereWard") {
				return true;
			}
		}

		return false;
	}

	class WardEffectVisitor final : public RE::MagicTarget::ForEachActiveEffectVisitor
	{
	public:
		WardEffectVisitor(const RE::BSScript::IObjectHandlePolicy* a_policy) :
			policy(a_policy)
		{}

		RE::BSContainer::ForEachResult Accept(RE::ActiveEffect* a_effect) override
		{
			const auto* baseEffect = a_effect ? a_effect->GetBaseObject() : nullptr;
			if (!IsWardEffect(baseEffect)) {
				return RE::BSContainer::ForEachResult::kContinue;
			}

			if (HasSphereWardScript(a_effect, policy)) {
				hasSphereWardScript = true;
			} else {
				hasNativeCandidate = true;
			}

			return RE::BSContainer::ForEachResult::kContinue;
		}

		const RE::BSScript::IObjectHandlePolicy* policy;
		bool hasNativeCandidate = false;
		bool hasSphereWardScript = false;
	};

	[[nodiscard]] TESObjectREFR* ExtractReferenceArgument(
		RE::BSScript::IVirtualMachine* a_vm,
		RE::BSScript::IFunctionArguments* a_args)
	{
		if (!a_vm || !a_args) {
			return nullptr;
		}

		auto* policy = a_vm->GetObjectHandlePolicy();
		if (!policy) {
			return nullptr;
		}

		RE::BSScrapArray<RE::BSScript::Variable> values;
		if (!(*a_args)(values) || values.empty() || !values[0].IsObject()) {
			return nullptr;
		}

		const auto object = values[0].GetObject();
		if (!object || !object->IsValid()) {
			return nullptr;
		}

		auto* form = static_cast<RE::TESForm*>(policy->GetObjectForHandle(static_cast<RE::VMTypeID>(RE::FormType::Reference), object->GetHandle()));
		return form ? form->As<TESObjectREFR>() : nullptr;
	}

	void QueueShieldPulse(Actor* a_ward, TESObjectREFR* a_facingReference)
	{
		if (!a_ward || !g_wardShieldPulse) {
			return;
		}

		const auto wardHandle = a_ward->GetHandle();
		const auto facingHandle = a_facingReference ? a_facingReference->GetHandle() : RE::ObjectRefHandle{};
		const auto artObject = g_wardShieldPulse;
		const auto* taskInterface = SKSE::GetTaskInterface();
		if (!taskInterface) {
			return;
		}

		taskInterface->AddTask([wardHandle, facingHandle, artObject]() {
			const auto wardReference = wardHandle.get();
			if (!wardReference || !artObject) {
				return;
			}

			auto* wardActor = wardReference->As<Actor>();
			if (!wardActor) {
				return;
			}

			auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			auto* policy = vm ? vm->GetObjectHandlePolicy() : nullptr;
			auto* magicTarget = wardActor->GetMagicTarget();
			if (!policy || !magicTarget) {
				return;
			}

			WardEffectVisitor visitor(policy);
			magicTarget->VisitEffects(visitor);
			if (!WardVisualForwarder::ShouldPlayNativePulse(visitor.hasNativeCandidate, visitor.hasSphereWardScript)) {
				return;
			}

			TESObjectREFR* facingReference = nullptr;
			RE::NiPointer<TESObjectREFR> facingSmartPointer;
			if (facingHandle) {
				facingSmartPointer = facingHandle.get();
				facingReference = facingSmartPointer.get();
			}

			wardActor->ApplyArtObject(artObject, -1.0f, facingReference, facingReference != nullptr);
		});
	}

	void HandleWardEvent(
		RE::BSScript::IVirtualMachine* a_vm,
		RE::VMHandle a_handle,
		const RE::BSFixedString& a_eventName,
		RE::BSScript::IFunctionArguments* a_args)
	{
		if (!a_vm || !g_wardShieldPulse || !WardVisualForwarder::IsWardPulseEvent(a_eventName.c_str())) {
			return;
		}

		auto* policy = a_vm->GetObjectHandlePolicy();
		if (!policy) {
			return;
		}

		// SendAndRelayEvent first sends the event to the TESObjectREFR itself,
		// then relays it to aliases and active magic effects. Restrict the native
		// pulse to that actor-level dispatch so the relay does not duplicate it.
		auto* form = policy->GetObjectForHandle(RE::FormType::Reference, a_handle);
		auto* ward = form ? form->As<Actor>() : nullptr;
		if (!ward) {
			return;
		}

		QueueShieldPulse(ward, ExtractReferenceArgument(a_vm, a_args));
	}

	void HookedSendEvent(
		RE::BSScript::IVirtualMachine* a_vm,
		RE::VMHandle a_handle,
		const RE::BSFixedString& a_eventName,
		RE::BSScript::IFunctionArguments* a_args)
	{
		if (g_originalSendEvent.address() != 0) {
			g_originalSendEvent(a_vm, a_handle, a_eventName, a_args);
		}

		HandleWardEvent(a_vm, a_handle, a_eventName, a_args);
	}

	[[nodiscard]] bool InstallPulseHook()
	{
		if (g_pulseHookInstalled || !g_wardShieldPulse) {
			return g_pulseHookInstalled;
		}

		REL::Relocation<std::uintptr_t> virtualMachineVTable{ RE::VTABLE_BSScript__Internal__VirtualMachine[0] };
		const auto sendEventIndex = REL::Module::IsVR() ? 0x26 : 0x24;
		g_originalSendEvent = virtualMachineVTable.write_vfunc(sendEventIndex, HookedSendEvent);
		g_pulseHookInstalled = g_originalSendEvent.address() != 0;
		return g_pulseHookInstalled;
	}

	struct PatchStats
	{
		std::uint32_t scanned{};
		std::uint32_t forwarded{};
		std::uint32_t casting{};
		std::uint32_t hit{};
		std::uint32_t enchant{};
	};

	[[nodiscard]] PatchStats ForwardWardVisuals()
	{
		PatchStats stats;
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return stats;
		}

		auto* wardInHand = LookupArt(kWardInHandFormID, "Skyrim.esm");
		auto* wardHit = LookupArt(kWardHitFormID, "Skyrim.esm");
		auto* ward360Hit = LookupArt(k360WardHitFormID, "360 Ward.esp");
		g_wardInHand = wardInHand;
		g_wardHit = wardHit;
		g_ward360Hit = ward360Hit;
		g_wardShieldPulse = LookupShieldPulseArt();
		if (!wardInHand || !wardHit) {
			SKSE::log::error("Required vanilla ward ArtObjects could not be resolved");
			return stats;
		}

		for (auto* effect : dataHandler->GetFormArray<EffectSetting>()) {
			if (!effect) {
				continue;
			}
			++stats.scanned;

			const auto& effectData = effect->data;
			const WardVisualForwarder::EffectVisualSummary summary{
				.wardPower = effectData.primaryAV == RE::ActorValue::kWardPower,
				.castingReferencesWard = IsWardArt(effectData.castingArt, wardInHand, wardHit, ward360Hit),
				.hitReferencesWard = IsWardArt(effectData.hitEffectArt, wardInHand, wardHit, ward360Hit),
				.enchantReferencesWard = IsWardArt(effectData.enchantEffectArt, wardInHand, wardHit, ward360Hit)
			};

			if (!WardVisualForwarder::ShouldForward(summary)) {
				continue;
			}
			++stats.forwarded;

			// Preserve the vanilla ward slot semantics: casting art is the
			// in-hand visual and hit art is the 360 Ward sphere visual.
			if (summary.wardPower || summary.castingReferencesWard) {
				effect->data.castingArt = wardInHand;
				++stats.casting;
			}
			if (summary.wardPower || summary.hitReferencesWard) {
				effect->data.hitEffectArt = wardHit;
				++stats.hit;
			}
			if (summary.enchantReferencesWard) {
				effect->data.enchantEffectArt = wardHit;
				++stats.enchant;
			}
		}

		return stats;
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);

	SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* a_message) {
		if (a_message->type != SKSE::MessagingInterface::kDataLoaded) {
			return;
		}

		const auto stats = ForwardWardVisuals();
		const auto pulseHookInstalled = InstallPulseHook();
		SKSE::log::info(
			"WardVisualForwarder: scanned {}, forwarded {}, casting {}, hit {}, enchant {}, native pulse hook {}",
			stats.scanned,
			stats.forwarded,
			stats.casting,
			stats.hit,
			stats.enchant,
			pulseHookInstalled ? "installed" : "not installed");
	});

	return true;
}
