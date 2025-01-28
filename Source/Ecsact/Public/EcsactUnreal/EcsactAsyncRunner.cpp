#include "EcsactUnreal/EcsactAsyncRunner.h"
#include "EcsactUnreal/Ecsact.h"
#include <span>
#include "EcsactUnreal/EcsactUnrealExecutionOptions.h"
#include "EcsactUnreal/EcsactRunnerSubsystem.h"
#include "ecsact/runtime/async.h"
#include "ecsact/runtime/common.h"

TMap<ecsact_async_session_id, TWeakObjectPtr<UEcsactAsyncRunner>>
	UEcsactAsyncRunner::sessions = {};

auto UEcsactAsyncRunner::StopAllAsyncSessions() -> void {
	for(auto&& [session_id, runner] : sessions) {
		if(runner.IsValid()) {
			ecsact_async_stop(session_id);
			runner.Get()->SessionId = ECSACT_INVALID_ID(async_session);
		}
	}

	sessions.Empty();
}

auto UEcsactAsyncRunner::GetAsyncRunnerBySession( //
	ecsact_async_session_id id
) -> TWeakObjectPtr<UEcsactAsyncRunner> {
	return sessions.FindRef(id);
}

UEcsactAsyncRunner::UEcsactAsyncRunner() {
	async_evc = ecsact_async_events_collector{
		.async_error_callback = ThisClass::OnAsyncErrorRaw,
		.async_error_callback_user_data = this,

		.system_error_callback = ThisClass::OnExecuteSysErrorRaw,
		.system_error_callback_user_data = this,

		.async_request_done_callback = ThisClass::OnAsyncRequestDoneRaw,
		.async_request_done_callback_user_data = this,

		.async_session_event_callback = ThisClass::OnAsyncSessionEventRaw,
		.async_session_event_callback_user_data = this,
	};
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

	ecsact_async_stream(SessionId, Entity, ComponentId, ComponentData, nullptr);
}

auto UEcsactAsyncRunner::Stop() -> void {
	if(SessionId != ECSACT_INVALID_ID(async_session)) {
		AsyncSessionStop();
	}

	Super::Stop();
}

auto UEcsactAsyncRunner::AsyncSessionStart( //
	const void* options,
	int32_t     options_size
) -> void {
	if(SessionId != ECSACT_INVALID_ID(async_session)) {
		AsyncSessionStop();
	}

	SessionId = ecsact_async_start(options, options_size);
	sessions.Add(SessionId, this);
}

auto UEcsactAsyncRunner::AsyncSessionStop() -> void {
	if(SessionId != ECSACT_INVALID_ID(async_session)) {
		if(ecsact_async_stop) {
			ecsact_async_stop(SessionId);
		} else {
			UE_LOG(
				Ecsact,
				Error,
				TEXT("ecsact_async_stop unavailable - unable to stop ecsact async "
						 "session")
			);
		}
		sessions.Remove(SessionId);
		SessionId = ECSACT_INVALID_ID(async_session);
	}
}

auto UEcsactAsyncRunner::GetAsyncSessionTick() const -> int32 {
	if(SessionId != ECSACT_INVALID_ID(async_session)) {
		if(ecsact_async_get_current_tick) {
			return ecsact_async_get_current_tick(SessionId);
		} else {
			UE_LOG(Ecsact, Error, TEXT("ecsact_async_get_current_tick unavailable"));
		}
	}

	return 0;
}

auto UEcsactAsyncRunner::OnAsyncErrorRaw(
	ecsact_async_session_id  session_id,
	ecsact_async_error       async_err,
	int                      request_ids_length,
	ecsact_async_request_id* request_ids_data,
	void*                    callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);
	if(!self) {
		UE_LOG(
			Ecsact,
			Warning,
			TEXT("Ecsact async session %i could not be found"),
			session_id
		);
		return;
	}
	auto request_ids =
		std::span{request_ids_data, static_cast<size_t>(request_ids_length)};

	for(auto req_id : request_ids) {
		auto cbs = self->RequestErrorCallbacks.Find(req_id);

		if(cbs && !cbs->IsEmpty()) {
			for(auto& cb : *cbs) {
				if(!cb.ExecuteIfBound(session_id, async_err)) {
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
	ecsact_async_session_id      session_id,
	ecsact_execute_systems_error execute_err,
	void*                        callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);
	if(!self) {
		UE_LOG(
			Ecsact,
			Warning,
			TEXT("Ecsact async session %i could not be found"),
			session_id
		);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("System error"));
}

auto UEcsactAsyncRunner::OnAsyncRequestDoneRaw(
	ecsact_async_session_id  session_id,
	int                      request_ids_length,
	ecsact_async_request_id* request_ids_data,
	void*                    callback_user_data
) -> void {
	auto self = static_cast<ThisClass*>(callback_user_data);
	if(!self) {
		UE_LOG(
			Ecsact,
			Warning,
			TEXT("Ecsact async session %i could not be found"),
			session_id
		);
		return;
	}

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

auto UEcsactAsyncRunner::OnAsyncSessionEventRaw(
	ecsact_async_session_id    session_id,
	ecsact_async_session_event event,
	void*                      callback_user_data
) -> void {
	UE_LOG(LogTemp, Log, TEXT("OnAsyncSessionEventRaw %i %i"), session_id, event);
	auto self = static_cast<ThisClass*>(callback_user_data);
	self->AsyncSessionEvent.Broadcast(
		static_cast<int32_t>(session_id),
		static_cast<EEcsactAsyncSessionEvent>(event)
	);

	for(auto subsystem : self->GetSubsystemArray<UEcsactRunnerSubsystem>()) {
		if(subsystem) {
			subsystem->AsyncSessionEvent( //
				static_cast<EEcsactAsyncSessionEvent>(event)
			);
		}
	}

	if(event == ECSACT_ASYNC_SESSION_STOPPED) {
		sessions.Remove(session_id);
	}
}

auto UEcsactAsyncRunner::Tick(float DeltaTime) -> void {
	if(IsStopped()) {
		return;
	}

	EnqueueExecutionOptions();

	if(ecsact_async_flush_events == nullptr) {
		UE_LOG(Ecsact, Error, TEXT("ecsact_async_flush_events is unavailable"));
	} else if(SessionId != ECSACT_INVALID_ID(async_session)) {
		ecsact_async_flush_events(SessionId, GetEventsCollector(), &async_evc);
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
		auto req_id = ecsact_async_enqueue_execution_options(
			SessionId,
			*ExecutionOptions->GetCPtr()
		);
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
