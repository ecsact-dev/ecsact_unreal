#include "EcsactUnreal/EcsactSettings.h"

UEcsactSettings::UEcsactSettings() {
}

#if WITH_EDITORONLY_DATA
auto UEcsactSettings::GetValidRecipes() const -> TArray<FString> {
	return Recipes.FilterByPredicate([](const FString& recipe) -> bool {
		return !recipe.TrimStartAndEnd().IsEmpty();
	});
}
#endif
