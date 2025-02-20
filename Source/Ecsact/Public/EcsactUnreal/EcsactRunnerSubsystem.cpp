// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#include "EcsactRunnerSubsystem.h"
#include "EcsactRunner.h"

auto UEcsactRunnerSubsystem::InitComponentRaw(
	ecsact_entity_id    EntityId,
	ecsact_component_id ComponentId,
	const void*         ComponentData
) -> void {
}

auto UEcsactRunnerSubsystem::UpdateComponentRaw(
	ecsact_entity_id    EntityId,
	ecsact_component_id ComponentId,
	const void*         ComponentData
) -> void {
}

auto UEcsactRunnerSubsystem::RemoveComponentRaw(
	ecsact_entity_id    EntityId,
	ecsact_component_id ComponentId,
	const void*         ComponentData
) -> void {
}

auto UEcsactRunnerSubsystem::GetRunner() -> class UEcsactRunner* {
	return OwningRunner;
}

auto UEcsactRunnerSubsystem::GetRunner() const -> const class UEcsactRunner* {
	return OwningRunner;
}

auto UEcsactRunnerSubsystem::GetWorld() const -> class UWorld* {
	if(OwningRunner != nullptr) {
		return OwningRunner->GetWorld();
	} else {
		return nullptr;
	}
}

auto UEcsactRunnerSubsystem::AsyncSessionEvent_Implementation( //
	EEcsactAsyncSessionEvent Event
) -> void {
}

auto UEcsactRunnerSubsystem::RunnerStart_Implementation(
	class UEcsactRunner* Runner
) -> void {
}

auto UEcsactRunnerSubsystem::RunnerStop_Implementation(
	class UEcsactRunner* Runner
) -> void {
}

auto UEcsactRunnerSubsystem::EntityCreated_Implementation( //
	int32 Entity
) -> void {
}

auto UEcsactRunnerSubsystem::EntityDestroyed_Implementation( //
	int32 Entity
) -> void {
}

auto UEcsactRunnerSubsystem::WorldChanged_Implementation( //
	UWorld* OldWorld,
	UWorld* NewWorld
) -> void {
}
