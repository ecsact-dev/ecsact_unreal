// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ecsact/runtime/common.h"
#include "ecsact/runtime/async.h"
#include "EcsactAsyncRunnerEvents.generated.h"

/// 1:1 match with ecsact_async_session_event
UENUM(BlueprintType)
enum class EEcsactAsyncSessionEvent : uint8 {
	Stopped = 0,
	Pending = 1,
	Started = 2,
};

static_assert(
	static_cast<int32_t>(EEcsactAsyncSessionEvent::Stopped) ==
	static_cast<int32_t>(ECSACT_ASYNC_SESSION_STOPPED)
);

static_assert(
	static_cast<int32_t>(EEcsactAsyncSessionEvent::Pending) ==
	static_cast<int32_t>(ECSACT_ASYNC_SESSION_PENDING)
);

static_assert(
	static_cast<int32_t>(EEcsactAsyncSessionEvent::Started) ==
	static_cast<int32_t>(ECSACT_ASYNC_SESSION_START)
);

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
	DECLARE_DELEGATE_TwoParams( // NOLINT
		FAsyncRequestErrorCallback,
		ecsact_async_session_id,
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
