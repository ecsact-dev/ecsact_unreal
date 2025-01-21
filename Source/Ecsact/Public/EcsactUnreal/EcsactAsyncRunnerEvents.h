#pragma once

#include "CoreMinimal.h"
#include "ecsact/runtime/common.h"
#include "ecsact/runtime/async.h"
#include "EcsactAsyncRunnerEvents.generated.h"

UINTERFACE(MinimalAPI)

class UEcsactAsyncRunnerEvents : public UInterface {
	GENERATED_BODY() // NOLINT
public:
};

class IEcsactAsyncRunnerEvents {
	GENERATED_BODY() // NOLINT
	friend class UEcsactAsyncConnectBlueprintAction;
	friend class UEcsactBlueprintLibrary;

public:
	DECLARE_DELEGATE(FAsyncRequestDoneCallback); // NOLINT
	DECLARE_DELEGATE_OneParam( // NOLINT
		FAsyncRequestErrorCallback,
		ecsact_async_error
	);

	/**
	 * Add a generic 'session start' callback.
	 * NOTE: only works if connecting via runner, not if using
	 * ecsact_async_start directly.
	 */
	virtual auto OnAsyncSessionStart(TDelegate<void()> Callback) -> void;

	/**
	 * Add a generic 'session stop' callback. Called when an error that is
	 * supposed to stop the async implementation or when AsyncSessionStop is
	 * explicitly called on the runner. NOTE: only works on the runner Disconnect
	 * method, not if using ecsact_async_stop directly.
	 */
	virtual auto OnAsyncSessionStop(TDelegate<void()> Callback) -> void;

	virtual auto OnRequestDone(
		ecsact_async_request_id   RequestId,
		FAsyncRequestDoneCallback Callback
	) -> void;

	virtual auto OnRequestError(
		ecsact_async_request_id    RequestId,
		FAsyncRequestErrorCallback Callback
	) -> void;

protected:
	virtual auto TriggerGenericConnectCallbacks() -> void;
	virtual auto TriggerGenericDisconnectCallbacks() -> void;
};
