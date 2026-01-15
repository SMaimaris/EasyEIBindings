// Fill out your copyright notice in the Description page of Project Settings.


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
