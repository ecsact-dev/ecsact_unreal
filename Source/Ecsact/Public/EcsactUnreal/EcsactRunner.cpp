#include "EcsactUnreal/EcsactRunner.h"
#include "EcsactUnreal/EcsactAsyncRunnerEvents.h"
#include "EcsactUnreal/EcsactSettings.h"
#include "EcsactUnreal/EcsactUnrealExecutionOptions.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectIterator.h"
#include "EcsactUnreal/EcsactRunnerSubsystem.h"
#include "EcsactUnreal/Ecsact.h"
#include "ecsact/runtime/common.h"

static auto GetRunnerSubsystemsWarn(
	UEcsactRunner* runner,
	const TCHAR*   EventName
) -> const TArray<UEcsactRunnerSubsystem*>& {
	auto& subsystems = runner->GetSubsystemArray<UEcsactRunnerSubsystem>();
	if(subsystems.IsEmpty()) {
		UE_LOG(
			Ecsact,
			Warning,
			TEXT("No EcsactRunner subsystems available for event '%s'"),
			EventName
		);
	}
	return subsystems;
}

UEcsactRunner::UEcsactRunner() : EventsCollector{} {
	ExecutionOptions = CreateDefaultSubobject<UEcsactUnrealExecutionOptions>( //
		TEXT("ExecutionOptions")
	);

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

auto UEcsactRunner::GetWorld() const -> UWorld* {
	return World;
}

auto UEcsactRunner::StreamImpl(
	ecsact_entity_id    Entity,
	ecsact_component_id ComponentId,
	const void*         ComponentData
) -> void {
}

auto UEcsactRunner::Start() -> void {
	bIsStopped = false;

	RunnerSubsystems.Initialize(this);

	for(auto subsystem : GetSubsystemArray<UEcsactRunnerSubsystem>()) {
		if(subsystem) {
			UE_LOG(
				Ecsact,
				Log,
				TEXT("Starting Ecsact runner subsystem: %s"),
				*subsystem->GetClass()->GetName()
			);
			subsystem->OwningRunner = this;
			subsystem->RunnerStart(this);
		}
	}
}

auto UEcsactRunner::Stop() -> void {
	for(auto subsystem : GetSubsystemArray<UEcsactRunnerSubsystem>()) {
		if(subsystem) {
			subsystem->RunnerStop(this);
			subsystem->OwningRunner = nullptr;
		}
	}
	RunnerSubsystems.Deinitialize();
	bIsStopped = true;
}

void UEcsactRunner::OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld) {
	for(auto subsystem : GetSubsystemArray<UEcsactRunnerSubsystem>()) {
		if(subsystem) {
			subsystem->WorldChanged(OldWorld, NewWorld);
		}
	}
}

auto UEcsactRunner::IsStopped() const -> bool {
	return bIsStopped;
}

auto UEcsactRunner::HasAsyncEvents() const -> bool {
	return Cast<IEcsactAsyncRunnerEvents>(this) != nullptr;
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

auto UEcsactRunner::CreateEntity() -> EcsactRunnerCreateEntityBuilder {
	return {this, GeneratePlaceholderId()};
}

auto UEcsactRunner::DestroyEntity(ecsact_entity_id Entity) -> void {
	ExecutionOptions->DestroyEntity(Entity);
}

auto UEcsactRunner::GeneratePlaceholderId() -> ecsact_placeholder_entity_id {
	static ecsact_placeholder_entity_id LastPlaceholderId = {};
	using ref_t = std::add_lvalue_reference_t<
		std::underlying_type_t<decltype(LastPlaceholderId)>>;
	reinterpret_cast<ref_t>(LastPlaceholderId) += 1;
	return LastPlaceholderId;
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
	for(auto s : GetRunnerSubsystemsWarn(self, TEXT("InitComponent"))) {
		s->InitComponentRaw(entity_id, component_id, component_data);
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
	for(auto s : GetRunnerSubsystemsWarn(self, TEXT("UpdateComponent"))) {
		s->UpdateComponentRaw(entity_id, component_id, component_data);
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
	for(auto s : GetRunnerSubsystemsWarn(self, TEXT("RemoveComponent"))) {
		s->RemoveComponentRaw(entity_id, component_id, component_data);
	}
}

auto UEcsactRunner::OnEntityCreatedRaw(
	ecsact_event                 event,
	ecsact_entity_id             entity_id,
	ecsact_placeholder_entity_id placeholder_entity_id,
	void*                        callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);

	auto create_callback =
		self->CreateEntityCallbacks.Find(placeholder_entity_id);
	if(create_callback) {
		create_callback->Execute(entity_id);
		self->CreateEntityCallbacks.Remove(placeholder_entity_id);
	} else if((int32)placeholder_entity_id > 0) {
		UE_LOG(
			Ecsact,
			Error,
			TEXT("Unable to find create entity callback for placeholder %i"),
			(int32)placeholder_entity_id
		);
	}

	for(auto s : GetRunnerSubsystemsWarn(self, TEXT("EntityCreated"))) {
		s->EntityCreated(static_cast<int32>(entity_id));
	}
}

auto UEcsactRunner::OnEntityDestroyedRaw(
	ecsact_event                 event,
	ecsact_entity_id             entity_id,
	ecsact_placeholder_entity_id placeholder_entity_id,
	void*                        callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);
	for(auto s : GetRunnerSubsystemsWarn(self, TEXT("EntityDestroyed"))) {
		s->EntityDestroyed(static_cast<int32>(entity_id));
	}
}

UEcsactRunner::EcsactRunnerCreateEntityBuilder::EcsactRunnerCreateEntityBuilder(
	UEcsactRunner*               Owner,
	ecsact_placeholder_entity_id PlaceholderId
)
	: Owner{Owner}
	, PlaceholderId{PlaceholderId}
	, Builder{Owner->ExecutionOptions->CreateEntity(PlaceholderId)} {
}

UEcsactRunner::EcsactRunnerCreateEntityBuilder::
	EcsactRunnerCreateEntityBuilder(EcsactRunnerCreateEntityBuilder&&) = default;

UEcsactRunner::EcsactRunnerCreateEntityBuilder::
	~EcsactRunnerCreateEntityBuilder() = default;

auto UEcsactRunner::EcsactRunnerCreateEntityBuilder::Finish() -> void {
	Builder.Finish();
}

auto UEcsactRunner::EcsactRunnerCreateEntityBuilder::OnCreate(
	TDelegate<void(ecsact_entity_id)> Callback
) && -> EcsactRunnerCreateEntityBuilder {
	Owner->CreateEntityCallbacks.Add(PlaceholderId, Callback);
	return std::move(*this);
}
