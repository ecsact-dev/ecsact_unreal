#include "EcsactAsyncConnectBlueprintAction.h"
#include "EcsactExecution.h"
#include "EcsactAsyncRunnerEvents.h"
#include "ecsact/runtime/async.h"
#include "Ecsact.h"

auto UEcsactAsyncConnectBlueprintAction::AsyncConnect( //
	const FString& ConnectionString
) -> UEcsactAsyncConnectBlueprintAction* {
	auto action = NewObject<UEcsactAsyncConnectBlueprintAction>();
	action->Utf8ConnectionString = TCHAR_TO_UTF8(*ConnectionString);
	return action;
}

auto UEcsactAsyncConnectBlueprintAction::Activate() -> void {
	UE_LOG(Ecsact, Warning, TEXT("AsyncConnectActivate()"));
	auto runner = EcsactUnrealExecution::Runner();
	auto async_events = Cast<IEcsactAsyncRunnerEvents>(runner);
	if(!async_events) {
		UE_LOG(
			Ecsact,
			Error,
			TEXT("Cannot use Ecsact async blueprint api with runner that does not "
					 "implement IEcsactAsyncRunnerEvents")
		);
		return;
	}

	auto req_id = ecsact_async_connect(Utf8ConnectionString.c_str());

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
	if(!bConnectFailed) {
		OnSuccess.Broadcast({});
	}
	OnDone.Broadcast({});
}

auto UEcsactAsyncConnectBlueprintAction::OnRequestError( //
	ecsact_async_error Error
) -> void {
	switch(Error) {
		case ECSACT_ASYNC_ERR_PERMISSION_DENIED:
			OnError.Broadcast(EAsyncConnectError::PermissionDenied);
			break;
		case ECSACT_ASYNC_INVALID_CONNECTION_STRING:
			OnError.Broadcast(EAsyncConnectError::PermissionDenied);
			break;
	}

	bConnectFailed = true;
}
