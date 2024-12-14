#include "EcsactUnreal/Blueprint/EcsactAsyncConnectBlueprintAction.h"
#include "EcsactUnreal/EcsactExecution.h"
#include "EcsactUnreal/EcsactAsyncRunnerEvents.h"
#include "ecsact/runtime/async.h"
#include "EcsactUnreal/Ecsact.h"

auto UEcsactAsyncConnectBlueprintAction::AsyncConnect( //
	const FString& ConnectionString
) -> UEcsactAsyncConnectBlueprintAction* {
	auto action = NewObject<UEcsactAsyncConnectBlueprintAction>();
	action->Utf8ConnectionString = TCHAR_TO_UTF8(*ConnectionString);
	return action;
}

auto UEcsactAsyncConnectBlueprintAction::Activate() -> void {
	ConnectRequest(Utf8ConnectionString);
}

auto UEcsactAsyncConnectBlueprintAction::ConnectRequest(
	std::string ConnectionString
) -> void {
	UE_LOG(Ecsact, Warning, TEXT("AsyncConnectActivate()"));
	auto runner = EcsactUnrealExecution::Runner(GetWorld());
	auto async_events = Cast<IEcsactAsyncRunnerEvents>(runner);
	if(!async_events) {
		UE_LOG(
			Ecsact,
			Error,
			TEXT("Cannot use Ecsact async blueprint api with runner that does not "
					 "implement IEcsactAsyncRunnerEvents")
		);
		OnError.Broadcast(EAsyncConnectError::AsyncRunnerEventsUnavailable);
		OnDone.Broadcast({});
		return;
	}

	auto req_id = ecsact_async_connect(ConnectionString.c_str());
	UE_LOG(Ecsact, Warning, TEXT("async connect request id=%i"), req_id);

	if(req_id == ECSACT_INVALID_ID(async_request)) {
		UE_LOG(Ecsact, Error, TEXT("Invalid Request ID"));
		OnError.Broadcast(EAsyncConnectError::InvalidRequestId);
		OnDone.Broadcast({});
		return;
	}

	async_events->OnRequestDone(
		req_id,
		IEcsactAsyncRunnerEvents::FAsyncRequestDoneCallback::CreateUObject(
			this,
			&ThisClass::OnRequestDone
		)
	);

	async_events->OnRequestError(
		req_id,
		IEcsactAsyncRunnerEvents::FAsyncRequestErrorCallback::CreateUObject(
			this,
			&ThisClass::OnRequestError
		)
	);
}

auto UEcsactAsyncConnectBlueprintAction::OnRequestDone() -> void {
	auto runner = EcsactUnrealExecution::Runner(GetWorld());
	auto async_events = Cast<IEcsactAsyncRunnerEvents>(runner);
	UE_LOG(Ecsact, Error, TEXT("OnRequestDone??"));
	if(!bConnectFailed) {
		async_events->TriggerGenericConnectCallbacks();
		OnSuccess.Broadcast({});
	}
	OnDone.Broadcast({});
}

auto UEcsactAsyncConnectBlueprintAction::OnRequestError( //
	ecsact_async_error Error
) -> void {
	UE_LOG(Ecsact, Error, TEXT("OnRequestError??"));
	switch(Error) {
		case ECSACT_ASYNC_ERR_PERMISSION_DENIED:
			OnError.Broadcast(EAsyncConnectError::PermissionDenied);
			break;
		case ECSACT_ASYNC_INVALID_CONNECTION_STRING:
			OnError.Broadcast(EAsyncConnectError::InvalidConnectionString);
			break;
	}

	bConnectFailed = true;
}
