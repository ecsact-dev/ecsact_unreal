#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EcsactRunner.h"
#include "EcsactSettings.generated.h"

#if WITH_EDITOR
UENUM()
enum class EEcsactBuildReportFilter : uint8 {
	None,
	ErrorOnly,
	ErrorsAndWarnings,
};
#endif

// clang-format off
UENUM()
enum class EEcsactRuntimeRunnerType : uint8 {
	Automatic UMETA(ToolTip = "Runner will use ecsact async methods if they are available. Otherwise the synchronous methods will be used."),
	Asynchronous UMETA(ToolTip = "Always use asynchronous methods. If async methods are not available an error will occur"),
	Custom UMETA(ToolTip = "(Advanced) Use a custom EcsactRunner class or select None to handle Ecsact execution completely manually."),
};
// clang-format on

UCLASS(Config = Ecsact, DefaultConfig)

class ECSACT_API UEcsactSettings : public UObject {
	GENERATED_BODY()

public:
	UEcsactSettings();

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Config, Category = "Build")
	EEcsactBuildReportFilter BuildReportFilter;

	UPROPERTY(EditAnywhere, Config, Category = "Build")
	TArray<FString> Recipes;

	auto GetValidRecipes() const -> TArray<FString>;

#endif

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
};