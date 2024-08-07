#include "EcsactEditor.h"
#include "Async/TaskGraphInterfaces.h"
#include "Editor.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "ISettingsContainer.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManagerGeneric.h"
#include "FileHelpers.h"
#include "DirectoryWatcherModule.h"
#include "IDirectoryWatcher.h"
#include "Json.h"
#include "EcsactSettings.h"

#define LOCTEXT_NAMESPACE "FEcsactEditorModule"

DEFINE_LOG_CATEGORY(EcsactEditor);

static auto SourceDir() -> FString {
	return FPaths::Combine(FPaths::ProjectDir(), "Source");
}

static auto GetDirectoryWatcher() -> IDirectoryWatcher* {
	auto& watcher = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>( //
		TEXT("DirectoryWatcher")
	);
	return watcher.Get();
}

static auto GetAllEcsactFiles() -> TArray<FString> {
	auto& fm = FPlatformFileManager::Get().GetPlatformFile();
	auto  files = TArray<FString>{};
	fm.FindFilesRecursively(files, *SourceDir(), TEXT(".ecsact"));
	return files;
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
			TEXT("Ecsact files changed. Re-generating C++ ...")
		);

		RunCodegen();
		RunBuild();
	}
}

auto FEcsactEditorModule::RunCodegen() -> void {
	auto ecsact_files = GetAllEcsactFiles();
	auto args = TArray<FString>{
		"codegen",
		"--format=json",
		"--plugin=cpp_header",
		"--plugin=systems_header",
		"--plugin=cpp_systems_header",
		"--plugin=cpp_systems_source",
	};
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

	auto ecsact_runtime_path = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Binaries/Win64/EcsactRuntime.dll")
	);
	auto temp_dir = FPaths::CreateTempFilename(TEXT("EcsactBuild"));

	auto ecsact_files = GetAllEcsactFiles();
	auto args = TArray<FString>{
		"build",
		"--format=json",
		"--recipe=rt_entt",
		"-o",
		ecsact_runtime_path,
		"--temp=" + temp_dir,
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
	return true;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FEcsactEditorModule, EcsactEditor)
