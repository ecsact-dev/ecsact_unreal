// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

#include "EcsactUnreal/EcsactSettings.h"
#include "Misc/Paths.h"

UEcsactSettings::UEcsactSettings() {
}

auto UEcsactSettings::GetDefaultEcsactRuntimeLibraryPath() const -> FString {
	return FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Binaries/Win64/EcsactRuntime.dll")
	);
}

auto UEcsactSettings::GetEcsactRuntimeLibraryPath() const -> FString {
#if WITH_EDITORONLY_DATA
	if(bEnableBuild) {
		return GetDefaultEcsactRuntimeLibraryPath();
	}
#endif

	if(CustomEcsactRuntimeLibraryPath.IsEmpty()) {
		return GetDefaultEcsactRuntimeLibraryPath();
	} else {
		if(!FPaths::IsRelative(CustomEcsactRuntimeLibraryPath)) {
			return CustomEcsactRuntimeLibraryPath;
		} else {
			return FPaths::Combine(
				FPaths::ProjectDir(),
				CustomEcsactRuntimeLibraryPath
			);
		}
	}
}

#if WITH_EDITORONLY_DATA
auto UEcsactSettings::GetValidRecipes() const -> TArray<FString> {
	return Recipes.FilterByPredicate([](const FString& recipe) -> bool {
		return !recipe.TrimStartAndEnd().IsEmpty();
	});
}
#endif
