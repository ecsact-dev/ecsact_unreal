#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "EcsactUnreal/EcsactUnrealExecutionOptions.h"
#include "ecsact/runtime/common.h"
#include "EcsactRunner.generated.h"

UCLASS(Abstract)

class UEcsactRunner : public UObject, public FTickableGameObject {
	GENERATED_BODY() // NOLINT

	TArray<class UEcsactRunnerSubsystem*> RunnerSubsystems;
	ecsact_execution_events_collector     EventsCollector;

	static auto OnInitComponentRaw(
		ecsact_event        event,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id,
		const void*         component_data,
		void*               callback_user_data
	) -> void;

	static auto OnUpdateComponentRaw(
		ecsact_event        event,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id,
		const void*         component_data,
		void*               callback_user_data
	) -> void;

	static auto OnRemoveComponentRaw(
		ecsact_event        event,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id,
		const void*         component_data,
		void*               callback_user_data
	) -> void;

	static auto OnEntityCreatedRaw(
		ecsact_event                 event,
		ecsact_entity_id             entity_id,
		ecsact_placeholder_entity_id placeholder_entity_id,
		void*                        callback_user_data
	) -> void;

	static auto OnEntityDestroyedRaw(
		ecsact_event                 event,
		ecsact_entity_id             entity_id,
		ecsact_placeholder_entity_id placeholder_entity_id,
		void*                        callback_user_data
	) -> void;

protected:
	UPROPERTY()
	class UEcsactUnrealExecutionOptions* ExecutionOptions;

	auto GetEventsCollector() -> ecsact_execution_events_collector*;

	virtual auto InitRunnerSubsystems() -> void;
	virtual auto ShutdownRunnerSubsystems() -> void;

public:
	UEcsactRunner();

	auto Tick(float DeltaTime) -> void override;
	auto GetStatId() const -> TStatId override;
	auto IsTickable() const -> bool override;

	template<typename A>
	auto PushAction(const A& Action) -> void {
		return ExecutionOptions->PushAction<A>(Action);
	}

	template<typename C>
	auto AddComponent(ecsact_entity_id Entity, const C& Component) -> void {
		return ExecutionOptions->AddComponent<C>(Entity, Component);
	}

	template<typename C>
	auto UpdateComponent(ecsact_entity_id Entity, const C& Component) -> void {
		return ExecutionOptions->UpdateComponent<C>(Entity, Component);
	}

	template<typename C>
	auto RemoveComponent(ecsact_entity_id Entity) -> void {
		return ExecutionOptions->RemoveComponent<C>(Entity);
	}
};
