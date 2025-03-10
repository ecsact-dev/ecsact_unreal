// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#include "EcsactEditor.h"
#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"
#include "Interfaces/IPluginManager.h"
#include "CoreGlobals.h"
#include "EcsactUnreal/Ecsact.h"
#include "Editor.h"
#include "Engine/GameViewportClient.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "ISettingsContainer.h"
#include "LevelEditor.h"
#include "HAL/PlatformFileManager.h"
#include "DirectoryWatcherModule.h"
#include "IDirectoryWatcher.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Serialization/JsonReader.h"
#include "EcsactUnreal/EcsactSettings.h"
#include "EcsactUnreal/EcsactRunnerSubsystem.h"
#include "Engine/Blueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "FEcsactEditorModule"

DEFINE_LOG_CATEGORY(EcsactEditor);

// clang-format off
// We track some settings to detect if they change
static decltype(GetDefault<UEcsactSettings>()->GetValidRecipes()) prev_valid_recipes = {};
static auto prev_auto_subsystem = false;
// clang-format on

static auto UpdatePreviousSettings() -> void {
	auto settings = GetDefault<UEcsactSettings>();
	prev_valid_recipes = settings->GetValidRecipes();
	prev_auto_subsystem = settings->bAutoCollectBlueprintRunnerSubsystems;
}

static auto SourceDir() -> FString {
	return FPaths::Combine(FPaths::ProjectDir(), "Source");
}

static auto PlatformBinariesDirname() -> FString {
	auto platform_name = FString{FPlatformProperties::PlatformName()};

	if(platform_name.Contains(TEXT("Windows"), ESearchCase::IgnoreCase)) {
		return TEXT("Win64");
	} else if(platform_name.Equals(TEXT("Mac"), ESearchCase::IgnoreCase)) {
		return TEXT("Mac");
	} else if(platform_name.Equals(TEXT("Linux"), ESearchCase::IgnoreCase)) {
		return TEXT("Linux");
	}

	UE_LOG(
		EcsactEditor,
		Error,
		TEXT("Failed to get platform binaries directory name from platform: %s"),
		*platform_name
	);

	return TEXT("Unknown");
}

static auto PlatformEcsactPluginExtension() -> FString {
	auto platform_name = FString{FPlatformProperties::PlatformName()};

	if(platform_name.Contains(TEXT("Windows"), ESearchCase::IgnoreCase)) {
		return TEXT(".dll");
	} else if(platform_name.Equals(TEXT("Mac"), ESearchCase::IgnoreCase)) {
		return TEXT(".dylib");
	} else if(platform_name.Equals(TEXT("Linux"), ESearchCase::IgnoreCase)) {
		return TEXT(".so");
	}

	UE_LOG(
		EcsactEditor,
		Error,
		TEXT("Failed to get platform Ecsact plugin extension name from platform: %s"
		),
		*platform_name
	);

	return TEXT("");
}

static auto GetDirectoryWatcher() -> IDirectoryWatcher* {
	auto& watcher = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>( //
		TEXT("DirectoryWatcher")
	);
	return watcher.Get();
}

auto FEcsactEditorModule::GetInstalledPluginDir() -> FString {
	auto Plugin = IPluginManager::Get().FindPlugin(TEXT("Ecsact"));
	if(Plugin.IsValid()) {
		return Plugin->GetBaseDir();
	}
	return FString{};
}

auto FEcsactEditorModule::GetEcsactSdkBinaryPath( //
	FString BinaryName
) -> FString {
	auto plugin_install_dir = GetInstalledPluginDir();
	if(plugin_install_dir.IsEmpty()) {
		return BinaryName;
	}

	auto ecsact_sdk_dir = FPaths::Combine( //
		plugin_install_dir,
		"Source",
		"ThirdParty",
		"EcsactSDK"
	);
	if(!FPaths::DirectoryExists(ecsact_sdk_dir)) {
		return BinaryName;
	}

#ifdef _WIN32
	FString exe_suffix = ".exe";
#else
	FString exe_suffix = "";
#endif

	auto binary_path = FPaths::Combine( //
		ecsact_sdk_dir,
		"bin",
		BinaryName + exe_suffix
	);
	if(!FPaths::FileExists(binary_path)) {
		return BinaryName;
	}

	return binary_path;
}

auto FEcsactEditorModule::GetEcsactCli() -> FString {
	return GetEcsactSdkBinaryPath("ecsact");
}

auto FEcsactEditorModule::SpawnEcsactCli(
	const TArray<FString>& Args,
	FOnReceiveLine         OnReceiveLine,
	FOnExitDelegate        OnExit
) -> void {
	check(OnExit.IsBound());

	auto args_str = FString{};
	for(const auto& arg : Args) {
		args_str += "\"" + arg + "\" ";
	}

	AsyncTask(
		ENamedThreads::AnyBackgroundThreadNormalTask,
		[=, this, OnExit = std::move(OnExit)] {
			void* PipeWriteChild;
			void* PipeReadChild;
			void* PipeWriteParent;
			void* PipeReadParent;
			FPlatformProcess::CreatePipe(PipeReadParent, PipeWriteChild, false);
			FPlatformProcess::CreatePipe(PipeReadChild, PipeWriteParent, true);

			auto proc_id = uint32{};
			auto proc_handle = FPlatformProcess::CreateProc(
				*GetEcsactCli(),
				*args_str,
				false,
				true,
				true,
				&proc_id,
				0,
				nullptr,
				PipeWriteChild,
				PipeReadChild
			);

			auto read_buf = FString{};
			auto read_line = [&]() -> FString {
				read_buf += FPlatformProcess::ReadPipe(PipeReadParent);
				auto nl_index = int32{};
				if(read_buf.FindChar(TCHAR('\n'), nl_index)) {
					auto line = read_buf.Mid(0, nl_index);
					read_buf = read_buf.Mid(nl_index + 1);
					return line;
				}
				return "";
			};

			for(auto line = read_line();
					!line.IsEmpty() || FPlatformProcess::IsProcRunning(proc_handle);
					line = read_line()) {
				if(line.IsEmpty()) {
					continue;
				}

				AsyncTask(ENamedThreads::GameThread, [=] {
					OnReceiveLine.Execute(line);
				});
			}

			auto exit_code = int32{};
			FPlatformProcess::GetProcReturnCode(proc_handle, &exit_code);

			FPlatformProcess::ClosePipe(PipeReadChild, PipeWriteParent);
			FPlatformProcess::ClosePipe(PipeReadParent, PipeWriteChild);
			FPlatformProcess::CloseProc(proc_handle);

			AsyncTask(ENamedThreads::GameThread, [=, OnExit = std::move(OnExit)] {
				OnExit.Execute(exit_code);
			});
		}
	);
}

auto FEcsactEditorModule::StartupModule() -> void {
	auto& settings_module =
		FModuleManager::GetModuleChecked<ISettingsModule>("Settings");
	auto settings_container = settings_module.GetContainer("Project");
	auto settings_section = settings_module.RegisterSettings(
		"Project",
		"Plugins",
		"Ecsact",
		LOCTEXT("EcsactSettingsName", "Ecsact"),
		LOCTEXT("EcsactSettingsDescription", "Configuration settings for Ecsact"),
		GetMutableDefault<UEcsactSettings>()
	);
	check(settings_section.IsValid());

	settings_section->OnModified().BindRaw(
		this,
		&FEcsactEditorModule::OnEcsactSettingsModified
	);

	auto& level_editor_module =
		FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<FExtender> menu_extender = MakeShareable(new FExtender());
	menu_extender->AddMenuExtension(
		"Tools",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(this, &FEcsactEditorModule::AddMenuEntry)
	);

	level_editor_module.GetMenuExtensibilityManager()->AddExtender(menu_extender);

	auto* watcher = GetDirectoryWatcher();
	watcher->RegisterDirectoryChangedCallback_Handle(
		SourceDir(),
		IDirectoryWatcher::FDirectoryChanged::CreateRaw(
			this,
			&FEcsactEditorModule::OnProjectSourcesChanged
		),
		SourcesWatchHandle
	);

	FEditorDelegates::OnEditorInitialized.AddRaw(
		this,
		&FEcsactEditorModule::OnEditorInitialized
	);

	// clang-format off
	auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	auto& AssetRegistry = AssetRegistryModule.Get();
	// clang-format on

	AssetRegistry.OnAssetsAdded().AddRaw(
		this,
		&FEcsactEditorModule::OnAssetsAdded
	);
	AssetRegistry.OnAssetsRemoved().AddRaw(
		this,
		&FEcsactEditorModule::OnAssetsRemoved
	);
	AssetRegistry.OnAssetsUpdatedOnDisk().AddRaw(
		this,
		&FEcsactEditorModule::OnAssetsUpdatedOnDisk
	);
	AssetRegistry.OnFilesLoaded().AddRaw(
		this,
		&FEcsactEditorModule::OnAssetRegistryFilesLoaded
	);

	// Set the initial previous settings to start detection
	UpdatePreviousSettings();
}

auto FEcsactEditorModule::ShutdownModule() -> void {
	UE_LOG(EcsactEditor, Warning, TEXT("Ecsact Editor Module ShutdownModule()"));

	auto* watcher = GetDirectoryWatcher();
	watcher->UnregisterDirectoryChangedCallback_Handle(
		SourceDir(),
		SourcesWatchHandle
	);
	SourcesWatchHandle = {};
	FEditorDelegates::OnEditorInitialized.RemoveAll(this);
}

auto FEcsactEditorModule::LoadRunnerSubsystemBlueprints() -> void {
	auto* settings = GetMutableDefault<UEcsactSettings>();
	auto  settings_changed = false;

	// clang-format off
	auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	auto& AssetRegistry = AssetRegistryModule.Get();
	// clang-format on

	auto filter = FARFilter{};
	filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	filter.bRecursiveClasses = true;
	filter.bRecursivePaths = true;

	auto asset_list = TArray<FAssetData>{};
	AssetRegistry.GetAssets(filter, asset_list);

	if(!asset_list.IsEmpty()) {
		OnAssetsAdded(asset_list);
	}
}

auto FEcsactEditorModule::OnAssetsAdded( //
	TConstArrayView<FAssetData> Assets
) -> void {
	auto* settings = GetMutableDefault<UEcsactSettings>();
	auto  settings_changed = false;

	if(!settings->bAutoCollectBlueprintRunnerSubsystems) {
		return;
	}

	for(auto& asset : Assets) {
		auto generated_tag = asset.TagsAndValues.FindTag(TEXT("GeneratedClass"));
		if(!generated_tag.IsSet()) {
			continue;
		}

		auto generated_class = generated_tag.GetValue();
		auto class_object_path =
			FPackageName::ExportTextPathToObjectPath(*generated_class);
		auto blueprint = Cast<UBlueprint>(asset.GetAsset());

		if(blueprint) {
			auto parent = blueprint->ParentClass;
			if(parent && parent->IsChildOf(UEcsactRunnerSubsystem::StaticClass())) {
				auto subsystem_soft_class_ptr = TSoftClassPtr<UEcsactRunnerSubsystem>(
					FSoftObjectPath{class_object_path}
				);

				auto index =
					settings->RunnerSubsystems.AddUnique(subsystem_soft_class_ptr);
				if(index == settings->RunnerSubsystems.Num() - 1) {
					UE_LOG(
						EcsactEditor,
						Log,
						TEXT("Added blueprint Ecsact runner subsystem: %s"),
						*class_object_path
					);
					settings_changed = true;
				}
			}
		}
	}

	if(settings_changed) {
		settings->TryUpdateDefaultConfigFile();
	}
}

auto FEcsactEditorModule::OnAssetsUpdatedOnDisk(
	TConstArrayView<FAssetData> Assets
) -> void {
}

auto FEcsactEditorModule::OnAssetsRemoved( //
	TConstArrayView<FAssetData> Assets
) -> void {
	auto* settings = GetMutableDefault<UEcsactSettings>();
	auto  settings_changed = false;

	if(!settings->bAutoCollectBlueprintRunnerSubsystems) {
		return;
	}

	for(auto& asset : Assets) {
		auto generated_tag = asset.TagsAndValues.FindTag(TEXT("GeneratedClass"));
		if(!generated_tag.IsSet()) {
			continue;
		}

		auto generated_class = generated_tag.GetValue();
		auto class_object_path =
			FPackageName::ExportTextPathToObjectPath(*generated_class);
		auto blueprint = Cast<UBlueprint>(asset.FastGetAsset());

		if(blueprint) {
			auto parent = blueprint->ParentClass;
			if(parent && parent->IsChildOf(UEcsactRunnerSubsystem::StaticClass())) {
				auto subsystem_soft_class_ptr = TSoftClassPtr<UEcsactRunnerSubsystem>(
					FSoftObjectPath{class_object_path}
				);

				auto removed =
					settings->RunnerSubsystems.Remove(subsystem_soft_class_ptr);
				if(removed > 0) {
					settings_changed = true;
					UE_LOG(
						EcsactEditor,
						Log,
						TEXT("Removed blueprint Ecsact runner subsystem: %s"),
						*class_object_path
					);
				}
			}
		}
	}

	if(settings_changed) {
		settings->TryUpdateDefaultConfigFile();
	}
}

auto FEcsactEditorModule::OnAssetRegistryFilesLoaded() -> void {
}

auto FEcsactEditorModule::AddMenuEntry(FMenuBuilder& MenuBuilder) -> void {
	MenuBuilder.BeginSection(
		"EcsactTools",
		LOCTEXT("EcsactToolsSectionTitle", "Ecsact")
	);
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("EcsactRebuild", "Rebuild runtime"),
			LOCTEXT(
				"EcsactRebuildTooltip",
				"Rebuild the Ecsact runtime. This usually happens automatically and is "
				"not necessary to run manually from the menu."
			),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this] { RunBuild(); }))
		);
	}
	MenuBuilder.EndSection();
}

auto FEcsactEditorModule::OnProjectSourcesChanged(
	const TArray<FFileChangeData>& FileChanges
) -> void {
	auto any_ecsact_files_changed = false;
	for(auto& change : FileChanges) {
		if(!change.Filename.EndsWith(".ecsact")) {
			continue;
		}

		any_ecsact_files_changed = true;
	}

	if(any_ecsact_files_changed) {
		UE_LOG(
			EcsactEditor,
			Log,
			TEXT("Ecsact files changed. Rebuilding runtime ...")
		);

		RunBuild();
	}
}

auto FEcsactEditorModule::RunBuild() -> void {
	const auto* settings = GetDefault<UEcsactSettings>();

	if(!settings->bEnableBuild) {
		return;
	}

	auto ecsact_runtime_path = settings->GetEcsactRuntimeLibraryPath();
	auto temp_dir = FPaths::ConvertRelativePathToFull(FPaths::Combine(
		FPaths::ProjectIntermediateDir(),
		"Temp",
		FPaths::CreateTempFilename(TEXT("EcsactBuild"))
	));
	auto recipes = settings->GetValidRecipes();
	auto ecsact_files = GetAllEcsactFiles();

	if(ecsact_files.IsEmpty()) {
		UE_LOG(
			EcsactEditor,
			Warning,
			TEXT("There are no .ecsact files found in your Source directory")
		);
		return;
	}

	if(recipes.IsEmpty()) {
		FMessageDialog::Open(
			EAppMsgType::Ok,
			LOCTEXT(
				"NoRecipesErrorMessage",
				"There are no recipes configured in your Ecsact plugin settings."
			)
		);
		return;
	}

	auto args = TArray<FString>{
		"build",
		"--format=json",
		"-o",
		ecsact_runtime_path,
		"--temp=" + temp_dir,
		"--debug",
	};

	switch(settings->BuildReportFilter) {
		case EEcsactBuildReportFilter::None:
			break;
		case EEcsactBuildReportFilter::ErrorOnly:
			args.Push("--report_filter=error_only");
			break;
		case EEcsactBuildReportFilter::ErrorsAndWarnings:
			args.Push("--report_filter=errors_and_warnings");
			break;
	}

	for(const auto& recipe : recipes) {
		args.Push("--recipe=" + recipe);
	}

	args.Append(ecsact_files);

	SpawnEcsactCli(
		args,
		FOnReceiveLine::CreateLambda([this](FString Line) {
			OnReceiveEcsactCliJsonMessage(Line);
		}),
		FOnExitDelegate::CreateLambda([](int32 ExitCode) -> void {
			if(ExitCode == 0) {
				UE_LOG(EcsactEditor, Log, TEXT("Ecsact build success"));
			} else {
				UE_LOG(
					EcsactEditor,
					Error,
					TEXT("Ecsact build failed with exit code %i"),
					ExitCode
				);
			}
		})
	);
}

auto FEcsactEditorModule::PreUnloadCallback() -> void {
	UE_LOG(
		EcsactEditor,
		Warning,
		TEXT("Ecsact Editor Module PreUnloadCallback()")
	);
}

auto FEcsactEditorModule::PostLoadCallback() -> void {
	UE_LOG(
		EcsactEditor,
		Warning,
		TEXT("Ecsact Editor Module PostLoadCallback()")
	);
}

auto FEcsactEditorModule::SupportsDynamicReloading() -> bool {
	return true;
}

auto FEcsactEditorModule::OnEditorInitialized(double Duration) -> void {
	RunBuild();
}

auto FEcsactEditorModule::OnReceiveEcsactCliJsonMessage(FString Json) -> void {
	auto json_object = TSharedPtr<FJsonObject>{};
	auto reader = TJsonReaderFactory<>::Create(Json);

	if(!FJsonSerializer::Deserialize(reader, json_object)) {
		UE_LOG(
			EcsactEditor,
			Error,
			TEXT("Failed to parse JSON message: %s"),
			*Json
		);
		return;
	}
	if(!json_object.IsValid()) {
		return;
	}

	auto message_type = json_object->GetStringField(TEXT("type"));

	if(message_type == "info") {
		UE_LOG(
			EcsactEditor,
			Log,
			TEXT("%s"),
			*json_object->GetStringField(TEXT("content"))
		);
	} else if(message_type == "error") {
		UE_LOG(
			EcsactEditor,
			Error,
			TEXT("%s"),
			*json_object->GetStringField(TEXT("content"))
		);
	} else if(message_type == "warning") {
		UE_LOG(
			EcsactEditor,
			Warning,
			TEXT("%s"),
			*json_object->GetStringField(TEXT("content"))
		);
	} else if(message_type == "subcommand_start") {
		UE_LOG(
			EcsactEditor,
			Log,
			TEXT("subcommand(%i): %s"),
			json_object->GetIntegerField(TEXT("id")),
			*json_object->GetStringField(TEXT("executable"))
		);
	} else if(message_type == "subcommand_end") {
		UE_LOG(
			EcsactEditor,
			Log,
			TEXT("subcommand(%i): exit code %i"),
			json_object->GetIntegerField(TEXT("id")),
			json_object->GetIntegerField(TEXT("exit_code"))
		);
	} else if(message_type == "subcommand_stdout") {
		UE_LOG(
			EcsactEditor,
			Log,
			TEXT("subcommand(%i): %s"),
			json_object->GetIntegerField(TEXT("id")),
			*json_object->GetStringField(TEXT("line"))
		);
	} else if(message_type == "subcommand_stderr") {
		UE_LOG(
			EcsactEditor,
			Error,
			TEXT("subcommand(%i): %s"),
			json_object->GetIntegerField(TEXT("id")),
			*json_object->GetStringField(TEXT("line"))
		);
	} else if(message_type == "success") {
		UE_LOG(
			EcsactEditor,
			Log,
			TEXT("%s"),
			*json_object->GetStringField(TEXT("content"))
		);
	} else {
		UE_LOG(
			EcsactEditor,
			Warning,
			TEXT("Unhandled Message (%s): %s"),
			*message_type,
			*Json
		);
	}
}

auto FEcsactEditorModule::OnEcsactSettingsModified() -> bool {
	const auto* settings = GetDefault<UEcsactSettings>();

	if(prev_auto_subsystem != settings->bAutoCollectBlueprintRunnerSubsystems) {
		if(settings->bAutoCollectBlueprintRunnerSubsystems) {
			LoadRunnerSubsystemBlueprints();
		}
	}

	if(prev_valid_recipes != settings->GetValidRecipes()) {
		RunBuild();
	}

	UpdatePreviousSettings();
	return true;
}

auto FEcsactEditorModule::Get() -> FEcsactEditorModule& {
	return FModuleManager::Get().GetModuleChecked<FEcsactEditorModule>(
		"EcsactEditor"
	);
}

auto FEcsactEditorModule::GetAllEcsactFiles() -> TArray<FString> {
	auto& fm = FPlatformFileManager::Get().GetPlatformFile();
	auto  files = TArray<FString>{};
	fm.FindFilesRecursively(files, *SourceDir(), TEXT(".ecsact"));
	return files;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FEcsactEditorModule, EcsactEditor)
