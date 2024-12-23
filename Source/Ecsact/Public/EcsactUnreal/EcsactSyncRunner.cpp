#include "EcsactUnreal/EcsactSyncRunner.h"
#include "EcsactUnreal/Ecsact.h"
#include "EcsactUnreal/EcsactUnrealExecutionOptions.h"
#include "EcsactUnreal/EcsactExecution.h"
#include "ecsact/runtime/core.h"

auto UEcsactSyncRunner::StreamImpl(
	ecsact_entity_id    Entity,
	ecsact_component_id ComponentId,
	const void*         ComponentData
) -> void {
	if(registry_id == ECSACT_INVALID_ID(registry)) {
		UE_LOG(Ecsact, Warning, TEXT("UEcsactSyncRunner register_id is unset."));
		return;
	}

	ecsact_stream(registry_id, Entity, ComponentId, ComponentData, nullptr);
}

auto UEcsactSyncRunner::Tick(float DeltaTime) -> void {
	if(ecsact_execute_systems == nullptr) {
		UE_LOG(Ecsact, Error, TEXT("ecsact_execute_systems is unavailable"));
		return;
	}

	EcsactUnrealExecution::DeltaTime_ = DeltaTime;

	if(registry_id == ECSACT_INVALID_ID(registry)) {
		UE_LOG(
			Ecsact,
			Warning,
			TEXT("UEcsactSyncRunner register_id is unset. Creating one for you. We "
					 "recommend creating your own instead.")
		);
		registry_id = ecsact_create_registry("Default Registry");
	}

	ecsact_execution_options* exec_opts = nullptr;
	if(ExecutionOptions != nullptr && ExecutionOptions->IsNotEmpty()) {
		exec_opts = ExecutionOptions->GetCPtr();
	}

	auto err =
		ecsact_execute_systems(registry_id, 1, exec_opts, GetEventsCollector());
	if(err != ECSACT_EXEC_SYS_OK) {
		UE_LOG(Ecsact, Error, TEXT("Ecsact execution failed"));
	}
}

auto UEcsactSyncRunner::GetStatId() const -> TStatId {
	RETURN_QUICK_DECLARE_CYCLE_STAT( // NOLINT
		UEcsactSyncRunner,
		STATGROUP_Tickables
	);
}
