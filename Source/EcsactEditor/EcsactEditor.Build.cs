// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

using UnrealBuildTool;
using System.IO;

public class EcsactEditor : ModuleRules {
	public EcsactEditor(ReadOnlyTargetRules Target) : base(Target) {
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] {
			// ... add public include paths required here ...
		});

		PrivateIncludePaths.AddRange(new string[] {
			// ... add other private include paths required here ...
		});

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"Engine",
			"UnrealEd",
			"Ecsact",
			"Settings",
			"PropertyEditor",
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"Json",
		});

		DynamicallyLoadedModuleNames.AddRange(new string[] {});
	}
}
