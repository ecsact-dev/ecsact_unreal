#include "EcsactUnreal/EcsactAsyncRunnerEvents.h"

auto IEcsactAsyncRunnerEvents::OnConnect( //
	TDelegate<void()> Callback
) -> void {
}

auto IEcsactAsyncRunnerEvents::OnDisconnect( //
	TDelegate<void()> Callback
) -> void {
}

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

auto IEcsactAsyncRunnerEvents::TriggerGenericConnectCallbacks() -> void {
}

auto IEcsactAsyncRunnerEvents::TriggerGenericDisconnectCallbacks() -> void {
}
