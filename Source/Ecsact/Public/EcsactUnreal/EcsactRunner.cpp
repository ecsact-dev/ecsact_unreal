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
	return GEngine->GameViewport->GetWorld();
}

auto UEcsactRunner::GetRunnerSubsystems()
	-> TArray<class UEcsactRunnerSubsystem*> {
	return RunnerSubsystems;
}

auto UEcsactRunner::StreamImpl(
	ecsact_entity_id    Entity,
	ecsact_component_id ComponentId,
	const void*         ComponentData
) -> void {
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

auto UEcsactRunner::GetSubsystem( //
	UClass* SubsystemClass
) -> UEcsactRunnerSubsystem* {
	for(auto* subsystem : RunnerSubsystems) {
		if(subsystem->IsA(SubsystemClass)) {
			return subsystem;
		}
	}

	return nullptr;
}

auto UEcsactRunner::CreateEntity() -> EcsactRunnerCreateEntityBuilder {
	return {this, GeneratePlaceholderId()};
}

auto UEcsactRunner::GeneratePlaceholderId() -> ecsact_placeholder_entity_id {
	static ecsact_placeholder_entity_id LastPlaceholderId = {};
	using ref_t = std::add_lvalue_reference_t<
		std::underlying_type_t<decltype(LastPlaceholderId)>>;
	reinterpret_cast<ref_t>(LastPlaceholderId) += 1;
	return LastPlaceholderId;
}

auto UEcsactRunner::InitRunnerSubsystems() -> void {
	const auto* settings = GetDefault<UEcsactSettings>();

	check(RunnerSubsystems.IsEmpty());
	RunnerSubsystems.Empty();

	for(auto t : settings->RunnerSubsystems) {
		UE_LOG(Ecsact, Log, TEXT("Starting ecsact subsystem %s"), *t->GetName());
		auto subsystem = NewObject<UEcsactRunnerSubsystem>(this, t);
		subsystem->OwningRunner = this;
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

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Received entity created callback for %i"),
		static_cast<int>(entity_id)
	);

	auto create_callback =
		self->CreateEntityCallbacks.Find(placeholder_entity_id);
	if(create_callback) {
		create_callback->Execute(entity_id);
		self->CreateEntityCallbacks.Remove(placeholder_entity_id);
	} else {
		UE_LOG(
			Ecsact,
			Error,
			TEXT("Unable to find create entity callback for placeholder %i"),
			(int32)placeholder_entity_id
		);
	}

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
