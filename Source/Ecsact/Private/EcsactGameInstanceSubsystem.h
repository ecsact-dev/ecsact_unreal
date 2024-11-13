#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "EcsactUnreal/RuntimeHandle.h"
#include "EcsactGameInstanceSubsystem.generated.h"

UCLASS()

class UEcsactGameInstanceSubsystem : public UGameInstanceSubsystem {
	GENERATED_BODY() // NOLINT

	friend class EcsactUnrealExecution;

	FEcsactRuntimeHandle                RuntimeHandle;
	TWeakObjectPtr<class UEcsactRunner> Runner;

	auto OnPreWorldInitialization(
		UWorld*                            World,
		const UWorld::InitializationValues IVS
	) -> void;
	auto OnPostWorldCleanup(
		UWorld* World,
		bool    bSessionEnded,
		bool    bCleanupResources
	) -> void;

	auto StartRunner(UWorld* World) -> void;
	auto StopRunner() -> void;

public:
	auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
	auto Deinitialize() -> void override;
};
