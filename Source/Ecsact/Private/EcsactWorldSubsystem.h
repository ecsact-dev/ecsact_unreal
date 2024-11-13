#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "EcsactUnreal/RuntimeHandle.h"
#include "EcsactWorldSubsystem.generated.h"

UCLASS()

class UEcsactWorldSubsystem : public UWorldSubsystem {
	GENERATED_BODY() // NOLINT

public:
	TWeakObjectPtr<class UEcsactRunner> Runner;

	auto ShouldCreateSubsystem(UObject* Outer) const -> bool override;
	auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
	auto Deinitialize() -> void override;
};
