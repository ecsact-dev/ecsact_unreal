#include "EcsactUnreal/EcsactGameModuleImpl.h"
#include "EcsactUnreal/Ecsact.h"
#include "EcsactUnreal/RuntimeLoad.h"

auto FEcsactGameModuleImpl::StartupModule() -> void {
#if WITH_EDITOR
	FEditorDelegates::PreBeginPIE.AddRaw(
		this,
		&FEcsactGameModuleImpl::OnPreBeginPIE
	);
	FEditorDelegates::ShutdownPIE.AddRaw(
		this,
		&FEcsactGameModuleImpl::OnShutdownPIE
	);
	if(IsRunningGame()) {
		RuntimeHandle = ECSACT_LOAD_RUNTIME();
	}
#else
	RuntimeHandle = ECSACT_LOAD_RUNTIME();
#endif
}

auto FEcsactGameModuleImpl::ShutdownModule() -> void {
	if(RuntimeHandle) {
		ECSACT_UNLOAD_RUNTIME(RuntimeHandle);
	}
}

#if WITH_EDITOR
auto FEcsactGameModuleImpl::OnPreBeginPIE(bool bIsSimulating) -> void {
	RuntimeHandle = ECSACT_LOAD_RUNTIME();
}

auto FEcsactGameModuleImpl::OnShutdownPIE(const bool bIsSimulating) -> void {
	if(RuntimeHandle) {
		ECSACT_UNLOAD_RUNTIME(RuntimeHandle);
	}
}
#endif
