// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#include "EcsactUnreal/EcsactExecution.h"
#include "EcsactGameInstanceSubsystem.h"
#include "EcsactUnreal/Ecsact.h"
#include "EcsactGameInstanceSubsystem.h"
#include "Engine/GameInstance.h"

auto EcsactUnrealExecution::Runner( //
	class UWorld* World
) -> TWeakObjectPtr<class UEcsactRunner> {
	check(World);
	auto game_instance = World->GetGameInstance();
	if(!game_instance) {
		return nullptr;
	}
	auto subsystem = game_instance->GetSubsystem<UEcsactGameInstanceSubsystem>();
	if(!subsystem) {
		return nullptr;
	}

	return subsystem->Runner;
}

auto EcsactUnrealExecution::RunnerOrWarn( //
	class UWorld* World
) -> TWeakObjectPtr<class UEcsactRunner> {
	if(!World) {
		UE_LOG(Ecsact, Error, TEXT("RunnerOrWarn: World is invalid"));
		return {};
	}

	auto runner = Runner(World).Get();
	if(!runner) {
		static bool AlreadyWarned = false;
		if(!AlreadyWarned) {
			AlreadyWarned = true;
			UE_LOG(
				Ecsact,
				Warning,
				TEXT("RunnerOrWarn: Runner is not available. Please make sure the "
						 "ecsact runtime is loaded.")
			);
		}
		return {};
	}

	return runner;
}
