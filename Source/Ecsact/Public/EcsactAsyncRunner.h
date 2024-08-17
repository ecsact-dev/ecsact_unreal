
#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "UObject/NoExportTypes.h"
#include "EcsactRunner.h"
#include "EcsactAsyncRunnerEvents.h"
#include "EcsactAsyncRunner.generated.h"

UCLASS(NotBlueprintable)

class UEcsactAsyncRunner : public UEcsactRunner,
													 public IEcsactAsyncRunnerEvents {
	GENERATED_BODY() // NOLINT

	ecsact_async_events_collector async_evc;

	TMap<ecsact_async_request_id, TArray<FAsyncRequestDoneCallback>>
		RequestDoneCallbacks;
	TMap<ecsact_async_request_id, TArray<FAsyncRequestErrorCallback>>
		RequestErrorCallbacks;

	static auto OnAsyncErrorRaw(
		ecsact_async_error       async_err,
		int                      request_ids_length,
		ecsact_async_request_id* request_ids,
		void*                    callback_user_data
	) -> void;

	static auto OnExecuteSysErrorRaw(
		ecsact_execute_systems_error execute_err,
		void*                        callback_user_data
	) -> void;

	static auto OnAsyncRequestDoneRaw(
		int                      request_ids_length,
		ecsact_async_request_id* request_ids,
		void*                    callback_user_data
	) -> void;

public:
	UEcsactAsyncRunner();

	auto Tick(float DeltaTime) -> void override;
	auto GetStatId() const -> TStatId override;

	/**
	 * Usually execution options are enqueued during `Tick`, but if you'd prefer
	 * to enqueue them earlier then you can call this function to immediate
	 * enqueue the execution options.
	 */
	auto EnqueueExecutionOptions() -> void;

	auto OnRequestDone(
		ecsact_async_request_id   RequestId,
		FAsyncRequestDoneCallback Callback
	) -> void override;

	auto OnRequestError(
		ecsact_async_request_id    RequestId,
		FAsyncRequestErrorCallback Callback
	) -> void override;
};
