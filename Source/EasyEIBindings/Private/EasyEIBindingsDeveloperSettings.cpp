// Copyright Stylianos Maimaris. All Rights Reserved.

#include "EasyEIBindingsDeveloperSettings.h"

#include "InputTriggers.h"

UEasyEIBindingsDeveloperSettings::UEasyEIBindingsDeveloperSettings()
{
	DefaultInputActionPath.Path = TEXT("/Game/Input");
	InputActionPrefix = TEXT("IA_");

	DefaultEnabledEvents = static_cast<int32>(ETriggerEvent::Triggered) | static_cast<int32>(ETriggerEvent::Completed);

	bShowBindingStatus = true;
}

const UEasyEIBindingsDeveloperSettings* UEasyEIBindingsDeveloperSettings::Get()
{
	return GetDefault<UEasyEIBindingsDeveloperSettings>();
}
