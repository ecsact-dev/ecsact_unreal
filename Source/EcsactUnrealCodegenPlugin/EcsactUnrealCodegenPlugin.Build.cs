// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

using UnrealBuildTool;
using System.IO;
using System.Diagnostics;
using System;

public class EcsactUnrealCodegenPlugin : ModuleRules {
	public EcsactUnrealCodegenPlugin(ReadOnlyTargetRules Target) : base(Target) {
		PCHUsage = PCHUsageMode.NoPCHs;

		// No unreal dependencies for Ecsact plugins
		PublicDependencyModuleNames.AddRange(new string[] {});
		bUseRTTI = false;
		bEnableExceptions = false;

		var EcsactSdkIncludeDir = GetEcsactSdkIncludeDir();
		PublicIncludePaths.Add(EcsactSdkIncludeDir);

		// Codegen plugins utilise the ecsact 'meta' module
		PrivateDefinitions.AddRange(new string[] {
			"ECSACT_META_API_LOAD_AT_RUNTIME",
			"ECSACT_META_API_EXPORT",
		});
	}

	private string GetEcsactSdkIncludeDir() {
		var includePath = "";
		var startInfo = new ProcessStartInfo();
		startInfo.FileName = GetEcsactSdkBinary("ecsact");
		startInfo.Arguments = "config include_dir";
		startInfo.RedirectStandardOutput = true;
		startInfo.UseShellExecute = false;
		startInfo.CreateNoWindow = true;

		using(Process process = Process.Start(startInfo)) {
			using(StreamReader reader = process.StandardOutput) {
				includePath = reader.ReadToEnd().Trim();
			}
		}

		return includePath;
	}

	private string GetEcsactSdkBinary(string binaryName) {
		var thirdPartyEcsactSdk =
			Path.Combine(PluginDirectory, "ThirdParty/EcsactSDK");
		var exePath = Path.Combine(thirdPartyEcsactSdk, $"bin/{binaryName}.exe");
		if(File.Exists(exePath)) {
			return exePath;
		}

		return binaryName;
	}
}
