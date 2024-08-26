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
};
