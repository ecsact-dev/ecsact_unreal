// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "UObject/NoExportTypes.h"
#include "EcsactUnreal/EcsactRunner.h"
#include "EcsactUnreal/EcsactAsyncRunnerEvents.h"
#include "EcsactAsyncRunner.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FEcsactAsyncSessionEventDelegate,
	int32,
	EEcsactAsyncSessionEvent
);

UCLASS(NotBlueprintable)

class ECSACT_API UEcsactAsyncRunner //
	: public UEcsactRunner,
		public IEcsactAsyncRunnerEvents {
	GENERATED_BODY() // NOLINT

	static TMap<ecsact_async_session_id, TWeakObjectPtr<UEcsactAsyncRunner>>
		sessions;

	ecsact_async_events_collector async_evc;

	ecsact_async_session_id SessionId = ECSACT_INVALID_ID(async_session);
	TMap<ecsact_async_request_id, TArray<FAsyncRequestDoneCallback>>
		RequestDoneCallbacks;
	TMap<ecsact_async_request_id, TArray<FAsyncRequestErrorCallback>>
		RequestErrorCallbacks;

	static auto OnAsyncErrorRaw(
		ecsact_async_session_id  session_id,
		ecsact_async_error       async_err,
		int                      request_ids_length,
		ecsact_async_request_id* request_ids,
		void*                    callback_user_data
	) -> void;

	static auto OnExecuteSysErrorRaw(
		ecsact_async_session_id      session_id,
		ecsact_execute_systems_error execute_err,
		void*                        callback_user_data
	) -> void;

	static auto OnAsyncRequestDoneRaw(
		ecsact_async_session_id  session_id,
		int                      request_ids_length,
		ecsact_async_request_id* request_ids,
		void*                    callback_user_data
	) -> void;

	static auto OnAsyncSessionEventRaw(
		ecsact_async_session_id    session_id,
		ecsact_async_session_event event,
		void*                      callback_user_data
	) -> void;

protected:
	auto StreamImpl(
		ecsact_entity_id    Entity,
		ecsact_component_id ComponentId,
		const void*         ComponentData
	) -> void override;

public:
	static auto StopAllAsyncSessions() -> void;

	/**
	 * Gets an async runner by the session ID if it the session was started by an
	 * UEcsactAsyncRunner or derived classes of UEcsactAsyncRunner.
	 */
	static auto GetAsyncRunnerBySession( //
		ecsact_async_session_id id
	) -> TWeakObjectPtr<UEcsactAsyncRunner>;

	FEcsactAsyncSessionEventDelegate AsyncSessionEvent;

	UEcsactAsyncRunner();

	auto Tick(float DeltaTime) -> void override;
	auto GetStatId() const -> TStatId override;
	auto Stop() -> void override;

	/**
	 * Usually execution options are enqueued during `Tick`, but if you'd prefer
	 * to enqueue them earlier then you can call this function to immediate
	 * enqueue the execution options.
	 */
	auto EnqueueExecutionOptions() -> void;

	/**
	 * Wrapper around `ecsact_async_start`
	 */
	auto AsyncSessionStart(const void* options, int32_t options_size) -> void;

	/**
	 * Wrapper around `ecsact_async_stop`
	 */
	auto AsyncSessionStop() -> void;

	/**
	 * Wrapper around `ecsact_async_get_current_tick`
	 */
	auto GetAsyncSessionTick() const -> int32;

	auto OnRequestDone(
		ecsact_async_request_id   RequestId,
		FAsyncRequestDoneCallback Callback
	) -> void override;

	auto OnRequestError(
		ecsact_async_request_id    RequestId,
		FAsyncRequestErrorCallback Callback
	) -> void override;
};
