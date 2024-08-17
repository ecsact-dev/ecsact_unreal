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
public:
	DECLARE_DELEGATE(FAsyncRequestDoneCallback); // NOLINT
	DECLARE_DELEGATE_OneParam( // NOLINT
		FAsyncRequestErrorCallback,
		ecsact_async_error
	);

	virtual auto OnRequestDone(
		ecsact_async_request_id   RequestId,
		FAsyncRequestDoneCallback Callback
	) -> void;

	virtual auto OnRequestError(
		ecsact_async_request_id    RequestId,
		FAsyncRequestErrorCallback Callback
	) -> void;
};
