#include "EcsactUnreal/EcsactSettings.h"
#include "Misc/Paths.h"

UEcsactSettings::UEcsactSettings() {
}

auto UEcsactSettings::GetEcsactRuntimeLibraryPath() const -> FString {
#if WITH_EDITORONLY_DATA
	if(bEnableBuild) {
		return FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("Binaries/Win64/EcsactRuntime.dll")
		);
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
#else
	return FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Binaries/Win64/EcsactRuntime.dll")
	);
#endif
}

#if WITH_EDITORONLY_DATA
auto UEcsactSettings::GetValidRecipes() const -> TArray<FString> {
	return Recipes.FilterByPredicate([](const FString& recipe) -> bool {
		return !recipe.TrimStartAndEnd().IsEmpty();
	});
}
#endif
