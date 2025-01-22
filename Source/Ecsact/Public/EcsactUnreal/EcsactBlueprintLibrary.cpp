#include "EcsactUnreal/EcsactBlueprintLibrary.h"
#include "EcsactUnreal/EcsactExecution.h"
#include "EcsactUnreal/EcsactAsyncRunner.h"

auto UEcsactBlueprintLibrary::AsyncDisconnect( //
	const UObject* WorldContext
) -> void {
	auto world = WorldContext->GetWorld();
	check(world);
	auto runner = EcsactUnrealExecution::Runner(world);
	auto async_runner = Cast<UEcsactAsyncRunner>(runner);
	if(async_runner) {
		async_runner->AsyncSessionStop();
	}
}
