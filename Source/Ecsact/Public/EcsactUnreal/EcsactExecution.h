#pragma once

#include "UObject/WeakObjectPtrTemplates.h"

class EcsactUnrealExecution {
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
	 *
	 */
	static auto Runner() -> TWeakObjectPtr<class UEcsactRunner>;
};
