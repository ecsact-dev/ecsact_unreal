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

	/**
	 * Called when an ecsact_async_connect succeeds
	 * NOTE: For 'async' runners only
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Ecsact Runner")
	void AsyncConnect();

	virtual auto AsyncConnect_Implementation() -> void;

	/**
	 * Called when an ecsact_async_disconnect is called or if an error that
	 * triggers a disconnect occurs.
	 * NOTE: For 'async' runners only
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Ecsact Runner")
	void AsyncDisconnect();

	virtual auto AsyncDisconnect_Implementation() -> void;

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
