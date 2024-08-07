#include "Ecsact.h"
#include "CoreGlobals.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "ecsact/runtime.h"

#define LOCTEXT_NAMESPACE "FEcsactModule"

DEFINE_LOG_CATEGORY(Ecsact);

#define INIT_ECSACT_API_FN(fn, UNUSED_PARAM) decltype(fn) fn = nullptr
FOR_EACH_ECSACT_API_FN(INIT_ECSACT_API_FN, UNUSED_PARAM);
#undef INIT_ECSACT_API_FN

auto FEcsactModule::Abort() -> void {
#ifdef WITH_EDITOR
	if(GEditor) {
		GEditor->EndPlayMap();
		return;
	}
#endif
}

auto FEcsactModule::LoadEcsactRuntime() -> void {
	auto ecsact_runtime_path = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Binaries/Win64/EcsactRuntime.dll")
	);

	EcsactRuntimeHandle = FPlatformProcess::GetDllHandle(*ecsact_runtime_path);

	if(!EcsactRuntimeHandle) {
		UE_LOG(
			Ecsact,
			Error,
			TEXT("Failed to load ecsact runtime: %s"),
			*ecsact_runtime_path
		);
		Abort();
		return;
	}

#define LOAD_ECSACT_FN(fn, UNUSED_PARAM)                           \
	fn = reinterpret_cast<decltype(fn)>(                             \
		FPlatformProcess::GetDllExport(EcsactRuntimeHandle, TEXT(#fn)) \
	)
	FOR_EACH_ECSACT_API_FN(LOAD_ECSACT_FN);
#undef LOAD_ECSACT_FN
}

auto FEcsactModule::UnloadEcsactRuntime() -> void {
	if(EcsactRuntimeHandle) {
		FPlatformProcess::FreeDllHandle(EcsactRuntimeHandle);
		EcsactRuntimeHandle = nullptr;
	}

#define RESET_ECSACT_FN(fn, UNUSED_PARAM) fn = nullptr
	FOR_EACH_ECSACT_API_FN(RESET_ECSACT_FN);
#undef RESET_ECSACT_FN
}

auto FEcsactModule::StartupModule() -> void {
	if(!GIsEditor) {
		LoadEcsactRuntime();
	}
#if WITH_EDITOR
	FEditorDelegates::PreBeginPIE.AddRaw(this, &FEcsactModule::OnPreBeginPIE);
	FEditorDelegates::EndPIE.AddRaw(this, &FEcsactModule::OnEndPIE);
#endif
}

auto FEcsactModule::ShutdownModule() -> void {
	if(!GIsEditor) {
		UnloadEcsactRuntime();
	}

#if WITH_EDITOR
	FEditorDelegates::PreBeginPIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);
#endif
}

auto FEcsactModule::OnPreBeginPIE(bool _) -> void {
	LoadEcsactRuntime();
}

auto FEcsactModule::OnEndPIE(bool _) -> void {
	UnloadEcsactRuntime();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FEcsactModule, Ecsact)
