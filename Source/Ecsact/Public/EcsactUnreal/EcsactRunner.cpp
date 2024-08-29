#include "EcsactUnreal/EcsactRunner.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectIterator.h"
#include "EcsactUnreal/EcsactRunnerSubsystem.h"
#include "EcsactUnreal/Ecsact.h"
#include "ecsact/runtime/common.h"

UEcsactRunner::UEcsactRunner() : EventsCollector{} {
	EventsCollector.init_callback_user_data = this;
	EventsCollector.update_callback_user_data = this;
	EventsCollector.remove_callback_user_data = this;
	EventsCollector.entity_created_callback_user_data = this;
	EventsCollector.entity_destroyed_callback_user_data = this;

	EventsCollector.init_callback = ThisClass::OnInitComponentRaw;
	EventsCollector.update_callback = ThisClass::OnUpdateComponentRaw;
	EventsCollector.remove_callback = ThisClass::OnRemoveComponentRaw;
	EventsCollector.entity_created_callback = ThisClass::OnEntityCreatedRaw;
	EventsCollector.entity_destroyed_callback = ThisClass::OnEntityDestroyedRaw;
}

auto UEcsactRunner::Start() -> void {
	bIsStopped = false;
	InitRunnerSubsystems();
}

auto UEcsactRunner::Stop() -> void {
	ShutdownRunnerSubsystems();
	bIsStopped = true;
}

auto UEcsactRunner::IsStopped() const -> bool {
	return bIsStopped;
}

auto UEcsactRunner::Tick(float DeltaTime) -> void {
}

auto UEcsactRunner::GetStatId() const -> TStatId {
	RETURN_QUICK_DECLARE_CYCLE_STAT( // NOLINT
		UEcsactRunner,
		STATGROUP_Tickables
	);
}

auto UEcsactRunner::IsTickable() const -> bool {
	return !IsTemplate() && !bIsStopped;
}

auto UEcsactRunner::InitRunnerSubsystems() -> void {
	auto subsystem_types = TArray<UClass*>{};
	for(auto it = TObjectIterator<UClass>{}; it; ++it) {
		auto uclass = *it;

		if(!uclass->IsChildOf(UEcsactRunnerSubsystem::StaticClass())) {
			continue;
		}

		if(uclass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated)) {
			continue;
		}

		subsystem_types.Add(uclass);
	}

	check(RunnerSubsystems.IsEmpty());
	RunnerSubsystems.Empty();

	for(auto t : subsystem_types) {
		UE_LOG(Ecsact, Log, TEXT("Starting ecsact subsystem %s"), *t->GetName());
		auto subsystem = NewObject<UEcsactRunnerSubsystem>(this, t);
		subsystem->AddToRoot();
		RunnerSubsystems.Add(subsystem);
	}

	for(auto subsystem : RunnerSubsystems) {
		subsystem->RunnerStart(this);
	}
}

auto UEcsactRunner::ShutdownRunnerSubsystems() -> void {
	for(auto subsystem : RunnerSubsystems) {
		if(subsystem == nullptr) {
			continue;
		}
		subsystem->RunnerStop(this);
	}

	RunnerSubsystems.Empty();
}

auto UEcsactRunner::GetEventsCollector() -> ecsact_execution_events_collector* {
	return &EventsCollector;
}

auto UEcsactRunner::OnInitComponentRaw(
	ecsact_event        event,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	void*               callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);
	for(auto subsystem : self->RunnerSubsystems) {
		subsystem->InitComponentRaw(entity_id, component_id, component_data);
	}
}

auto UEcsactRunner::OnUpdateComponentRaw(
	ecsact_event        event,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	void*               callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);
	for(auto subsystem : self->RunnerSubsystems) {
		subsystem->UpdateComponentRaw(entity_id, component_id, component_data);
	}
}

auto UEcsactRunner::OnRemoveComponentRaw(
	ecsact_event        event,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	void*               callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);
	for(auto subsystem : self->RunnerSubsystems) {
		subsystem->RemoveComponentRaw(entity_id, component_id, component_data);
	}
}

auto UEcsactRunner::OnEntityCreatedRaw(
	ecsact_event                 event,
	ecsact_entity_id             entity_id,
	ecsact_placeholder_entity_id placeholder_entity_id,
	void*                        callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);
	for(auto subsystem : self->RunnerSubsystems) {
		subsystem->EntityCreated(static_cast<int32>(entity_id));
	}
}

auto UEcsactRunner::OnEntityDestroyedRaw(
	ecsact_event                 event,
	ecsact_entity_id             entity_id,
	ecsact_placeholder_entity_id placeholder_entity_id,
	void*                        callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);
	for(auto subsystem : self->RunnerSubsystems) {
		subsystem->EntityDestroyed(static_cast<int32>(entity_id));
	}
}
