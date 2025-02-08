#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "EcsactUnreal/EcsactUnrealExecutionOptions.h"
#include "EcsactUnreal/EcsactRunnerSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "ecsact/runtime/common.h"
#include "EcsactRunner.generated.h"

UCLASS(Abstract)

class ECSACT_API UEcsactRunner : public UObject, public FTickableGameObject {
	GENERATED_BODY() // NOLINT

	friend class UEcsactWorldSubsystem;
	friend class UEcsactGameInstanceSubsystem;

	UWorld*                           World;
	ecsact_execution_events_collector EventsCollector;
	bool                              bIsStopped = false;

	FSubsystemCollection<class UEcsactRunnerSubsystem> RunnerSubsystems;

	TMap<ecsact_placeholder_entity_id, TDelegate<void(ecsact_entity_id)>>
		CreateEntityCallbacks;

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
	auto GetRunnerSubsystems() -> TArray<class UEcsactRunnerSubsystem*>;

protected:
	virtual auto GeneratePlaceholderId() -> ecsact_placeholder_entity_id;
	virtual auto StreamImpl(
		ecsact_entity_id    Entity,
		ecsact_component_id ComponentId,
		const void*         ComponentData
	) -> void;

public:
	class EcsactRunnerCreateEntityBuilder;

	UEcsactRunner();

	virtual auto Start() -> void;
	virtual auto Stop() -> void;
	virtual auto IsStopped() const -> bool;

	virtual auto OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld) -> void;

	UFUNCTION(BlueprintPure, Category = "Ecsact Runner")
	bool HasAsyncEvents() const;

	auto Tick(float DeltaTime) -> void override;
	auto GetStatId() const -> TStatId override;
	auto IsTickable() const -> bool override;
	auto GetWorld() const -> class UWorld* override;

	/**
	 * Get a Subsystem of specified type
	 */
	template<typename TSubsystemClass>
	auto GetSubsystem() const -> TSubsystemClass* {
		return RunnerSubsystems.GetSubsystem<TSubsystemClass>(
			TSubsystemClass::StaticClass()
		);
	}

	/**
	 * Check if runner has a subsystem of the specified type
	 */
	template<typename TSubsystemClass>
	auto HasSubsystem() const -> bool {
		return GetSubsystem<TSubsystemClass>() != nullptr;
	}

	/**
	 * Get all Subsystem of specified type, this is only necessary for interfaces
	 * that can have multiple implementations instanced at a time.
	 *
	 * Do not hold onto this Array reference unless you are sure the lifetime is
	 * less than that of UGameInstance
	 */
	template<typename TSubsystemClass>
	auto GetSubsystemArray() const -> const TArray<TSubsystemClass*>& {
		return RunnerSubsystems.GetSubsystemArray<TSubsystemClass>(
			TSubsystemClass::StaticClass()
		);
	}

	template<typename C>
	auto Stream(ecsact_entity_id Entity, const C& StreamComponent) -> void {
		return StreamImpl(Entity, C::id, &StreamComponent);
	}

	/**
	 * Returns a builder for creating a new entity. This builder is 'runner aware'
	 * meaning that any lifecycle hooks that the builder exposes is provided by
	 * the runners execution
	 */
	auto CreateEntity() -> EcsactRunnerCreateEntityBuilder;

	auto DestroyEntity(ecsact_entity_id Entity) -> void;

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

class ECSACT_API UEcsactRunner::EcsactRunnerCreateEntityBuilder {
	friend UEcsactRunner;

	UEcsactRunner*               Owner;
	ecsact_placeholder_entity_id PlaceholderId;

	UEcsactUnrealExecutionOptions::CreateEntityBuilder Builder;

	EcsactRunnerCreateEntityBuilder(
		UEcsactRunner*               Owner,
		ecsact_placeholder_entity_id PlacerholderId
	);

public:
	EcsactRunnerCreateEntityBuilder(EcsactRunnerCreateEntityBuilder&&);
	~EcsactRunnerCreateEntityBuilder();

	template<typename C>
	auto AddComponent( //
		const C& Component
	) && -> EcsactRunnerCreateEntityBuilder {
		using CreateEntityBuilder =
			UEcsactUnrealExecutionOptions::CreateEntityBuilder;
		Builder = std::move(Builder).AddComponent<C>(Component);
		return std::move(*this);
	}

	/**
	 * Listens for when the entity is created.
	 */
	auto OnCreate( //
		TDelegate<void(ecsact_entity_id)> Callback
	) && -> EcsactRunnerCreateEntityBuilder;

	/**
	 * This is automatically called by the destructor, but can be called manually
	 * to 'finish' building your entity.
	 */
	auto Finish() -> void;
};
