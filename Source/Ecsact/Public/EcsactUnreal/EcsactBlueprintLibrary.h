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
