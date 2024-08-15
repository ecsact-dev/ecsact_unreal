#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "UObject/NoExportTypes.h"
#include "EcsactRunner.h"
#include "ecsact/runtime/common.h"
#include "EcsactSyncRunner.generated.h"

UCLASS(NotBlueprintable)

class UEcsactSyncRunner : public UEcsactRunner {
	GENERATED_BODY()
public:
	// TODO: Put this somewhere good.
	ecsact_registry_id registry_id = ECSACT_INVALID_ID(registry);

	auto Tick(float DeltaTime) -> void override;
};