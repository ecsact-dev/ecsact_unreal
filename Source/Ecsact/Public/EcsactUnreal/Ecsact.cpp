#include "Ecsact.h"
#include "CoreGlobals.h"
#include "EcsactUnreal/EcsactSettings.h"
#include "Engine/World.h"
#include "HAL/PlatformProcess.h"
#include "Logging/LogVerbosity.h"
#include "Misc/Paths.h"
#include "EcsactUnreal/EcsactAsyncRunner.h"
#include "ecsact/runtime.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "FEcsactModule"

DEFINE_LOG_CATEGORY(Ecsact);

#define INIT_ECSACT_API_FN(fn, UNUSED_PARAM) decltype(fn) fn = nullptr
FOR_EACH_ECSACT_API_FN(INIT_ECSACT_API_FN, UNUSED_PARAM);
#undef INIT_ECSACT_API_FN

FEcsactModule* FEcsactModule::Self = nullptr;

auto FEcsactModule::Get() -> FEcsactModule& {
	if(GIsEditor) {
		return FModuleManager::Get().GetModuleChecked<FEcsactModule>("Ecsact");
	} else {
		check(Self != nullptr);
		return *Self;
	}
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
	const auto* settings = GetDefault<UEcsactSettings>();
	auto        ecsact_runtime_path = settings->GetEcsactRuntimeLibraryPath();
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
}

auto FEcsactModule::UnloadEcsactRuntime() -> void {
	UE_LOG(Ecsact, Log, TEXT("Unloading ecsact runtime"));

	if(ecsact_async_disconnect != nullptr) {
		ecsact_async_disconnect();
	}

	for(auto i = 0; RunnerWorlds.Num() > i; ++i) {
		StopRunner(i);
	}

#define RESET_ECSACT_FN(fn, UNUSED_PARAM) fn = nullptr
	FOR_EACH_ECSACT_API_FN(RESET_ECSACT_FN);
#undef RESET_ECSACT_FN

	if(EcsactRuntimeHandle) {
		// NOTE: Freeing the ecsact runtime causes unreal editor to crash
		// FPlatformProcess::FreeDllHandle(EcsactRuntimeHandle);
		EcsactRuntimeHandle = nullptr;
	}
}

auto FEcsactModule::StartupModule() -> void {
	UE_LOG(Ecsact, Warning, TEXT("Ecsact Startup Module"));
	Self = this;
	if(!GIsEditor) {
		LoadEcsactRuntime();
	}
#if WITH_EDITOR
	FEditorDelegates::PreBeginPIE.AddRaw(this, &FEcsactModule::OnPreBeginPIE);
	FEditorDelegates::ShutdownPIE.AddRaw(this, &FEcsactModule::OnShutdownPIE);
#endif
	FWorldDelegates::OnPreWorldInitialization.AddRaw(
		this,
		&FEcsactModule::OnPreWorldInitialization
	);
	FWorldDelegates::OnPostWorldCleanup.AddRaw(
		this,
		&FEcsactModule::OnPostWorldCleanup
	);
}

auto FEcsactModule::OnPreWorldInitialization( //
	UWorld*                            World,
	const UWorld::InitializationValues IVS
) -> void {
	switch(World->WorldType.GetValue()) {
		case EWorldType::None:
		case EWorldType::Editor:
		case EWorldType::EditorPreview:
		case EWorldType::GamePreview:
		case EWorldType::GameRPC:
		case EWorldType::Inactive:
			return;
		case EWorldType::PIE:
		case EWorldType::Game:
			break;
	}

	auto index = RunnerWorlds.Num();
	RunnerWorlds.Add(World);
	Runners.AddDefaulted();
	check(RunnerWorlds.Num() == Runners.Num());
	StartRunner(index);
}

auto FEcsactModule::OnPostWorldCleanup( //
	UWorld* World,
	bool    bSessionEnded,
	bool    bCleanupResources
) -> void {
	for(auto i = 0; RunnerWorlds.Num() > i; ++i) {
		if(RunnerWorlds[i].Get() == World) {
			StopRunner(i);
			break;
		}
	}
}

auto FEcsactModule::ShutdownModule() -> void {
	if(!GIsEditor) {
		UnloadEcsactRuntime();
	}

#if WITH_EDITOR
	FEditorDelegates::PreBeginPIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);
#endif
	UE_LOG(Ecsact, Warning, TEXT("Ecsact Shutdown Module"));
	Self = nullptr;
}

auto FEcsactModule::OnPreBeginPIE(bool _) -> void {
	LoadEcsactRuntime();
}

auto FEcsactModule::OnShutdownPIE(bool _) -> void {
	UnloadEcsactRuntime();
}

auto FEcsactModule::StartRunner(int32 Index) -> void {
	const auto* settings = GetDefault<UEcsactSettings>();

	if(Index >= Runners.Num()) {
		UE_LOG(
			Ecsact,
			Error,
			TEXT("StartRunner() was called before the associated world was "
					 "initialized")
		);
		return;
	}

	for(auto Runner : Runners) {
		if(Runner.IsValid()) {
			UE_LOG(
				Ecsact,
				Error,
				TEXT("Multiple Ecsact runners are not supported at this time")
			);
			return;
		}
	}

	if(auto Runner = Runners[Index].Get(); Runner) {
		UE_LOG(
			Ecsact,
			Warning,
			TEXT("StartRunner() was called while runner was already running. "
					 "Stopping previous one before starting new.")
		);
		StopRunner(Index);
	}

	auto World = RunnerWorlds[Index].Get();

	if(!World) {
		UE_LOG(Ecsact, Error, TEXT("StartRunner() was called on invalid world"));
		return;
	}

	switch(settings->Runner) {
		case EEcsactRuntimeRunnerType::Automatic:
			if(ecsact_async_flush_events == nullptr) {
				Runners[Index] = NewObject<UEcsactSyncRunner>();
			} else {
				Runners[Index] = NewObject<UEcsactAsyncRunner>();
			}
			break;
		case EEcsactRuntimeRunnerType::Asynchronous:
			Runners[Index] = NewObject<UEcsactAsyncRunner>();
			break;
		case EEcsactRuntimeRunnerType::Custom:
			if(settings->CustomRunnerClass != nullptr) {
				Runners[Index] =
					NewObject<UEcsactRunner>(nullptr, settings->CustomRunnerClass);
			}
			break;
	}

	if(auto Runner = Runners[Index].Get(); Runner) {
		UE_LOG(
			Ecsact,
			Log,
			TEXT("Starting ecsact runner: %s"),
			*Runner->GetClass()->GetName()
		);
		Runner->World = World;
		Runner->AddToRoot();
		Runner->Start();
	}
}

auto FEcsactModule::StopRunner(int32 Index) -> void {
	if(Index >= Runners.Num()) {
		return;
	}

	auto Runner = Runners[Index].Get();

	if(Runner) {
		UE_LOG(
			Ecsact,
			Log,
			TEXT("Stopping ecsact runner: %s"),
			*Runner->GetClass()->GetName()
		);
		Runner->Stop();
		Runner->RemoveFromRoot();
		Runner->MarkAsGarbage();
		Runners[Index].Reset();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FEcsactModule, Ecsact)
