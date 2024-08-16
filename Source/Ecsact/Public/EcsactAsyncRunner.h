
#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "UObject/NoExportTypes.h"
#include "EcsactRunner.h"
#include "EcsactAsyncRunner.generated.h"

UCLASS(NotBlueprintable)

class UEcsactAsyncRunner : public UEcsactRunner {
	GENERATED_BODY()
public:
	auto Tick(float DeltaTime) -> void override;
	auto GetStatId() const -> TStatId override;

	/**
	 * Usually execution options are enqueued during `Tick`, but if you'd prefer
	 * to enqueue them earlier then you can call this function to immediate
	 * enqueue the execution options.
	 */
	auto EnqueueExecutionOptions() -> void;
};
