#include "EcsactSettings.h"

UEcsactSettings::UEcsactSettings() {
}

auto UEcsactSettings::GetValidRecipes() const -> TArray<FString> {
	return Recipes.FilterByPredicate([](const FString& recipe) -> bool {
		return !recipe.TrimStartAndEnd().IsEmpty();
	});
}
