#pragma once

#include "CoreMinimal.h"
#include "EcsactRunner.generated.h"

UCLASS(Abstract)

class UEcsactRunner : public UObject, public FTickableGameObject {
	GENERATED_BODY()

protected:
	UPROPERTY()
	class UEcsactUnrealEventsCollector* EventsCollector;

public:
	auto Tick(float DeltaTime) -> void override;
	auto GetStatId() const -> TStatId override;
	auto IsTickable() const -> bool override;
};
