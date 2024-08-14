#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(Ecsact, Log, All);

class FEcsactModule : public IModuleInterface {
	friend class EcsactUnrealExecution;

	static FEcsactModule* Self;
	void*                 EcsactRuntimeHandle;
	class UEcsactRunner*  Runner;

	auto LoadEcsactRuntime() -> void;
	auto UnloadEcsactRuntime() -> void;
	auto Abort() -> void;
	auto OnPreBeginPIE(bool bIsSimulating) -> void;
	auto OnEndPIE(const bool bIsSimulating) -> void;

	auto StartRunner() -> void;
	auto StopRunner() -> void;

public:
	static auto Get() -> FEcsactModule&;

	auto StartupModule() -> void override;
	auto ShutdownModule() -> void override;
};
