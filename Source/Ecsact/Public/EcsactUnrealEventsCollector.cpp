#include "EcsactUnrealEventsCollector.h"
#include "Ecsact.h"

UEcsactUnrealEventsCollector::UEcsactUnrealEventsCollector() : evc{} {
	evc.init_callback_user_data = this;
	evc.update_callback_user_data = this;
	evc.remove_callback_user_data = this;
	evc.entity_created_callback_user_data = this;
	evc.entity_destroyed_callback_user_data = this;

	evc.init_callback = ThisClass::OnInitComponentRaw;
	evc.update_callback = ThisClass::OnUpdateComponentRaw;
	evc.remove_callback = ThisClass::OnRemoveComponentRaw;
	evc.entity_created_callback = ThisClass::OnEntityCreatedRaw;
	evc.entity_destroyed_callback = ThisClass::OnEntityDestroyedRaw;
}

auto UEcsactUnrealEventsCollector::OnInitComponentRaw(
	ecsact_event        event,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	void*               callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);

	UE_LOG(
		Ecsact,
		Log,
		TEXT("OnInitComponent (entity=%i, component=%i)"),
		static_cast<int32_t>(entity_id),
		static_cast<int32_t>(component_id)
	);

	self->InitComponentRawEvent
		.Broadcast(entity_id, component_id, component_data);
}

auto UEcsactUnrealEventsCollector::OnUpdateComponentRaw(
	ecsact_event        event,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	void*               callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);

	UE_LOG(
		Ecsact,
		Log,
		TEXT("OnUpdateComponent (entity=%i, component=%i)"),
		static_cast<int32_t>(entity_id),
		static_cast<int32_t>(component_id)
	);

	self->UpdateComponentRawEvent
		.Broadcast(entity_id, component_id, component_data);
}

auto UEcsactUnrealEventsCollector::OnRemoveComponentRaw(
	ecsact_event        event,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	void*               callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);

	UE_LOG(
		Ecsact,
		Log,
		TEXT("OnRemoveComponent (entity=%i, component=%i)"),
		static_cast<int32_t>(entity_id),
		static_cast<int32_t>(component_id)
	);
	self->RemoveComponentRawEvent
		.Broadcast(entity_id, component_id, component_data);
}

auto UEcsactUnrealEventsCollector::OnEntityCreatedRaw(
	ecsact_event                 event,
	ecsact_entity_id             entity_id,
	ecsact_placeholder_entity_id placeholder_entity_id,
	void*                        callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);

	UE_LOG(
		Ecsact,
		Log,
		TEXT("OnEntityCreated (entity=%i)"),
		static_cast<int32_t>(entity_id)
	);
	self->CreatedEntityRawEvent.Broadcast(entity_id, placeholder_entity_id);
}

auto UEcsactUnrealEventsCollector::OnEntityDestroyedRaw(
	ecsact_event                 event,
	ecsact_entity_id             entity_id,
	ecsact_placeholder_entity_id placeholder_entity_id,
	void*                        callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);

	UE_LOG(
		Ecsact,
		Log,
		TEXT("OnEntityDestroyed (entity=%i)"),
		static_cast<int32_t>(entity_id)
	);
	self->DestroyedEntityRawEvent.Broadcast(entity_id, placeholder_entity_id);
}

auto UEcsactUnrealEventsCollector::GetCEVC()
	-> ecsact_execution_events_collector* {
	return &evc;
}
