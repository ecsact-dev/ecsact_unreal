#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(EcsactEditor, Log, All);

class FEcsactEditorModule : public IModuleInterface {
	FDelegateHandle        SourcesWatchHandle;
	FDelegateHandle        PluginBinariesWatchHandle;
	TMap<FString, FString> CodegenPluginHotReloadNames;

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
	auto GetUnrealCodegenPlugins() -> TArray<FString>;

public:
	auto SpawnEcsactCli( //
		const TArray<FString>& Args,
		FOnReceiveLine         OnReceiveLine,
		FOnExitDelegate        OnExit
	) -> void;

	auto RunCodegen() -> void;
	auto RunBuild() -> void;

	auto StartupModule() -> void override;
	auto ShutdownModule() -> void override;
	auto PreUnloadCallback() -> void override;
	auto PostLoadCallback() -> void override;
	auto SupportsDynamicReloading() -> bool override;
};
