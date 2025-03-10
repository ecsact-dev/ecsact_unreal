// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#include "EcsactUnreal/EcsactSyncRunner.h"
#include "EcsactUnreal/Ecsact.h"
#include "EcsactUnreal/EcsactUnrealExecutionOptions.h"
#include "EcsactUnreal/EcsactExecution.h"
#include "ecsact/runtime/core.h"
#include "ecsact/si/wasm.h"

UEcsactSyncRunner::UEcsactSyncRunner() : Super() {
}

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

	if(registry_id == ECSACT_INVALID_ID(registry)) {
		if(ecsact_create_registry) {
			UE_LOG(
				Ecsact,
				Warning,
				TEXT("UEcsactSyncRunner register_id is unset. Creating one for you. We "
						 "recommend creating your own instead.")
			);
			registry_id = ecsact_create_registry("Default Registry");
		} else {
			UE_LOG(
				Ecsact,
				Error,
				TEXT("UEcsactSyncRunner registry_id is unset and "
						 "ecsact_create_registry is unavailable - unable to automatically "
						 "create an Ecsact registry")
			);
		}
	}

	if(registry_id != ECSACT_INVALID_ID(registry)) {
		if(ecsact_execute_systems) {
			ecsact_execution_options* exec_opts = nullptr;
			if(ExecutionOptions != nullptr && ExecutionOptions->IsNotEmpty()) {
				exec_opts = ExecutionOptions->GetCPtr();
			}
			auto err = ecsact_execute_systems( //
				registry_id,
				1,
				exec_opts,
				GetEventsCollector()
			);
			if(err != ECSACT_EXEC_SYS_OK) {
				UE_LOG(Ecsact, Error, TEXT("Ecsact execution failed"));
			}
			if(ExecutionOptions != nullptr) {
				ExecutionOptions->Clear();
			}
		} else {
			UE_LOG(
				Ecsact,
				Error,
				TEXT("ecsact_execute_systems unavailable - unable to execute systems")
			);
		}
	}

	if(ecsact_si_wasm_consume_logs != nullptr) {
		ecsact_si_wasm_consume_logs(
			[](
				ecsact_si_wasm_log_level log_level,
				const char*              message,
				int32_t                  message_length,
				void*                    user_data
			) {
				switch(log_level) {
					default:
					case ECSACT_SI_WASM_LOG_LEVEL_INFO:
						UE_LOG(Ecsact, Log, TEXT("%.*hs"), message_length, message);
						break;
					case ECSACT_SI_WASM_LOG_LEVEL_WARNING:
						UE_LOG(Ecsact, Warning, TEXT("%.*hs"), message_length, message);
						break;
					case ECSACT_SI_WASM_LOG_LEVEL_ERROR:
						UE_LOG(Ecsact, Error, TEXT("%.*hs"), message_length, message);
						break;
				}
			},
			nullptr
		);
	}
}

auto UEcsactSyncRunner::GetStatId() const -> TStatId {
	RETURN_QUICK_DECLARE_CYCLE_STAT( // NOLINT
		UEcsactSyncRunner,
		STATGROUP_Tickables
	);
}
