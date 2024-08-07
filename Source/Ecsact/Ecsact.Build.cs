using UnrealBuildTool;
using System.IO;
using System.Diagnostics;
using System;

[Serializable]
public class EcsactSdkNotFound : Exception {
	public EcsactSdkNotFound(Exception inner)
		: base(
				"Ecsact CLI not found in PATH. Please make sure you have the Ecsact " +
					"SDK installed. https://ecsact.dev/start",
				inner
			) {
	}
}

public class Ecsact : ModuleRules {
	public Ecsact(ReadOnlyTargetRules Target) : base(Target) {
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
		});

		if(Target.bBuildEditor) {
			PrivateDependencyModuleNames.Add("UnrealEd");
		}

		var ecsactSdkIncludeDir = GetEcsactSdkIncludeDir();
		PublicIncludePaths.Add(ecsactSdkIncludeDir);

		// NOTE: For now these APIs are loaded on module startup
		PublicDefinitions.AddRange(new string[] {
			"ECSACT_CORE_API_LOAD_AT_RUNTIME",
			"ECSACT_DYNAMIC_API_LOAD_AT_RUNTIME",
			"ECSACT_ASYNC_API_LOAD_AT_RUNTIME",
			"ECSACT_META_API_LOAD_AT_RUNTIME",
			"ECSACT_SERIALIZE_API_LOAD_AT_RUNTIME",
			"ECSACT_STATIC_API_LOAD_AT_RUNTIME",
		});
	}

	private string GetEcsactSdkIncludeDir() {
		var includePath = "";

		try {
			ProcessStartInfo startInfo = new ProcessStartInfo();
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
