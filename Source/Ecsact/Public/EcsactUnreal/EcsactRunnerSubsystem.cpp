#include "EcsactRunnerSubsystem.h"

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
