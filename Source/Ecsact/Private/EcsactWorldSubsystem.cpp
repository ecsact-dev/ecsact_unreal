// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#include "EcsactWorldSubsystem.h"
#include "EcsactUnreal/Ecsact.h"
#include "EcsactUnreal/EcsactRunner.h"
#include "EcsactUnreal/EcsactSettings.h"
#include "EcsactUnreal/EcsactSyncRunner.h"
#include "EcsactUnreal/EcsactAsyncRunner.h"

auto UEcsactWorldSubsystem::ShouldCreateSubsystem( //
	UObject* Outer
) const -> bool {
	auto world = static_cast<UWorld*>(Outer);

	// We only care about game and PIE worlds for the runner
	switch(world->WorldType.GetValue()) {
		case EWorldType::None:
		case EWorldType::Editor:
		case EWorldType::EditorPreview:
		case EWorldType::GamePreview:
		case EWorldType::GameRPC:
		case EWorldType::Inactive:
			return false;
	}

	return true;
}

auto UEcsactWorldSubsystem::Initialize( //
	FSubsystemCollectionBase& Collection
) -> void {
}

auto UEcsactWorldSubsystem::Deinitialize() -> void {
}
