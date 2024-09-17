#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "UObject/NoExportTypes.h"
#include "EcsactUnreal/EcsactRunner.h"
#include "ecsact/runtime/common.h"
#include "EcsactSyncRunner.generated.h"

UCLASS(NotBlueprintable)

class ECSACT_API UEcsactSyncRunner : public UEcsactRunner {
	GENERATED_BODY() // NOLINT
protected:
	auto StreamImpl(
		ecsact_entity_id    Entity,
		ecsact_component_id ComponentId,
		const void*         ComponentData
	) -> void override;

public:
	// TODO: Put this somewhere good.
	ecsact_registry_id registry_id = ECSACT_INVALID_ID(registry);

	auto Tick(float DeltaTime) -> void override;
	auto GetStatId() const -> TStatId override;
};
