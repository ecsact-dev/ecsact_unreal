#include "EcsactUnreal/EcsactBlueprintLibrary.h"
#include "EcsactUnreal/EcsactExecution.h"
#include "EcsactUnreal/EcsactAsyncRunnerEvents.h"
#include "ecsact/runtime/async.h"

auto UEcsactBlueprintLibrary::AsyncDisconnect() -> void {
	auto runner = EcsactUnrealExecution::Runner();
	auto async_events = Cast<IEcsactAsyncRunnerEvents>(runner);
	ecsact_async_disconnect();
	if(async_events) {
		async_events->TriggerGenericDisconnectCallbacks();
	}
}
