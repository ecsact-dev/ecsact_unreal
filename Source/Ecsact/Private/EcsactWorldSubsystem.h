// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

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
