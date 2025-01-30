#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Modules/ModuleManager.h"
#include "UObject/WeakObjectPtr.h"
#include "EcsactUnreal/RuntimeHandle.h"

ECSACT_API DECLARE_LOG_CATEGORY_EXTERN(Ecsact, Log, All);

namespace EcsactUnreal::Detail {
auto CheckRuntimeNotLoaded(FEcsactModule&) -> bool;
auto CheckRuntimeHandle(FEcsactModule&, const FEcsactRuntimeHandle&) -> bool;
auto GetDefaultRuntimeDllHandle() -> void*;

auto CheckUnloadable(FEcsactModule&, const FEcsactRuntimeHandle&) -> bool;
auto UnloadPostDisconnect(FEcsactModule&, FEcsactRuntimeHandle&) -> void;
auto UnloadPostReset(FEcsactModule&, FEcsactRuntimeHandle&) -> void;
} // namespace EcsactUnreal::Detail

class FEcsactModule : public IModuleInterface {
	// clang-format off
	friend class EcsactUnrealExecution;
	friend auto EcsactUnreal::Detail::CheckRuntimeNotLoaded(FEcsactModule&) -> bool;
	friend auto EcsactUnreal::Detail::CheckRuntimeHandle(FEcsactModule&, const FEcsactRuntimeHandle&) -> bool;
	friend auto EcsactUnreal::Detail::GetDefaultRuntimeDllHandle() -> void*;
	friend auto EcsactUnreal::Detail::CheckUnloadable(FEcsactModule&, const FEcsactRuntimeHandle&) -> bool;
	friend auto EcsactUnreal::Detail::UnloadPostDisconnect(FEcsactModule&, FEcsactRuntimeHandle&) -> void;
	friend auto EcsactUnreal::Detail::UnloadPostReset(FEcsactModule&, FEcsactRuntimeHandle&) -> void;
	// clang-format on

	static FEcsactModule* Self;
	void*                 EcsactRuntimeHandle;

	auto Abort() -> void;

	auto SetRuntimeHandle(const FEcsactRuntimeHandle&) -> void;
	auto IsSameRuntimeHandle(const FEcsactRuntimeHandle&) -> bool;
	auto FreeRuntimeHandle(FEcsactRuntimeHandle&) -> void;

public:
	static auto Get() -> FEcsactModule&;

	auto StartupModule() -> void override;
	auto ShutdownModule() -> void override;
};
