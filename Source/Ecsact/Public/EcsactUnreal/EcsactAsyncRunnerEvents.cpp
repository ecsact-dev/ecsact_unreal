// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

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
