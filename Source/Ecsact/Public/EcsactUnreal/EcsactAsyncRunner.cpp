#include "EcsactUnreal/EcsactAsyncRunner.h"
#include "EcsactUnreal/Ecsact.h"
#include <span>
#include "EcsactUnreal/EcsactUnrealExecutionOptions.h"
#include "EcsactUnreal/EcsactRunnerSubsystem.h"
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

auto UEcsactAsyncRunner::StreamImpl(
	ecsact_entity_id    Entity,
	ecsact_component_id ComponentId,
	const void*         ComponentData
) -> void {
#if WITH_EDITORONLY_DATA
	if(ecsact_async_stream == nullptr) {
		static bool notified_log = false;
		if(!notified_log) {
			notified_log = true;
			UE_LOG(
				Ecsact,
				Error,
				TEXT("ecsact_async_stream unavailable - cannot use "
						 "UEcsactRunner::Stream")
			);
		}
		return;
	}
#endif

	ecsact_async_stream(Entity, ComponentId, ComponentData, nullptr);
}

auto UEcsactAsyncRunner::Connect( //
	const char*                ConnectionStr,
	FAsyncRequestErrorCallback ErrorCallback,
	FAsyncRequestDoneCallback  Callback
) -> void {
	auto req_id = ecsact_async_connect(ConnectionStr);
	OnRequestDone(
		req_id,
		FAsyncRequestDoneCallback::CreateLambda( //
			[this, Callback = std::move(Callback)] {
				Callback.ExecuteIfBound();
				TriggerGenericConnectCallbacks();
			}
		)
	);
	OnRequestError(
		req_id,
		FAsyncRequestErrorCallback::CreateLambda( //
			[this, ErrorCallback = std::move(ErrorCallback)](ecsact_async_error err) {
				ErrorCallback.ExecuteIfBound(err);
			}
		)
	);
}

auto UEcsactAsyncRunner::Disconnect() -> void {
	ecsact_async_disconnect();
	TriggerGenericConnectCallbacks();
}

auto UEcsactAsyncRunner::TriggerGenericConnectCallbacks() -> void {
	UE_LOG(LogTemp, Log, TEXT("TriggerGenericConnectCallbacks()"));
	for(auto& cb : GenericConnectCallbacks) {
		cb.ExecuteIfBound();
	}

	for(auto subsystem : GetSubsystemArray<UEcsactRunnerSubsystem>()) {
		if(subsystem) {
			subsystem->AsyncConnected();
		}
	}
}

auto UEcsactAsyncRunner::TriggerGenericDisconnectCallbacks() -> void {
	UE_LOG(LogTemp, Log, TEXT("TriggerGenericDisconnectCallbacks()"));
	for(auto& cb : GenericDisconnectCallbacks) {
		cb.ExecuteIfBound();
	}

	for(auto subsystem : GetSubsystemArray<UEcsactRunnerSubsystem>()) {
		if(subsystem) {
			subsystem->AsyncDisconnected();
		}
	}
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

		if(cbs && !cbs->IsEmpty()) {
			for(auto& cb : *cbs) {
				if(!cb.ExecuteIfBound(async_err)) {
					UE_LOG(
						Ecsact,
						Warning,
						TEXT("Unbound async error callback for request %i"),
						req_id
					);
				}
			}
		}
	}
}

auto UEcsactAsyncRunner::OnExecuteSysErrorRaw(
	ecsact_execute_systems_error execute_err,
	void*                        callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);

	UE_LOG(LogTemp, Warning, TEXT("System error"));
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

		if(cbs && !cbs->IsEmpty()) {
			for(auto& cb : *cbs) {
				if(!cb.ExecuteIfBound()) {
					UE_LOG(
						Ecsact,
						Warning,
						TEXT("Unbound async done callback for request %i"),
						req_id
					);
				}
			}

			cbs->Empty();
		}
	}
}

auto UEcsactAsyncRunner::Tick(float DeltaTime) -> void {
	if(IsStopped()) {
		return;
	}

	EnqueueExecutionOptions();

	if(ecsact_async_flush_events == nullptr) {
		UE_LOG(Ecsact, Error, TEXT("ecsact_async_flush_events is unavailable"));
	} else {
		ecsact_async_flush_events(GetEventsCollector(), &async_evc);
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
		auto req_id =
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
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Adding request done handler (req=%i)"),
		static_cast<int>(RequestId)
	);
	RequestDoneCallbacks.FindOrAdd(RequestId).Add(Callback);
}

auto UEcsactAsyncRunner::OnRequestError(
	ecsact_async_request_id    RequestId,
	FAsyncRequestErrorCallback Callback
) -> void {
	check(RequestId != ECSACT_INVALID_ID(async_request));
	RequestErrorCallbacks.FindOrAdd(RequestId).Add(Callback);
}

auto UEcsactAsyncRunner::OnConnect(TDelegate<void()> Callback) -> void {
	GenericConnectCallbacks.Add(std::move(Callback));
}

auto UEcsactAsyncRunner::OnDisconnect(TDelegate<void()> Callback) -> void {
	GenericDisconnectCallbacks.Add(std::move(Callback));
}
