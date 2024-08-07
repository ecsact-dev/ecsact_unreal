#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(Ecsact, Log, All);

class FEcsactModule : public IModuleInterface {
	void* EcsactRuntimeHandle;
	auto  LoadEcsactRuntime() -> void;
	auto  UnloadEcsactRuntime() -> void;
	auto  Abort() -> void;
	auto  OnPreBeginPIE(bool bIsSimulating) -> void;
	auto  OnEndPIE(const bool bIsSimulating) -> void;

public:
	auto StartupModule() -> void override;
	auto ShutdownModule() -> void override;
};
