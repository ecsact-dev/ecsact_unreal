#include "EcsactEditor.h"
#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"
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

#define LOCTEXT_NAMESPACE "FEcsactEditorModule"

DEFINE_LOG_CATEGORY(EcsactEditor);

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

static auto PluginBinariesDir() -> FString {
	auto& fm = FPlatformFileManager::Get().GetPlatformFile();
	auto  plugins_dir = FPaths::ProjectPluginsDir();
	auto  plugin_dir = plugins_dir + "/" + "Ecsact";

	return FPaths::Combine( //
		FPaths::ProjectPluginsDir(),
		"Ecsact",
		"Binaries",
		PlatformBinariesDirname()
	);
}

static auto GetDirectoryWatcher() -> IDirectoryWatcher* {
	auto& watcher = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>( //
		TEXT("DirectoryWatcher")
	);
	return watcher.Get();
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
		ENamedThreads::BackgroundThreadPriority,
		[=, OnExit = std::move(OnExit)] {
			void* PipeWriteChild;
			void* PipeReadChild;
			void* PipeWriteParent;
			void* PipeReadParent;
			FPlatformProcess::CreatePipe(PipeReadParent, PipeWriteChild, false);
			FPlatformProcess::CreatePipe(PipeReadChild, PipeWriteParent, true);

			auto proc_id = uint32{};
			auto proc_handle = FPlatformProcess::CreateProc(
				TEXT("ecsact"),
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
	watcher->RegisterDirectoryChangedCallback_Handle(
		PluginBinariesDir(),
		IDirectoryWatcher::FDirectoryChanged::CreateRaw(
			this,
			&FEcsactEditorModule::OnPluginBinariesChanged
		),
		PluginBinariesWatchHandle
	);

	FEditorDelegates::OnEditorInitialized.AddRaw(
		this,
		&FEcsactEditorModule::OnEditorInitialized
	);
}

auto FEcsactEditorModule::ShutdownModule() -> void {
	UE_LOG(EcsactEditor, Warning, TEXT("Ecsact Editor Module ShutdownModule()"));

	auto* watcher = GetDirectoryWatcher();
	watcher->UnregisterDirectoryChangedCallback_Handle(
		SourceDir(),
		SourcesWatchHandle
	);
	watcher->UnregisterDirectoryChangedCallback_Handle(
		PluginBinariesDir(),
		PluginBinariesWatchHandle
	);
	SourcesWatchHandle = {};
	PluginBinariesWatchHandle = {};
	FEditorDelegates::OnEditorInitialized.RemoveAll(this);
}

auto FEcsactEditorModule::AddMenuEntry(FMenuBuilder& MenuBuilder) -> void {
	MenuBuilder.BeginSection(
		"EcsactTools",
		LOCTEXT("EcsactToolsSectionTitle", "Ecsact")
	);
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("EcsactRunCodegen", "Re-run codegen"),
			LOCTEXT(
				"EcsactRunCodegenTooltip",
				"Re-runs the ecsact codegen. This usually happens automatically and is "
				"not necessary to run manually from the menu."
			),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this] { RunCodegen(); }))
		);
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
			TEXT("Ecsact files changed. Re-generating C++ and rebuilding runtime ...")
		);

		RunCodegen();
		RunBuild();
	}
}

auto FEcsactEditorModule::OnPluginBinariesChanged(
	const TArray<FFileChangeData>& FileChanges
) -> void {
	auto any_codegen_plugins_changed = false;
	auto plugin_ext = PlatformEcsactPluginExtension();
	if(plugin_ext.IsEmpty()) {
		return;
	}

	for(auto& change : FileChanges) {
		if(!change.Filename.EndsWith(plugin_ext)) {
			continue;
		}

		auto change_basename = FPaths::GetBaseFilename(change.Filename);
		if(!change_basename.StartsWith("UnrealEditor-Ecsact")) {
			continue;
		}
		if(!change_basename.Contains("CodegenPlugin")) {
			continue;
		}

		auto dash_index = int32{};
		if(change_basename.FindLastChar('-', dash_index)) {
			auto plugin_name = change_basename.Mid(0, dash_index);
			CodegenPluginHotReloadNames.Add(plugin_name, change_basename);
		}

		any_codegen_plugins_changed = true;
	}

	if(any_codegen_plugins_changed) {
		UE_LOG(
			EcsactEditor,
			Log,
			TEXT("Ecsact codegen plugins changed. Re-generating C++ ...")
		);

		RunCodegen();
	}
}

auto FEcsactEditorModule::GetUnrealCodegenPlugins() -> TArray<FString> {
	auto& fm = FPlatformFileManager::Get().GetPlatformFile();
	auto  plugins_dir = FPaths::ProjectPluginsDir();
	auto  plugin_dir = FPaths::Combine(plugins_dir, "Ecsact");

	if(!fm.DirectoryExists(*plugin_dir)) {
		UE_LOG(
			EcsactEditor,
			Error,
			TEXT("Unable to find Ecsact Unreal integration plugin directory. Please "
					 "make sure it is installed as 'Ecsact' and not by any other name")
		);
		return {};
	}

	auto codegen_plugin_name = CodegenPluginHotReloadNames.FindRef(
		"UnrealEditor-EcsactUnrealCodegenPlugin",
		"UnrealEditor-EcsactUnrealCodegenPlugin"
	);

	auto plugin_path = FPaths::Combine(
		plugin_dir,
		"Binaries",
		PlatformBinariesDirname(),
		codegen_plugin_name + PlatformEcsactPluginExtension()
	);

	if(fm.FileExists(*plugin_path)) {
		return {plugin_path};
	}

	UE_LOG(EcsactEditor, Error, TEXT("Unable to find %s"), *plugin_path);

	return {};
}

auto FEcsactEditorModule::RunCodegen() -> void {
	auto ecsact_files = GetAllEcsactFiles();
	auto args = TArray<FString>{
		"codegen",
		"--format=json",
		"--plugin=cpp_header",
		// "--plugin=systems_header",
		// "--plugin=cpp_systems_header",
		// "--plugin=cpp_systems_source",
	};

	for(auto plugin : GetUnrealCodegenPlugins()) {
		args.Add("--plugin=" + plugin);
	}

	args.Append(ecsact_files);

	SpawnEcsactCli(
		args,
		FOnReceiveLine::CreateLambda([this](FString Line) {
			OnReceiveEcsactCliJsonMessage(Line);
		}),
		FOnExitDelegate::CreateLambda([](int32 ExitCode) -> void {
			if(ExitCode == 0) {
				UE_LOG(EcsactEditor, Log, TEXT("Ecsact codegen success"));
			} else {
				UE_LOG(
					EcsactEditor,
					Error,
					TEXT("Ecsact codegen failed with exit code %i"),
					ExitCode
				);
			}
		})
	);
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
	RunCodegen();
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
	static auto prev_valid_recipes = settings->GetValidRecipes();

	auto valid_recipes = settings->GetValidRecipes();

	if(prev_valid_recipes != valid_recipes) {
		prev_valid_recipes = valid_recipes;
		RunBuild();
	}

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
