
#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "UObject/NoExportTypes.h"
#include "EcsactRunner.h"
#include "EcsactAsyncRunner.generated.h"

UCLASS(NotBlueprintable)

class UEcsactAsyncRunner : public UEcsactRunner {
	GENERATED_BODY()
public:
	UPROPERTY()
	class UEcsactUnrealEventsCollector* EventsCollector;

	auto Tick(float DeltaTime) -> void override;
};
