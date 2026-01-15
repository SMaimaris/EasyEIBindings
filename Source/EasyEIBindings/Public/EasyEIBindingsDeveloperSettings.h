// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EasyEIBindingsDeveloperSettings.generated.h"

/**
 * Developer settings for EasyEI Bindings plugin configuration.
 */
UCLASS(Config=EditorPerProjectUserSettings, meta=(DisplayName="Easy EI Bindings"))
class EASYEIBINDINGS_API UEasyEIBindingsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UEasyEIBindingsDeveloperSettings();

	static const UEasyEIBindingsDeveloperSettings* Get();

	UPROPERTY(Config, EditAnywhere, Category = "Input Actions", meta = (ContentDir))
	FDirectoryPath DefaultInputActionPath;

	UPROPERTY(Config, EditAnywhere, Category = "Input Actions",
		meta = (Bitmask, BitmaskEnum = "/Script/EnhancedInput.ETriggerEvent"))
	int32 DefaultEnabledEvents;

	UPROPERTY(Config, EditAnywhere, Category = "Naming")
	FString InputActionPrefix;

	UPROPERTY(Config, EditAnywhere, Category = "Code Generation")
	bool bGenerateBlueprintEvents;

	UPROPERTY(Config, EditAnywhere, Category = "Editor")
	bool bShowBindingStatus;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
