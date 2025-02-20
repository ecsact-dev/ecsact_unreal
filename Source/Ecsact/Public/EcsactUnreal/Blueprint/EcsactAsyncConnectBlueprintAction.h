// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#pragma once

#include <string>
#include "EcsactUnreal/EcsactAsyncRunnerEvents.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ecsact/runtime/async.h"
#include "EcsactAsyncConnectBlueprintAction.generated.h"

UENUM()
enum class EAsyncConnectError : uint8 {
	NoError,
	AsyncRunnerUnavailable,
	InvalidRequestId,
	PermissionDenied,
	InvalidConnectionString,
	InvalidSessionId,
};

/**
 *
 */
UCLASS()

class ECSACT_API UEcsactAsyncConnectBlueprintAction
	: public UBlueprintAsyncActionBase {
	GENERATED_BODY() // NOLINT

	UWorld*     World = nullptr;
	std::string Utf8ConnectionString;
	bool        bConnectFailed = false;

	FDelegateHandle SessionEventHandle;

	auto OnRequestDone() -> void;
	auto OnRequestError(ecsact_async_error Error) -> void;

public:
	/**
	 * Called everytime an async request is updated. Error parameter is only
	 * valid during `OnError`.
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( // NOLINT
		FAsyncConnectDoneCallback,
		EAsyncConnectError,
		Error
	);

	UFUNCTION(
		BlueprintCallable,
		Category = "Ecsact Runtime",
		Meta = (BlueprintInternalUseOnly = true, WorldContext = "WorldContext")
	)
	static UEcsactAsyncConnectBlueprintAction* AsyncConnect(
		const UObject* WorldContext,
		const FString& ConnectionString
	);

	/**
	 * Async request is finally done.
	 */
	UPROPERTY(BlueprintAssignable)
	FAsyncConnectDoneCallback OnDone;

	/**
	 * Async request is done and no errors occurred.
	 */
	UPROPERTY(BlueprintAssignable)
	FAsyncConnectDoneCallback OnSuccess;

	/**
	 * Async request had an error. May be called multiple times.
	 */
	UPROPERTY(BlueprintAssignable)
	FAsyncConnectDoneCallback OnError;

	auto Activate() -> void override;

	auto GetWorld() const -> UWorld* override;

protected:
	auto ConnectRequest(std::string ConnectionString) -> void;
	auto OnAsyncSessionEvent( //
		int32                    SessionId,
		EEcsactAsyncSessionEvent Event
	) -> void;
};
