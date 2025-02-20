// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#pragma once

#include "CoreMinimal.h"
#include "EcsactUnreal/Ecsact.h"
#include "Modules/ModuleManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

DECLARE_LOG_CATEGORY_EXTERN(EcsactEditor, Log, All);

class ECSACTEDITOR_API FEcsactEditorModule : public IModuleInterface {
	FDelegateHandle SourcesWatchHandle;

public:
	using FOnExitDelegate = TDelegate<void(int32)>;
	using FOnReceiveLine = TDelegate<void(FString)>;

private:
	auto OnEditorInitialized(double Duration) -> void;
	auto OnEcsactSettingsModified() -> bool;
	auto OnProjectSourcesChanged( //
		const TArray<struct FFileChangeData>& FileChanges
	) -> void;
	auto OnPluginBinariesChanged( //
		const TArray<struct FFileChangeData>& FileChanges
	) -> void;
	auto OnReceiveEcsactCliJsonMessage(FString Json) -> void;
	auto AddMenuEntry(class FMenuBuilder& MenuBuilder) -> void;
	auto LoadRunnerSubsystemBlueprints() -> void;

	auto OnAssetsAdded(TConstArrayView<FAssetData> Assets) -> void;
	auto OnAssetsUpdatedOnDisk(TConstArrayView<FAssetData> Assets) -> void;
	auto OnAssetsRemoved(TConstArrayView<FAssetData> Assets) -> void;
	auto OnAssetRegistryFilesLoaded() -> void;
	auto GetInstalledPluginDir() -> FString;
	auto GetEcsactSdkBinaryPath(FString BinaryName) -> FString;

public:
	auto GetEcsactCli() -> FString;

	auto SpawnEcsactCli( //
		const TArray<FString>& Args,
		FOnReceiveLine         OnReceiveLine,
		FOnExitDelegate        OnExit
	) -> void;

	static auto GetAllEcsactFiles() -> TArray<FString>;

	auto RunBuild() -> void;

	auto StartupModule() -> void override;
	auto ShutdownModule() -> void override;
	auto PreUnloadCallback() -> void override;
	auto PostLoadCallback() -> void override;
	auto SupportsDynamicReloading() -> bool override;

	static auto Get() -> FEcsactEditorModule&;
};
