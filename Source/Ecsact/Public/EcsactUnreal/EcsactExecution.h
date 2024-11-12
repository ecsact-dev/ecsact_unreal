#pragma once

#include "EcsactUnreal/Ecsact.h"
#include "UObject/WeakObjectPtrTemplates.h"

class ECSACT_API EcsactUnrealExecution {
	friend class UEcsactSyncRunner;
	friend class UEcsactAsyncRunner;
	static float DeltaTime_;

public:
	/**
	 * Get the DeltaTime for the current EcsactRunner execution. This should be
	 * used by Ecsact systems that are built by unreal build tool.
	 *
	 * NOTE: This is a workaround until 'execution metadata' is available.
	 * SEE: https://github.com/ecsact-dev/ecsact_parse/issues/163
	 */
	static auto DeltaTime() -> float;

	/**
	 * Gets the runner for the given world.
	 *
	 * NOTE: Prefer using GetRunner() in your associated class. Most Ecsact
	 * classes will have this methods available. e.g. UEcsactRunnerSubsystem
	 */
	static auto Runner( //
		class UWorld* World
	) -> TWeakObjectPtr<class UEcsactRunner>;
};
