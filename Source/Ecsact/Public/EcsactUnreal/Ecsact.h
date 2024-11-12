#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Modules/ModuleManager.h"
#include "UObject/WeakObjectPtr.h"

DECLARE_LOG_CATEGORY_EXTERN(Ecsact, Log, All);

class FEcsactModule : public IModuleInterface {
	friend class EcsactUnrealExecution;

	static FEcsactModule* Self;
	void*                 EcsactRuntimeHandle;

	TArray<TWeakObjectPtr<UWorld>>              RunnerWorlds;
	TArray<TWeakObjectPtr<class UEcsactRunner>> Runners;

	auto LoadEcsactRuntime() -> void;
	auto UnloadEcsactRuntime() -> void;
	auto Abort() -> void;
	auto OnPreBeginPIE(bool bIsSimulating) -> void;
	auto OnShutdownPIE(const bool bIsSimulating) -> void;

	auto OnPreWorldInitialization( //
		UWorld*                            World,
		const UWorld::InitializationValues IVS
	) -> void;

	auto OnPostWorldCleanup( //
		UWorld* World,
		bool    bSessionEnded,
		bool    bCleanupResources
	) -> void;

	auto StartRunner(int32 Index) -> void;
	auto StopRunner(int32 Index) -> void;

public:
	static auto Get() -> FEcsactModule&;

	auto StartupModule() -> void override;
	auto ShutdownModule() -> void override;
};
