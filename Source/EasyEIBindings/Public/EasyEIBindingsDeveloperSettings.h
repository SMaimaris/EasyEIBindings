// Copyright Stylianos Maimaris. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EasyEIBindingsDeveloperSettings.generated.h"

/**
 * Project-wide defaults for Easy EI Bindings.
 * Found under Project Settings > Plugins > Easy EI Bindings.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Easy EI Bindings"))
class EASYEIBINDINGS_API UEasyEIBindingsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UEasyEIBindingsDeveloperSettings();

	static const UEasyEIBindingsDeveloperSettings* Get();

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	/** Default folder offered when creating new Input Action assets from the component details panel. */
	UPROPERTY(Config, EditAnywhere, Category = "Input Actions", meta = (ContentDir))
	FDirectoryPath DefaultInputActionPath;

	/** Trigger events enabled by default when an Input Action is added to a component. */
	UPROPERTY(Config, EditAnywhere, Category = "Input Actions",
		meta = (Bitmask, BitmaskEnum = "/Script/EnhancedInput.ETriggerEvent"))
	int32 DefaultEnabledEvents;

	/** Prefix used for new Input Action assets and stripped from asset names when deriving handler names. */
	UPROPERTY(Config, EditAnywhere, Category = "Naming")
	FString InputActionPrefix;

	/** Show the bound/missing handler summary in the component details panel. */
	UPROPERTY(Config, EditAnywhere, Category = "Editor")
	bool bShowBindingStatus;
};
