#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EcsactRunner.h"
#include "EcsactSettings.generated.h"

UENUM()
enum class EEcsactBuildReportFilter : uint8 {
	None,
	ErrorOnly,
	ErrorsAndWarnings,
};

// clang-format off
UENUM()
enum class EEcsactRuntimeRunnerType : uint8 {
	Automatic UMETA(ToolTip = "Runner will use ecsact async methods if they are available. Otherwise the synchronous methods will be used."),
	Asynchronous UMETA(ToolTip = "Always use asynchronous methods. If async methods are not available an error will occur"),
	Custom UMETA(ToolTip = "(Advanced) Use a custom EcsactRunner class or select None to handle Ecsact execution completely manually."),
};
// clang-format on

UCLASS(Config = Ecsact, DefaultConfig)

class ECSACTEDITOR_API UEcsactSettings : public UObject {
	GENERATED_BODY()

public:
	UEcsactSettings();

	UPROPERTY(EditAnywhere, Config, Category = "Build")
	EEcsactBuildReportFilter BuildReportFilter;

	UPROPERTY(EditAnywhere, Config, Category = "Build")
	TArray<FString> Recipes;

	UPROPERTY(EditAnywhere, Config, Category = "Runtime")
	EEcsactRuntimeRunnerType Runner;

	UPROPERTY(
		EditAnywhere,
		Config,
		Category = "Runtime",
		Meta = ( //
			EditCondition = "Runner == EEcsactRuntimeRunnerType::Custom",
			EditConditionHides
		)
	)
	TSubclassOf<UEcsactRunner> CustomRunnerClass;

	auto GetValidRecipes() const -> TArray<FString>;
};
