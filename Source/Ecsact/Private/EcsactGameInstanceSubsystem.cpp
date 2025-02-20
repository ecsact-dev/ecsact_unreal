// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#include "EcsactGameInstanceSubsystem.h"
#include "EcsactUnreal/Ecsact.h"
#include "EcsactUnreal/RuntimeLoad.h"
#include "EcsactUnreal/EcsactRunner.h"
#include "EcsactUnreal/EcsactSettings.h"
#include "EcsactUnreal/EcsactSyncRunner.h"
#include "EcsactUnreal/EcsactAsyncRunner.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/CoreDelegates.h"

auto UEcsactGameInstanceSubsystem::Initialize( //
	FSubsystemCollectionBase& Collection
) -> void {
	UE_LOG(LogTemp, Log, TEXT("Ecsact Game Instance Initialize"));
	FWorldDelegates::OnPreWorldInitialization.AddUObject(
		this,
		&ThisClass::OnPreWorldInitialization
	);

	FWorldDelegates::OnPostWorldCleanup.AddUObject(
		this,
		&ThisClass::OnPostWorldCleanup
	);

	auto game_instance = GetGameInstance();

	auto world = game_instance->GetWorld();
	world->GetGameInstance();
}

auto UEcsactGameInstanceSubsystem::Deinitialize() -> void {
	auto runner = Runner.Get();
	if(runner) {
		StopRunner();
	}
}

auto UEcsactGameInstanceSubsystem::OnPreWorldInitialization(
	UWorld*                            World,
	const UWorld::InitializationValues IVS
) -> void {
	// We only care about game and PIE worlds for the runner
	switch(World->WorldType.GetValue()) {
		case EWorldType::None:
		case EWorldType::Editor:
		case EWorldType::EditorPreview:
		case EWorldType::GamePreview:
		case EWorldType::GameRPC:
		case EWorldType::Inactive:
			return;
	}

	auto game_instance = GetGameInstance();
	auto game_instance_world = game_instance->GetWorld();
	if(auto runner = Runner.Get(); runner) {
		if(game_instance_world == World) {
			auto old_world = runner->World;
			runner->World = World;
			runner->OnWorldChanged(old_world, World);
		}
	} else if(game_instance_world == World) {
		StartRunner(World);
	}
}

auto UEcsactGameInstanceSubsystem::OnPostWorldCleanup(
	UWorld* World,
	bool    bSessionEnded,
	bool    bCleanupResources
) -> void {
	auto runner = Runner.Get();

	if(runner && runner->GetWorld() == World) {
		// Runner world is transitioning or we're about to deinitialize
		// TODO: Should we notify the runner that this has started happening?
	}
}

auto UEcsactGameInstanceSubsystem::StartRunner(UWorld* World) -> void {
	checkSlow(!Runner.IsValid());
	checkSlow(GetGameInstance()->GetWorld() == World);

	const auto* settings = GetDefault<UEcsactSettings>();

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

	if(auto RunnerPtr = Runner.Get(); RunnerPtr) {
		UE_LOG(
			Ecsact,
			Log,
			TEXT("Starting ecsact runner: %s"),
			*RunnerPtr->GetClass()->GetName()
		);
		RunnerPtr->World = World;
		RunnerPtr->AddToRoot();
		RunnerPtr->Start();
	}
}

auto UEcsactGameInstanceSubsystem::StopRunner() -> void {
	checkSlow(Runner.IsValid());
	auto runner = Runner.Get();
	UE_LOG(
		Ecsact,
		Log,
		TEXT("Stopping ecsact runner"),
		*runner->GetClass()->GetName()
	);
	runner->Stop();
	Runner.Reset();
}

auto UEcsactGameInstanceSubsystem::CanStartCustomRunner() const -> bool {
	const auto* settings = GetDefault<UEcsactSettings>();
	if(settings->Runner == EEcsactRuntimeRunnerType::Custom) {
		return !settings->CustomRunnerClass;
	}

	return false;
}
