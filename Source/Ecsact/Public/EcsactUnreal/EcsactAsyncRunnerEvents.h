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

public:
	DECLARE_DELEGATE(FAsyncRequestDoneCallback); // NOLINT
	DECLARE_DELEGATE_OneParam( // NOLINT
		FAsyncRequestErrorCallback,
		ecsact_async_error
	);

	/**
	 * Add a generic 'connected' callback.
	 * NOTE: only works if connecting via runner, not if using
	 * ecsact_async_connect directly.
	 */
	virtual auto OnConnect(TDelegate<void()> Callback) -> void;

	/**
	 * Add a generic 'disconnect' callback. Called when an error that is supposed
	 * to disconnect the async implementation or when Disconnect is explicitly
	 * called on the runner. NOTE: only works on the runner Disconnect method, not
	 * if using ecsact_async_disconnect directly.
	 */
	virtual auto OnDisconnect(TDelegate<void()> Callback) -> void;

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
