#pragma once

#include "CoreMinimal.h"
#include "ecsact/runtime/common.h"
#include "EcsactUnrealEventsCollector.generated.h"

UCLASS()

class UEcsactUnrealEventsCollector : public UObject {
	GENERATED_BODY()

	ecsact_execution_events_collector evc;

	static auto OnInitComponentRaw(
		ecsact_event        event,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id,
		const void*         component_data,
		void*               callback_user_data
	) -> void;

	static auto OnUpdateComponentRaw(
		ecsact_event        event,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id,
		const void*         component_data,
		void*               callback_user_data
	) -> void;

	static auto OnRemoveComponentRaw(
		ecsact_event        event,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id,
		const void*         component_data,
		void*               callback_user_data
	) -> void;

	static auto OnEntityCreatedRaw(
		ecsact_event                 event,
		ecsact_entity_id             entity_id,
		ecsact_placeholder_entity_id placeholder_entity_id,
		void*                        callback_user_data
	) -> void;

	static auto OnEntityDestroyedRaw(
		ecsact_event                 event,
		ecsact_entity_id             entity_id,
		ecsact_placeholder_entity_id placeholder_entity_id,
		void*                        callback_user_data
	) -> void;

public:
	DECLARE_MULTICAST_DELEGATE_ThreeParams( //
		FRawComponentEventDelegate,
		ecsact_entity_id,
		ecsact_component_id,
		const void*
	);
	DECLARE_MULTICAST_DELEGATE_TwoParams( //
		FRawEntityEventDelegate,
		ecsact_entity_id,
		ecsact_placeholder_entity_id
	);

	FRawComponentEventDelegate InitComponentRawEvent;
	FRawComponentEventDelegate UpdateComponentRawEvent;
	FRawComponentEventDelegate RemoveComponentRawEvent;

	FRawEntityEventDelegate CreatedEntityRawEvent;
	FRawEntityEventDelegate DestroyedEntityRawEvent;

	UEcsactUnrealEventsCollector();

	/**
	 * Get's the C `ecsact_execution_events_collector` pointer typically passed to
	 * `ecsact_execute_systems` or `ecsact_async_flush_events`. The lifetime of
	 * this pointer is the same as the owning `EcsactUnrealEventsCollector`.
	 */
	auto GetCEVC() -> ecsact_execution_events_collector*;
};
