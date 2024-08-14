#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "UObject/NoExportTypes.h"
#include "EcsactRunner.h"
#include "EcsactSyncRunner.generated.h"

UCLASS(NotBlueprintable)

class UEcsactSyncRunner : public UEcsactRunner {
	GENERATED_BODY()
public:
	auto Tick(float DeltaTime) -> void override;
};
