// Copyright Stylianos Maimaris. All Rights Reserved.

#include "EasyEIBindingsEditor.h"

#include "EasyEIBindingsComponent.h"
#include "EasyEIBindingsComponentDetails.h"
#include "PropertyEditorModule.h"

void FEasyEIBindingsEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.RegisterCustomClassLayout(
		UEasyEIBindingsComponent::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FEasyEIBindingsComponentDetails::MakeInstance));
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FEasyEIBindingsEditorModule::ShutdownModule()
{
	if (FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
	{
		PropertyEditorModule->UnregisterCustomClassLayout(UEasyEIBindingsComponent::StaticClass()->GetFName());
	}
}

IMPLEMENT_MODULE(FEasyEIBindingsEditorModule, EasyEIBindingsEditor)
