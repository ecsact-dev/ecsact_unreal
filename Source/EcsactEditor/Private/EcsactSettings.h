#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EcsactSettings.generated.h"

UENUM()
enum class EEcsactBuildReportFilter : uint8 {
	None,
	ErrorOnly,
	ErrorsAndWarnings,
};

UCLASS(Config = Ecsact, DefaultConfig)

class UEcsactSettings : public UObject {
	GENERATED_BODY()

public:
	UEcsactSettings();

	UPROPERTY(EditAnywhere, Config, Category = Ecsact)
	EEcsactBuildReportFilter BuildReportFilter;

	UPROPERTY(EditAnywhere, Config, Category = Ecsact)
	TArray<FString> Recipes;

	auto GetValidRecipes() const -> TArray<FString>;
};
