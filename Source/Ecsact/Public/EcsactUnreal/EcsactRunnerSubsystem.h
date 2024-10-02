#pragma once

#include "ecsact/runtime/common.h"
#include "EcsactRunnerSubsystem.generated.h"

UCLASS(Abstract, Blueprintable)

class ECSACT_API UEcsactRunnerSubsystem : public UObject {
	GENERATED_BODY() // NOLINT

	friend class UEcsactRunner;
	class UEcsactRunner* OwningRunner;

protected:
	virtual void InitComponentRaw(
		ecsact_entity_id    EntityId,
		ecsact_component_id ComponentId,
		const void*         ComponentData
	);
	virtual void UpdateComponentRaw(
		ecsact_entity_id    EntityId,
		ecsact_component_id ComponentId,
		const void*         ComponentData
	);
	virtual void RemoveComponentRaw(
		ecsact_entity_id    EntityId,
		ecsact_component_id ComponentId,
		const void*         ComponentData
	);

	auto GetRunner() -> class UEcsactRunner*;
	auto GetRunner() const -> const class UEcsactRunner*;

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Ecsact Runner")
	void RunnerStart(class UEcsactRunner* Runner);

	UFUNCTION(BlueprintNativeEvent, Category = "Ecsact Runner")
	void RunnerStop(class UEcsactRunner* Runner);

	virtual auto RunnerStart_Implementation( //
		class UEcsactRunner* Runner
	) -> void;
	virtual auto RunnerStop_Implementation( //
		class UEcsactRunner* Runner
	) -> void;

	UFUNCTION(BlueprintNativeEvent, Category = "Ecsact Runner")
	void EntityCreated(int32 Entity);

	UFUNCTION(BlueprintNativeEvent, Category = "Ecsact Runner")
	void EntityDestroyed(int32 Entity);

	virtual auto EntityCreated_Implementation( //
		int32 Entity
	) -> void;

	virtual auto EntityDestroyed_Implementation( //
		int32 Entity
	) -> void;
};
