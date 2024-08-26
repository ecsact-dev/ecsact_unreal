#include "EcsactUnreal/EcsactRunner.h"
#include "EcsactUnreal/EcsactRunnerSubsystem.h"

auto UEcsactRunner::Tick(float DeltaTime) -> void {
}

auto UEcsactRunner::GetStatId() const -> TStatId {
	RETURN_QUICK_DECLARE_CYCLE_STAT( // NOLINT
		UEcsactRunner,
		STATGROUP_Tickables
	);
}

auto UEcsactRunner::IsTickable() const -> bool {
	return !IsTemplate();
}

auto UEcsactRunner::InitRunnerSubsystems() -> void {
}

auto UEcsactRunner::ShutdownRunnerSubsystems() -> void {
}
