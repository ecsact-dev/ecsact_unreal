#include "EcsactUnreal/EcsactExecution.h"
#include "EcsactGameInstanceSubsystem.h"
#include "EcsactUnreal/Ecsact.h"
#include "EcsactGameInstanceSubsystem.h"
#include "Engine/GameInstance.h"

float EcsactUnrealExecution::DeltaTime_ = 0.f;

auto EcsactUnrealExecution::DeltaTime() -> float {
	return DeltaTime_;
}

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
