using UnrealBuildTool;
using System.IO;
using System.Diagnostics;
using System;
using System.Collections.Generic;
using EpicGames.Core;

class EcsactSdkNotFound : Exception {
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

		var EcsactSdkVersion = GetEcsactSdkVersion();
		Environment.SetEnvironmentVariable(
			"EcsactPlugin_SdkVersion",
			EcsactSdkVersion
		);

		var EcsactSdkIncludeDir = GetEcsactSdkIncludeDir();
		PublicIncludePaths.Add(EcsactSdkIncludeDir);

		// NOTE: For now these APIs are loaded on module startup
		PublicDefinitions.AddRange(new string[] {
			"ECSACT_CORE_API_LOAD_AT_RUNTIME",
			"ECSACT_DYNAMIC_API_LOAD_AT_RUNTIME",
			"ECSACT_ASYNC_API_LOAD_AT_RUNTIME",
			"ECSACT_META_API_LOAD_AT_RUNTIME",
			"ECSACT_SERIALIZE_API_LOAD_AT_RUNTIME",
			"ECSACT_STATIC_API_LOAD_AT_RUNTIME",
			"ECSACT_SI_WASM_API_LOAD_AT_RUNTIME",
		});
		PrivateDefinitions.AddRange(new string[] {
			"ECSACT_CORE_API_EXPORT",
			"ECSACT_DYNAMIC_API_EXPORT",
			"ECSACT_ASYNC_API_EXPORT",
			"ECSACT_META_API_EXPORT",
			"ECSACT_SERIALIZE_API_EXPORT",
			"ECSACT_STATIC_API_EXPORT",
			"ECSACT_SI_WASM_API_EXPORT",
		});
	}

	private string[] GetEcsactSources() {
		var projectRoot = GetProjectRoot(Target);
		return Directory.GetFiles(
			Path.Combine(projectRoot.FullName, "Source"),
			"*.ecsact",
			SearchOption.AllDirectories
		);
	}

	private string GetEcsactSdkVersion() {
		var version = "";

		try {
			var startInfo = new ProcessStartInfo();
			startInfo.FileName = "ecsact";
			startInfo.Arguments = "--version";
			startInfo.RedirectStandardOutput = true;
			startInfo.UseShellExecute = false;
			startInfo.CreateNoWindow = true;

			using(Process process = Process.Start(startInfo)) {
				using(StreamReader reader = process.StandardOutput) {
					version = reader.ReadToEnd().Trim();
				}
			}
		} catch(Exception err) {
			throw new EcsactSdkNotFound(err);
		}

		if(String.IsNullOrEmpty(version)) {
			throw new EcsactSdkNotFound(null);
		}

		return version;
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

	private void ExecEcsactCli(IList<string> Args) {
		try {
			var StartInfo = new ProcessStartInfo();
			StartInfo.FileName = "ecsact";
			foreach(var Arg in Args) {
				StartInfo.ArgumentList.Add(Arg);
			}
			StartInfo.UseShellExecute = false;
			StartInfo.CreateNoWindow = true;
			StartInfo.RedirectStandardOutput = true;
			StartInfo.RedirectStandardError = true;
			var Proc = Process.Start(StartInfo);

			var Line = "";
			while((Line = Proc.StandardOutput.ReadLine()) != null) {
				Console.WriteLine(Line);
			}

			Proc.WaitForExit();
			if(Proc.ExitCode != 0) {
				throw new BuildException("ecsact exited with code: " + Proc.ExitCode);
			}
		} catch(BuildException err) {
			throw err;
		} catch(Exception err) {
			throw new EcsactSdkNotFound(err);
		}
	}

	private DirectoryReference GetProjectRoot(ReadOnlyTargetRules Target) {
		if(Target.ProjectFile != null) {
			return Target.ProjectFile.Directory;
		}

		// Without a Target.ProjectFile we're probably installed as an Engine
		// plugin. Only information we have about the project is the current
		// directory.
		var Root = new DirectoryReference(Directory.GetCurrentDirectory());
		while(Root != null &&
					!File.Exists(Path.Combine(Root.FullName, "*.uproject"))) {
			Root = Root.ParentDirectory;
		}
		return Root;
	}
}
