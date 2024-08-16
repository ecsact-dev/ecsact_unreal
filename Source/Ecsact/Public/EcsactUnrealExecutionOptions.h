
#pragma once

#include "CoreMinimal.h"
#include "ecsact/runtime/common.h"
#include "EcsactUnrealExecutionOptions.generated.h"

UCLASS()

class UEcsactUnrealExecutionOptions : public UObject {
	GENERATED_BODY()

	TArray<ecsact_action>       ActionList;
	TArray<ecsact_component>    AddComponentList;
	TArray<ecsact_component>    UpdateComponentList;
	TArray<ecsact_component_id> RemoveComponentList;
	ecsact_execution_options    ExecOpts;

public:
	UEcsactUnrealExecutionOptions();

	auto Clear() -> void;
	auto IsNotEmpty() const -> bool;

	/**
	 * Get's the C `ecsact_execution_options` pointer typically passed to
	 * `ecsact_execute_systems` or `ecsact_async_enqueue_execution_options`. The
	 * lifetime of this pointer is the same as the owning
	 * `UEcsactUnrealExecutionOptions`.
	 */
	auto GetCPtr() -> ecsact_execution_options*;

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

	inline auto DestroyEntity(ecsact_entity_id Entity) -> void {
	}
};
