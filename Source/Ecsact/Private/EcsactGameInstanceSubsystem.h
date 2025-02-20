// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#pragma once

#include "EcsactUnreal/Ecsact.h"
#include "EcsactUnreal/EcsactRunner.h"
#include "Engine/World.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EcsactUnreal/RuntimeHandle.h"
#include "EcsactGameInstanceSubsystem.generated.h"

UCLASS()

class ECSACT_API UEcsactGameInstanceSubsystem : public UGameInstanceSubsystem {
	GENERATED_BODY() // NOLINT

	friend class EcsactUnrealExecution;

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

	/**
	 * @returns true if runtime runner type is 'Custom' and the custom runner
	 * class is `None`.
	 */
	auto CanStartCustomRunner() const -> bool;

	/**
	 * Instantiates and starts a custom runner of type @t RunnerT. May only be
	 * called `CanStartCustomRunner()` returns `true.
	 */
	template<typename RunnerT>
	auto StartCustomRunner() -> void {
		if(Runner.IsValid()) {
			UE_LOG(
				Ecsact,
				Error,
				TEXT("An Ecsact runner has already started - only one runner may be "
						 "running per game instance world.")
			);
			return;
		}

		auto world = GetWorld();
		check(world);
		if(!CanStartCustomRunner()) {
			UE_LOG(
				Ecsact,
				Error,
				TEXT("Cannot start custom ecsact runner - please make sure runner type "
						 "is set to 'Custom' and the custom runner class is set to 'None' "
						 "in the unreal Ecsact plugin settings.")
			);
			return;
		}

		Runner = NewObject<RunnerT>();

		if(auto RunnerPtr = Runner.Get(); RunnerPtr) {
			UE_LOG(
				Ecsact,
				Log,
				TEXT("Starting custom ecsact runner: %s"),
				*RunnerPtr->GetClass()->GetName()
			);
			RunnerPtr->World = world;
			RunnerPtr->AddToRoot();
			RunnerPtr->Start();
		}
	}
};
