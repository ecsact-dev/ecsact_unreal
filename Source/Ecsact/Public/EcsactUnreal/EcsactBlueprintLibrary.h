// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "EcsactBlueprintLibrary.generated.h"

UCLASS()

class ECSACT_API UEcsactBlueprintLibrary : public UBlueprintFunctionLibrary {
	GENERATED_BODY() // NOLINT
public:
	UFUNCTION(
		BlueprintCallable,
		Category = "Ecsact Runtime",
		Meta =
			(WorldContext = "WorldContext",
			 BlueprintFunctionLibraryIcon = "Ecsact.ecsact-color-32x32")
	)
	static void AsyncDisconnect(const UObject* WorldContext);
};
