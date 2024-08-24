#include "Ecsact.h"
#include "CoreGlobals.h"
#include "EcsactUnreal/EcsactSettings.h"
#include "HAL/PlatformProcess.h"
#include "Logging/LogVerbosity.h"
#include "Misc/Paths.h"
#include "EcsactUnreal/EcsactAsyncRunner.h"
#include "ecsact/runtime.h"

#define LOCTEXT_NAMESPACE "FEcsactModule"

DEFINE_LOG_CATEGORY(Ecsact);

#define INIT_ECSACT_API_FN(fn, UNUSED_PARAM) decltype(fn) fn = nullptr
FOR_EACH_ECSACT_API_FN(INIT_ECSACT_API_FN, UNUSED_PARAM);
#undef INIT_ECSACT_API_FN

FEcsactModule* FEcsactModule::Self = nullptr;

auto FEcsactModule::Get() -> FEcsactModule& {
	check(Self != nullptr);
	return *Self;
}

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
	UE_LOG(Ecsact, Log, TEXT("Loading ecsact runtime %s"), *ecsact_runtime_path);

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
	);                                                               \
	if(fn != nullptr) {                                              \
		UE_LOG(Ecsact, Log, TEXT("loaded %s"), TEXT(#fn));             \
	}                                                                \
	static_assert(true, "require ;")
	FOR_EACH_ECSACT_API_FN(LOAD_ECSACT_FN);
#undef LOAD_ECSACT_FN
	StartRunner();
}

auto FEcsactModule::UnloadEcsactRuntime() -> void {
	UE_LOG(Ecsact, Log, TEXT("Unloading ecsact runtime"));

	if(ecsact_async_disconnect != nullptr) {
		ecsact_async_disconnect();
	}

	StopRunner();

#define RESET_ECSACT_FN(fn, UNUSED_PARAM) fn = nullptr
	FOR_EACH_ECSACT_API_FN(RESET_ECSACT_FN);
#undef RESET_ECSACT_FN

	if(EcsactRuntimeHandle) {
		FPlatformProcess::FreeDllHandle(EcsactRuntimeHandle);
		EcsactRuntimeHandle = nullptr;
	}
}

auto FEcsactModule::StartupModule() -> void {
	Self = this;
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
	Self = nullptr;
}

auto FEcsactModule::OnPreBeginPIE(bool _) -> void {
	LoadEcsactRuntime();
}

auto FEcsactModule::OnEndPIE(bool _) -> void {
	UnloadEcsactRuntime();
}

auto FEcsactModule::StartRunner() -> void {
	const auto* settings = GetDefault<UEcsactSettings>();

	if(Runner != nullptr) {
		UE_LOG(
			Ecsact,
			Warning,
			TEXT("StartRunner() was called while runner was already running. "
					 "Stopping previous one before starting new.")
		);
		StopRunner();
	}

	switch(settings->Runner) {
		case EEcsactRuntimeRunnerType::Automatic:
			if(ecsact_async_flush_events == nullptr) {
				Runner = NewObject<UEcsactSyncRunner>();
			} else {
				Runner = NewObject<UEcsactAsyncRunner>();
			}
			break;
		case EEcsactRuntimeRunnerType::Asynchronous:
			Runner = NewObject<UEcsactAsyncRunner>();
			break;
		case EEcsactRuntimeRunnerType::Custom:
			if(settings->CustomRunnerClass != nullptr) {
				Runner = NewObject<UEcsactRunner>(nullptr, settings->CustomRunnerClass);
			}
			break;
	}

	if(Runner != nullptr) {
		UE_LOG(
			Ecsact,
			Log,
			TEXT("Using ecsact runner: %s"),
			*Runner->GetClass()->GetName()
		);
		Runner->AddToRoot();
	}
}

auto FEcsactModule::StopRunner() -> void {
	if(Runner != nullptr) {
		Runner->RemoveFromRoot();
		Runner = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FEcsactModule, Ecsact)
