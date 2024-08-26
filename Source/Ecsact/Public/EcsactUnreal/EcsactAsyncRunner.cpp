#include "EcsactUnreal/EcsactAsyncRunner.h"
#include "EcsactUnreal/Ecsact.h"
#include <span>
#include "EcsactUnreal/EcsactUnrealExecutionOptions.h"
#include "EcsactUnreal/EcsactUnrealEventsCollector.h"
#include "ecsact/runtime/async.h"
#include "ecsact/runtime/common.h"

UEcsactAsyncRunner::UEcsactAsyncRunner() {
	async_evc.async_error_callback = ThisClass::OnAsyncErrorRaw;
	async_evc.system_error_callback = ThisClass::OnExecuteSysErrorRaw;
	async_evc.async_request_done_callback = ThisClass::OnAsyncRequestDoneRaw;

	async_evc.async_error_callback_user_data = this;
	async_evc.system_error_callback_user_data = this;
	async_evc.async_request_done_callback_user_data = this;
}

auto UEcsactAsyncRunner::OnAsyncErrorRaw(
	ecsact_async_error       async_err,
	int                      request_ids_length,
	ecsact_async_request_id* request_ids_data,
	void*                    callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);
	auto request_ids =
		std::span{request_ids_data, static_cast<size_t>(request_ids_length)};

	for(auto req_id : request_ids) {
		auto cbs = self->RequestErrorCallbacks.Find(req_id);
		if(cbs) {
			for(auto& cb : *cbs) {
				cb.ExecuteIfBound(async_err);
			}
		}
	}
}

auto UEcsactAsyncRunner::OnExecuteSysErrorRaw(
	ecsact_execute_systems_error execute_err,
	void*                        callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);
}

auto UEcsactAsyncRunner::OnAsyncRequestDoneRaw(
	int                      request_ids_length,
	ecsact_async_request_id* request_ids_data,
	void*                    callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);
	auto request_ids =
		std::span{request_ids_data, static_cast<size_t>(request_ids_length)};

	for(auto req_id : request_ids) {
		auto cbs = self->RequestDoneCallbacks.Find(req_id);
		if(cbs) {
			for(auto& cb : *cbs) {
				cb.ExecuteIfBound();
			}

			cbs->Empty();
		}
	}
}

auto UEcsactAsyncRunner::Tick(float DeltaTime) -> void {
	EnqueueExecutionOptions();

	if(ecsact_async_flush_events == nullptr) {
		UE_LOG(Ecsact, Error, TEXT("ecsact_async_flush_events is unavailable"));
	} else {
		ecsact_execution_events_collector* evc_c = nullptr;
		if(EventsCollector != nullptr) {
			evc_c = EventsCollector->GetCEVC();
		}
		ecsact_async_flush_events(evc_c, &async_evc);
	}
}

auto UEcsactAsyncRunner::EnqueueExecutionOptions() -> void {
	if(!ExecutionOptions) {
		return;
	}
	if(ecsact_async_enqueue_execution_options == nullptr) {
		UE_LOG(
			Ecsact,
			Error,
			TEXT("ecsact_async_enqueue_execution_options is unavailable")
		);
		return;
	}

	if(ExecutionOptions->IsNotEmpty()) {
		ecsact_async_enqueue_execution_options(*ExecutionOptions->GetCPtr());
		ExecutionOptions->Clear();
	}
}

auto UEcsactAsyncRunner::GetStatId() const -> TStatId {
	RETURN_QUICK_DECLARE_CYCLE_STAT( // NOLINT
		UEcsactAsyncRunner,
		STATGROUP_Tickables
	);
}

auto UEcsactAsyncRunner::OnRequestDone(
	ecsact_async_request_id   RequestId,
	FAsyncRequestDoneCallback Callback
) -> void {
	check(RequestId != ECSACT_INVALID_ID(async_request));

	RequestDoneCallbacks.FindOrAdd(RequestId).Add(Callback);
}

auto UEcsactAsyncRunner::OnRequestError(
	ecsact_async_request_id    RequestId,
	FAsyncRequestErrorCallback Callback
) -> void {
	check(RequestId != ECSACT_INVALID_ID(async_request));

	RequestErrorCallbacks.FindOrAdd(RequestId).Add(Callback);
}