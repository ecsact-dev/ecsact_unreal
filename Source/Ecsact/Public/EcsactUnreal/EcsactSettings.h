#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EcsactUnreal/EcsactRunner.h"
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

class ECSACT_API UEcsactSettings : public UObject {
	GENERATED_BODY() // NOLINT

public:
	UEcsactSettings();

#if WITH_EDITORONLY_DATA

	UPROPERTY(EditAnywhere, Config, Category = "Build")
	bool bEnableBuild = false;

#endif

	/**
	 * This path is used when not using the built-in ecsact recipe build system.
	 * (i.e. bEnableBuild is false)
	 */
	UPROPERTY(
		EditAnywhere,
		Config,
		Category = "Build",
		Meta = ( //
			EditCondition = "!bEnableBuild",
			EditConditionHides
		)
	)
	FString CustomEcsactRuntimeLibraryPath;

#if WITH_EDITORONLY_DATA

	UPROPERTY(
		EditAnywhere,
		Config,
		Category = "Build",
		Meta = ( //
			EditCondition = "bEnableBuild",
			EditConditionHides
		)
	)
	EEcsactBuildReportFilter BuildReportFilter;

	UPROPERTY(
		EditAnywhere,
		Config,
		Category = "Build",
		Meta = ( //
			EditCondition = "bEnableBuild",
			EditConditionHides
		)
	)
	TArray<FString> Recipes;

	auto GetValidRecipes() const -> TArray<FString>;

#endif // WITH_EDITORONLY_DATA

	auto GetEcsactRuntimeLibraryPath() const -> FString;
	auto GetDefaultEcsactRuntimeLibraryPath() const -> FString;

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

	UPROPERTY(EditAnywhere, Config, Category = "Runtime")
	bool bAutoCollectBlueprintRunnerSubsystems = true;

	UPROPERTY(
		EditAnywhere,
		Config,
		Category = "Runtime",
		Meta = ( //
			EditCondition = "!bAutoCollectBlueprintRunnerSubsystems",
			AllowedClasses = "/Script/Ecsact.EcsactRunnerSubsystem"
		)
	)
	TArray<TSoftClassPtr<class UEcsactRunnerSubsystem>> RunnerSubsystems;
};
