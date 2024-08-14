#include "Ecsact.h"
#include "EcsactSyncRunner.h"
#include "EcsactExecution.h"
#include "ecsact/runtime/core.h"

auto UEcsactSyncRunner::Tick(float DeltaTime) -> void {
	EcsactUnrealExecution::DeltaTime_ = DeltaTime;
	auto err = ecsact_execute_systems({}, 1, nullptr, nullptr);
	if(err != ECSACT_EXEC_SYS_OK) {
		UE_LOG(Ecsact, Error, TEXT("Ecsact execution failed"));
	}
}
