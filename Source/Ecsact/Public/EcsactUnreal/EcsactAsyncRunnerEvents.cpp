#include "EcsactUnreal/EcsactAsyncRunnerEvents.h"

auto IEcsactAsyncRunnerEvents::OnRequestDone(
	ecsact_async_request_id   RequestId,
	FAsyncRequestDoneCallback Callback
) -> void {
}

auto IEcsactAsyncRunnerEvents::OnRequestError(
	ecsact_async_request_id    RequestId,
	FAsyncRequestErrorCallback Callback
) -> void {
}
