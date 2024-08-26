#pragma once

#include "ecsact/runtime/common.h"
#include "EcsactRunnerSubsystem.generated.h"

UCLASS(Blueprintable)

class ECSACT_API UEcsactRunnerSubsystem : public UObject {
	GENERATED_BODY() // NOLINT
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
