// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

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
