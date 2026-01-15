#include "EasyEIBindingsEditor.h"

#include "EasyEIBindingsComponentDetails.h"

#define LOCTEXT_NAMESPACE "FEasyEIBindingsEditorModule"

void FEasyEIBindingsEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.RegisterCustomClassLayout("EasyEIBindingsComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FEasyEIBindingsComponentDetails::MakeInstance));
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FEasyEIBindingsEditorModule::ShutdownModule()
{
    
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FEasyEIBindingsEditorModule, EasyEIBindingsEditor)