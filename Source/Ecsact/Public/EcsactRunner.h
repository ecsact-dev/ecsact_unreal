#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
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
};
