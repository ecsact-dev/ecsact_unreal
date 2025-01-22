#include "EcsactUnreal/Blueprint/EcsactAsyncConnectBlueprintAction.h"
#include "EcsactUnreal/EcsactAsyncRunnerEvents.h"
#include "EcsactUnreal/EcsactExecution.h"
#include "EcsactUnreal/EcsactAsyncRunner.h"
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
	auto async_runner = Cast<UEcsactAsyncRunner>(runner);
	if(!async_runner) {
		UE_LOG(
			Ecsact,
			Error,
			TEXT("Cannot use Ecsact async blueprint api with runner that does not "
					 "inherit UEcsactAsyncRunner")
		);
		OnError.Broadcast(EAsyncConnectError::AsyncRunnerUnavailable);
		OnDone.Broadcast({});
		return;
	}

	SessionEventHandle = async_runner->AsyncSessionEvent.AddUObject(
		this,
		&ThisClass::OnAsyncSessionEvent
	);

	async_runner->AsyncSessionStart(
		ConnectionString.c_str(),
		static_cast<int32_t>(ConnectionString.size())
	);

	auto session_id = ecsact_async_start(
		ConnectionString.c_str(),
		static_cast<int32_t>(ConnectionString.size())
	);
	UE_LOG(Ecsact, Warning, TEXT("async connect session_id=%i"), session_id);

	if(session_id == ECSACT_INVALID_ID(async_session)) {
		UE_LOG(Ecsact, Error, TEXT("Invalid Session ID"));
		OnError.Broadcast(EAsyncConnectError::InvalidSessionId);
		OnDone.Broadcast({});
		return;
	}
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
}

auto UEcsactAsyncConnectBlueprintAction::OnAsyncSessionEvent( //
	int32                    SessionId,
	EEcsactAsyncSessionEvent Event
) -> void {
	auto runner = EcsactUnrealExecution::Runner(GetWorld());
	auto async_runner = Cast<UEcsactAsyncRunner>(runner);

	if(async_runner) {
		switch(Event) {
			case EEcsactAsyncSessionEvent::Stopped:
				async_runner->AsyncSessionEvent.Remove(SessionEventHandle);
				OnDone.Broadcast({});
				break;
			case EEcsactAsyncSessionEvent::Started:
				async_runner->AsyncSessionEvent.Remove(SessionEventHandle);
				OnSuccess.Broadcast({});
				OnDone.Broadcast({});
				break;
			default:
				break;
		}
	}
}
