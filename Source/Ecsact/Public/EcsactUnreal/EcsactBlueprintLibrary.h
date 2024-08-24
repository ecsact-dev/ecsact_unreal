#pragma once

#include "EcsactBlueprintLibrary.generated.h"

UCLASS()

class ECSACT_API UEcsactBlueprintLibrary : public UBlueprintFunctionLibrary {
	GENERATED_BODY() // NOLINT
public:
	UFUNCTION(
		BlueprintCallable,
		Category = "Ecsact Runtime",
		Meta = (BlueprintFunctionLibraryIcon = "Ecsact.ecsact-color-32x32")
	)
	static void AsyncDisconnect();
};
