#include "EcsactUnreal/EcsactBlueprintLibrary.h"
#include "EcsactUnreal/EcsactExecution.h"
#include "EcsactUnreal/EcsactAsyncRunnerEvents.h"
#include "ecsact/runtime/async.h"

auto UEcsactBlueprintLibrary::AsyncDisconnect(const UObject* WorldContext
) -> void {
	auto world = WorldContext->GetWorld();
	check(world);
	auto runner = EcsactUnrealExecution::Runner(world);
	auto async_events = Cast<IEcsactAsyncRunnerEvents>(runner);
	// ecsact_async_disconnect();
	if(async_events) {
		async_events->TriggerGenericDisconnectCallbacks();
	}
}
