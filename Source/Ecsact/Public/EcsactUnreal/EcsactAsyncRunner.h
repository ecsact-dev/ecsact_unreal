
#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "UObject/NoExportTypes.h"
#include "EcsactUnreal/EcsactRunner.h"
#include "EcsactUnreal/EcsactAsyncRunnerEvents.h"
#include "EcsactAsyncRunner.generated.h"

UCLASS(NotBlueprintable)

class ECSACT_API UEcsactAsyncRunner //
	: public UEcsactRunner,
		public IEcsactAsyncRunnerEvents {
	GENERATED_BODY() // NOLINT

	static TMap<ecsact_async_session_id, UEcsactAsyncRunner*> sessions;
	static ecsact_async_events_collector                      async_evc;

	ecsact_async_session_id SessionId = ECSACT_INVALID_ID(async_session);
	TMap<ecsact_async_request_id, TArray<FAsyncRequestDoneCallback>>
		RequestDoneCallbacks;
	TMap<ecsact_async_request_id, TArray<FAsyncRequestErrorCallback>>
		RequestErrorCallbacks;

	TArray<TDelegate<void()>> GenericConnectCallbacks;
	TArray<TDelegate<void()>> GenericDisconnectCallbacks;

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

protected:
	auto StreamImpl(
		ecsact_entity_id    Entity,
		ecsact_component_id ComponentId,
		const void*         ComponentData
	) -> void override;

	auto TriggerGenericConnectCallbacks() -> void override;
	auto TriggerGenericDisconnectCallbacks() -> void override;

public:
	static void StopAllAsyncSessions();

	UEcsactAsyncRunner();

	auto Tick(float DeltaTime) -> void override;
	auto GetStatId() const -> TStatId override;

	/**
	 * Usually execution options are enqueued during `Tick`, but if you'd prefer
	 * to enqueue them earlier then you can call this function to immediate
	 * enqueue the execution options.
	 */
	auto EnqueueExecutionOptions() -> void;

	/**
	 * Wrapper around ecsact_async_connect
	 */
	auto Connect( //
		const char*                ConnectionStr,
		FAsyncRequestErrorCallback ErrorCallback,
		FAsyncRequestDoneCallback  Callback
	) -> void;

	auto Disconnect() -> void;

	auto OnAsyncSessionStart(TDelegate<void()> Callback) -> void override;

	auto OnAsyncSessionStop(TDelegate<void()> Callback) -> void override;

	auto OnRequestDone(
		ecsact_async_request_id   RequestId,
		FAsyncRequestDoneCallback Callback
	) -> void override;

	auto OnRequestError(
		ecsact_async_request_id    RequestId,
		FAsyncRequestErrorCallback Callback
	) -> void override;
};
