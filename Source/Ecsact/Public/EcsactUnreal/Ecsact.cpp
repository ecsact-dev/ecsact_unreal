// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#include "Ecsact.h"
#include "CoreGlobals.h"
#include "EcsactUnreal/EcsactSettings.h"
#include "Engine/World.h"
#include "HAL/PlatformProcess.h"
#include "Logging/LogVerbosity.h"
#include "Misc/Paths.h"
#include "EcsactUnreal/EcsactAsyncRunner.h"
#include "ecsact/runtime.h"
#include "ecsact/si/wasm.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "FEcsactModule"

DEFINE_LOG_CATEGORY(Ecsact);

#define INIT_ECSACT_API_FN(fn, UNUSED_PARAM) decltype(fn) fn = nullptr
FOR_EACH_ECSACT_API_FN(INIT_ECSACT_API_FN, UNUSED_PARAM);
FOR_EACH_ECSACT_SI_WASM_API_FN(INIT_ECSACT_API_FN, UNUSED_PARAM);
#undef INIT_ECSACT_API_FN

FEcsactModule* FEcsactModule::Self = nullptr;

auto FEcsactModule::Get() -> FEcsactModule& {
	if(GIsEditor) {
		return FModuleManager::Get().GetModuleChecked<FEcsactModule>("Ecsact");
	} else {
		if(Self == nullptr) {
			Self = FModuleManager::Get().GetModulePtr<FEcsactModule>("Ecsact");
		}
		check(Self != nullptr);
		return *Self;
	}
}

auto FEcsactModule::Abort() -> void {
#if WITH_EDITOR
	if(GEditor) {
		GEditor->EndPlayMap();
		return;
	}
#endif
}

auto FEcsactModule::StartupModule() -> void {
	UE_LOG(Ecsact, Log, TEXT("Ecsact Module Startup"));
	Self = this;
}

auto FEcsactModule::ShutdownModule() -> void {
	UE_LOG(Ecsact, Log, TEXT("Ecsact Module Shutdown"));
	Self = nullptr;
}

auto FEcsactModule::SetRuntimeHandle( //
	const FEcsactRuntimeHandle& Handle
) -> void {
	check(!EcsactRuntimeHandle);
	EcsactRuntimeHandle = Handle.DllHandle;
}

auto FEcsactModule::FreeRuntimeHandle(FEcsactRuntimeHandle& Handle) -> void {
	if(EcsactRuntimeHandle) {
		check(EcsactRuntimeHandle == Handle.DllHandle);
		// NOTE: Freeing the ecsact runtime causes unreal editor to crash
		// FPlatformProcess::FreeDllHandle(Handle.DllHandle);
		Handle.DllHandle = nullptr;
	}
}

auto FEcsactModule::IsSameRuntimeHandle( //
	const FEcsactRuntimeHandle& Handle
) -> bool {
	return EcsactRuntimeHandle == Handle.DllHandle;
}

auto EcsactUnreal::Detail::GetDefaultRuntimeDllHandle() -> void* {
	const auto* settings = GetDefault<UEcsactSettings>();

	auto runtime_path = settings->GetEcsactRuntimeLibraryPath();
	auto dll_handle = FPlatformProcess::GetDllHandle(*runtime_path);
	if(!dll_handle) {
		UE_LOG(
			Ecsact,
			Error,
			TEXT("Failed to load ecsact runtime: %s"),
			*runtime_path
		);
	}
	return dll_handle;
}

auto EcsactUnreal::Detail::CheckRuntimeNotLoaded( //
	FEcsactModule& Module
) -> bool {
	if(Module.EcsactRuntimeHandle) {
		UE_LOG(
			Ecsact,
			Error,
			TEXT("Ecsact runtime already loaded. Cannot load secondary ecsact "
					 "runtime..")
		);
		return false;
	}

	return true;
}

auto EcsactUnreal::Detail::CheckRuntimeHandle( //
	FEcsactModule&              Module,
	const FEcsactRuntimeHandle& Handle
) -> bool {
	if(!Handle) {
		return false;
	}

	Module.SetRuntimeHandle(Handle);

	return true;
}

auto EcsactUnreal::Detail::CheckUnloadable(
	FEcsactModule&              Module,
	const FEcsactRuntimeHandle& Handle
) -> bool {
	if(!Handle) {
		UE_LOG(
			Ecsact,
			Error,
			TEXT("Cannot unload already unloaded ecsact runtime handle")
		);
		return false;
	}
	if(!Module.IsSameRuntimeHandle({}) && !Module.IsSameRuntimeHandle(Handle)) {
		UE_LOG(Ecsact, Error, TEXT("Cannot unload old ecsact runtime handle"));
		return false;
	}

	return true;
}

auto EcsactUnreal::Detail::UnloadPostDisconnect(
	FEcsactModule&        Module,
	FEcsactRuntimeHandle& Handle
) -> void {
	check(Module.IsSameRuntimeHandle({}) || Module.IsSameRuntimeHandle(Handle));
}

auto EcsactUnreal::Detail::UnloadPostReset(
	FEcsactModule&        Module,
	FEcsactRuntimeHandle& Handle
) -> void {
	check(Module.IsSameRuntimeHandle({}) || Module.IsSameRuntimeHandle(Handle));
	Module.FreeRuntimeHandle(Handle);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FEcsactModule, Ecsact)
