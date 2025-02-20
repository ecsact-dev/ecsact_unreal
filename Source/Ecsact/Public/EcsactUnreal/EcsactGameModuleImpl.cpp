// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#include "EcsactUnreal/EcsactGameModuleImpl.h"
#include "EcsactUnreal/Ecsact.h"
#include "EcsactUnreal/RuntimeLoad.h"
#include "EcsactUnreal/EcsactAsyncRunner.h"

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
		if(ecsact_async_force_reset) {
			ecsact_async_force_reset();
		} else {
			UE_LOG(
				Ecsact,
				Warning,
				TEXT("ecsact_async_force_reset was not available during shutdown")
			);
		}
		ECSACT_UNLOAD_RUNTIME(RuntimeHandle);
	}
}

#if WITH_EDITOR
auto FEcsactGameModuleImpl::OnPreBeginPIE(bool bIsSimulating) -> void {
	UE_LOG(Ecsact, Log, TEXT("OnPreBeginPIE"));
	RuntimeHandle = ECSACT_LOAD_RUNTIME();
}

auto FEcsactGameModuleImpl::OnShutdownPIE(const bool bIsSimulating) -> void {
	UE_LOG(Ecsact, Log, TEXT("OnShutdownPIE"));
	if(RuntimeHandle) {
		if(ecsact_async_force_reset) {
			ecsact_async_force_reset();
		} else {
			UE_LOG(
				Ecsact,
				Warning,
				TEXT("ecsact_async_force_reset was not available during PIE shutdown")
			);
		}
		ECSACT_UNLOAD_RUNTIME(RuntimeHandle);
	}
}
#endif
