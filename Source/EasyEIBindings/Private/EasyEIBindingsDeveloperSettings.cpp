// Copyright Stylianos Maimaris. All Rights Reserved.


#include "EasyEIBindingsDeveloperSettings.h"


UEasyEIBindingsDeveloperSettings::UEasyEIBindingsDeveloperSettings()
{
	DefaultInputActionPath.Path = TEXT("/Game/Input");
	InputActionPrefix = TEXT("IA_");

	DefaultEnabledEvents = 17;

	bGenerateBlueprintEvents = false;
	bShowBindingStatus = true;
}

const UEasyEIBindingsDeveloperSettings* UEasyEIBindingsDeveloperSettings::Get()
{
	return GetDefault<UEasyEIBindingsDeveloperSettings>();
}
