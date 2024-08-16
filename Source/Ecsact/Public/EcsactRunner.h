#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "EcsactUnrealExecutionOptions.h"
#include "ecsact/runtime/common.h"
#include "EcsactRunner.generated.h"

UCLASS(Abstract)

class UEcsactRunner : public UObject, public FTickableGameObject {
	GENERATED_BODY()

protected:
	UPROPERTY()
	class UEcsactUnrealEventsCollector* EventsCollector;

	UPROPERTY()
	class UEcsactUnrealExecutionOptions* ExecutionOptions;

public:
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
