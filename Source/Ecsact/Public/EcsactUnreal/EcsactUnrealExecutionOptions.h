
#pragma once

#include "CoreMinimal.h"
#include "ecsact/runtime/common.h"
#include "EcsactUnrealExecutionOptions.generated.h"

UCLASS()

class ECSACT_API UEcsactUnrealExecutionOptions : public UObject {
	GENERATED_BODY() // NOLINT

	TArray<ecsact_action>       ActionList;
	TArray<ecsact_component>    AddComponentList;
	TArray<ecsact_component>    UpdateComponentList;
	TArray<ecsact_component_id> RemoveComponentList;

	TArray<ecsact_placeholder_entity_id> CreateEntityList;
	TArray<TArray<ecsact_component>>     CreateEntityComponentsList;

	TArray<ecsact_component*> CreateEntityComponentsListData;
	TArray<int>               CreateEntityComponentsListNums;

	TArray<ecsact_entity_id> DestroyEntityList;

	ecsact_execution_options ExecOpts;

public:
	class CreateEntityBuilder;
	friend CreateEntityBuilder;

	UEcsactUnrealExecutionOptions();

	auto Clear() -> void;
	auto IsNotEmpty() const -> bool;

#ifdef WITH_EDITORONLY_DATA
	auto DebugLog() const -> void;
#endif

	/**
	 * Get's the C `ecsact_execution_options` pointer typically passed to
	 * `ecsact_execute_systems` or `ecsact_async_enqueue_execution_options`. The
	 * lifetime of this pointer is the same as the owning
	 * `UEcsactUnrealExecutionOptions`.
	 */
	auto GetCPtr() -> ecsact_execution_options*;

	/**
	 * Create an entity. You may give a placeholder ID that will later be used in
	 * the events collector create entity event.
	 */
	auto CreateEntity( //
		ecsact_placeholder_entity_id PlaceholderEntityId
	) -> CreateEntityBuilder;

	inline auto DestroyEntity(ecsact_entity_id Entity) -> void {
		DestroyEntityList.Add(Entity);
	}

	template<typename A>
	auto PushAction(const A& Action) -> void {
		auto action_data = FMemory::Malloc(sizeof(A));
		FMemory::Memcpy(action_data, &Action, sizeof(A));
		ActionList.Push(ecsact_action{
			.action_id = A::id,
			.action_data = action_data,
		});

		ExecOpts.actions_length = ActionList.Num();
		ExecOpts.actions = ActionList.GetData();
	}

	template<typename C>
	auto AddComponent(ecsact_entity_id Entity, const C& Component) -> void {
		auto component_data = FMemory::Malloc(sizeof(C));
		FMemory::Memcpy(component_data, &Component, sizeof(C));
		AddComponentList.Push(ecsact_component{
			.component_id = C::id,
			.component_data = component_data,
		});

		ExecOpts.add_components_length = AddComponentList.Num();
		ExecOpts.add_components = AddComponentList.GetData();
	}

	template<typename C>
	auto UpdateComponent(ecsact_entity_id Entity, const C& Component) -> void {
		auto component_data = FMemory::Malloc(sizeof(C));
		FMemory::Memcpy(component_data, &Component, sizeof(C));
		UpdateComponentList.Push(ecsact_component{
			.component_id = C::id,
			.component_data = component_data,
		});

		ExecOpts.update_components_length = UpdateComponentList.Num();
		ExecOpts.update_components = UpdateComponentList.GetData();
	}

	template<typename C>
	auto RemoveComponent(ecsact_entity_id Entity) -> void {
		RemoveComponentList.Push(C::id);

		ExecOpts.remove_components_length = RemoveComponentList.Num();
		ExecOpts.remove_components = RemoveComponentList.GetData();
	}
};

class ECSACT_API UEcsactUnrealExecutionOptions::CreateEntityBuilder {
	friend class UEcsactUnrealExecutionOptions;
	bool                           bValid;
	UEcsactUnrealExecutionOptions* Owner;
	ecsact_placeholder_entity_id   PlaceholderId;
	TArray<ecsact_component>       ComponentList;

	CreateEntityBuilder(
		UEcsactUnrealExecutionOptions* Owner,
		ecsact_placeholder_entity_id   PlacerholderId
	);

public:
	CreateEntityBuilder(CreateEntityBuilder&&);
	~CreateEntityBuilder();

	auto operator=(CreateEntityBuilder&&) -> CreateEntityBuilder&;

	template<typename C>
	auto AddComponent(const C& Component) && -> CreateEntityBuilder {
		auto component_data = FMemory::Malloc(sizeof(C));
		FMemory::Memcpy(component_data, &Component, sizeof(C));
		ComponentList.Push(ecsact_component{
			.component_id = C::id,
			.component_data = component_data,
		});
		return std::move(*this);
	}

	/**
	 * This is automatically called by the destructor, but can be called manually
	 * to 'finish' building your entity.
	 */
	auto Finish() -> void;
};
