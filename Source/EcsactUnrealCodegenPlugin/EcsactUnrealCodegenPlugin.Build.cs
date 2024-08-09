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
		});
	}

	private string GetEcsactSdkIncludeDir() {
		var includePath = "";

		try {
			var startInfo = new ProcessStartInfo();
			startInfo.FileName = "ecsact";
			startInfo.Arguments = "config include_dir";
			startInfo.RedirectStandardOutput = true;
			startInfo.UseShellExecute = false;
			startInfo.CreateNoWindow = true;

			using(Process process = Process.Start(startInfo)) {
				using(StreamReader reader = process.StandardOutput) {
					includePath = reader.ReadToEnd().Trim();
				}
			}
		} catch(Exception err) {
			throw new EcsactSdkNotFound(err);
		}

		return includePath;
	}
}
