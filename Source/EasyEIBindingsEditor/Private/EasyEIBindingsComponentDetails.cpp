// Copyright Stylianos Maimaris. All Rights Reserved.


#include "EasyEIBindingsComponentDetails.h"

#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EasyEIBindingsComponent.h"
#include "EasyEIBindingsDeveloperSettings.h"
#include "IContentBrowserSingleton.h"
#include "InputAction.h"
#include "SourceCodeNavigation.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraphSchema_K2.h"

// Enhanced Input Blueprint nodes
#include "K2Node_EnhancedInputAction.h"

#define LOCTEXT_NAMESPACE "EasyEIBindingsComponentDetails"

static bool PromptForInputActionSavePath(FString& OutPackagePath, FString& OutAssetName)
{
	const UEasyEIBindingsDeveloperSettings* Settings = UEasyEIBindingsDeveloperSettings::Get();

	FSaveAssetDialogConfig SaveConfig;
	SaveConfig.DialogTitleOverride = FText::FromString("Create New Input Action");
	SaveConfig.DefaultPath = Settings ? Settings->DefaultInputActionPath.Path : TEXT("/Game/Input");
	SaveConfig.DefaultAssetName = Settings ? Settings->InputActionPrefix : TEXT("IA_");
	SaveConfig.AssetClassNames.Add(UInputAction::StaticClass()->GetClassPathName());
	SaveConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;

	FContentBrowserModule& CB = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	FString ObjectPath = CB.Get().CreateModalSaveAssetDialog(SaveConfig);

	if (ObjectPath.IsEmpty())
	{
		return false;
	}

	const FString PackagePathName = FPackageName::ObjectPathToPackageName(ObjectPath);
	OutPackagePath = FPackageName::GetLongPackagePath(PackagePathName);
	OutAssetName = FPackageName::GetShortName(PackagePathName);
	return true;
}

struct FBindingStatus
{
	FString FunctionName;
	ETriggerEvent Event;
	bool bExists;
	bool bIsEnabled;
};

void FEasyEIBindingsComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> EditedObjects;
	DetailBuilder.GetObjectsBeingCustomized(EditedObjects);

	if (EditedObjects.Num() == 1)
	{
		OwnerComponent = Cast<UEasyEIBindingsComponent>(EditedObjects[0].Get());
	}
	else
	{
		OwnerComponent = nullptr;
	}

	CachedOwnerClass = nullptr;
	if (OwnerComponent.IsValid())
	{
		UBlueprintGeneratedClass* BlueprintGeneratedClass = OwnerComponent->GetTypedOuter<UBlueprintGeneratedClass>();
		AActor* Outer = OwnerComponent->GetTypedOuter<AActor>();
		UObject* CDOArchetype = BlueprintGeneratedClass ? BlueprintGeneratedClass->GetArchetypeForCDO() : nullptr;
		CachedOwnerClass = CDOArchetype
			                   ? CDOArchetype->GetClass()
			                   : Outer
			                   ? Outer->GetClass()->GetArchetypeForCDO()->GetClass()
			                   : nullptr;
	}

	const TSharedRef<IPropertyHandle> InputBindingsArray = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UEasyEIBindingsComponent, InputBindings));

	IDetailCategoryBuilder& Cat = DetailBuilder.EditCategory("Easy EI Bindings");
	Cat.AddProperty(InputBindingsArray);

	const UEasyEIBindingsDeveloperSettings* Settings = UEasyEIBindingsDeveloperSettings::Get();
	if (Settings && Settings->bShowBindingStatus && OwnerComponent.IsValid() && CachedOwnerClass)
	{
		AddBindingStatusWidget(Cat);
	}

	Cat.AddCustomRow(FText::FromString("EasyEIButtons"))
	   .WholeRowWidget
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 4)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding(0, 0, 4, 0)
			[
				SNew(SButton)
				.Text(FText::FromString("+ New Input Action..."))
				.OnClicked(FOnClicked::CreateSP(this, &FEasyEIBindingsComponentDetails::OnCreateInputAction))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SNew(SButton)
				.Text(FText::FromString("Generate Function Stubs"))
				.OnClicked(FOnClicked::CreateSP(this, &FEasyEIBindingsComponentDetails::GenerateStubs))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding(0, 0, 4, 0)
			[
				SNew(SButton)
				.Text(FText::FromString("Generate BP Event Stubs"))
				.OnClicked(FOnClicked::CreateSP(this, &FEasyEIBindingsComponentDetails::GenerateBlueprintStubs))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SNew(SButton)
				.Text(FText::FromString("Add IAs from Folder..."))
				.OnClicked(FOnClicked::CreateSP(this, &FEasyEIBindingsComponentDetails::OnAddFromFolder))
			]
		]
	];
}

void FEasyEIBindingsComponentDetails::AddBindingStatusWidget(IDetailCategoryBuilder& Category)
{
	if (!OwnerComponent.IsValid() || !CachedOwnerClass)
	{
		return;
	}

	TArray<FBindingStatus> AllStatuses;
	GatherBindingStatuses(AllStatuses);

	if (AllStatuses.Num() == 0)
	{
		return;
	}

	int32 BoundCount = 0;
	int32 MissingCount = 0;
	for (const FBindingStatus& Status : AllStatuses)
	{
		if (Status.bIsEnabled)
		{
			if (Status.bExists)
			{
				BoundCount++;
			}
			else
			{
				MissingCount++;
			}
		}
	}

	FString StatusText = FString::Printf(TEXT("Binding Status: %d bound, %d missing"), BoundCount, MissingCount);
	FSlateColor StatusColor = MissingCount > 0 ? FSlateColor(FLinearColor::Yellow) : FSlateColor(FLinearColor::Green);

	Category.AddCustomRow(FText::FromString("BindingStatus"))
	        .WholeRowWidget
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4, 2)
		[
			SNew(STextBlock)
			.Text(FText::FromString(StatusText))
			.ColorAndOpacity(StatusColor)
		]
	];
}

void FEasyEIBindingsComponentDetails::GatherBindingStatuses(TArray<FBindingStatus>& OutStatuses)
{
	if (!OwnerComponent.IsValid() || !CachedOwnerClass)
	{
		return;
	}

	const TPair<const TCHAR*, ETriggerEvent> Specs[] = {
		{TEXT("Triggered"), ETriggerEvent::Triggered},
		{TEXT("Started"), ETriggerEvent::Started},
		{TEXT("Ongoing"), ETriggerEvent::Ongoing},
		{TEXT("Completed"), ETriggerEvent::Completed},
		{TEXT("Canceled"), ETriggerEvent::Canceled}
	};

	for (const FEasyEIBinding& Binding : OwnerComponent->InputBindings)
	{
		if (!Binding.InputAction)
		{
			continue;
		}

		FString ActionName = Binding.InputAction->GetName();
		ActionName.RemoveFromStart(TEXT("IA_"));

		for (const auto& Spec : Specs)
		{
			FBindingStatus Status;
			Status.FunctionName = FString::Printf(TEXT("IA_%s_%s"), *ActionName, Spec.Key);
			Status.Event = Spec.Value;
			Status.bIsEnabled = Binding.IsEventEnabled(Spec.Value);
			Status.bExists = CachedOwnerClass->FindFunctionByName(FName(*Status.FunctionName)) != nullptr;
			OutStatuses.Add(Status);
		}
	}
}

bool FEasyEIBindingsComponentDetails::DoesFunctionExist(UClass* OwnerClass, const FString& FunctionName)
{
	if (!OwnerClass)
	{
		return false;
	}

	if (OwnerClass->FindFunctionByName(FName(*FunctionName)))
	{
		return true;
	}

	return false;
}

FReply FEasyEIBindingsComponentDetails::OnCreateInputAction()
{
	FString PackageName, AssetName;
	if (!PromptForInputActionSavePath(PackageName, AssetName))
	{
		return FReply::Handled();
	}

	UObject* NewAsset = FAssetToolsModule::GetModule().Get().CreateAsset(
		AssetName, FPackageName::GetLongPackagePath(PackageName),
		UInputAction::StaticClass(), nullptr);

	if (NewAsset)
	{
		if (OwnerComponent.IsValid())
		{
			const UEasyEIBindingsDeveloperSettings* Settings = UEasyEIBindingsDeveloperSettings::Get();

			OwnerComponent->Modify();
			FEasyEIBinding NewBinding;
			NewBinding.InputAction = Cast<UInputAction>(NewAsset);
			NewBinding.EnabledEvents = Settings ? Settings->DefaultEnabledEvents : 0x1F;
			OwnerComponent->InputBindings.Add(NewBinding);
		}

		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(NewAsset);
	}
	return FReply::Handled();
}

FReply FEasyEIBindingsComponentDetails::OnAddFromFolder()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(
		"ContentBrowser");

	FOpenAssetDialogConfig Config;
	Config.DialogTitleOverride = FText::FromString("Select Input Actions");
	Config.DefaultPath = TEXT("/Game/Input");
	Config.bAllowMultipleSelection = true;
	Config.AssetClassNames.Add(UInputAction::StaticClass()->GetClassPathName());

	TArray<FAssetData> SelectedAssets = ContentBrowserModule.Get().CreateModalOpenAssetDialog(Config);

	if (SelectedAssets.Num() > 0 && OwnerComponent.IsValid())
	{
		const UEasyEIBindingsDeveloperSettings* Settings = UEasyEIBindingsDeveloperSettings::Get();
		FScopedTransaction Tx(LOCTEXT("AddInputActionsFromFolder", "Add Input Actions from Folder"));
		OwnerComponent->Modify();

		for (const FAssetData& AssetData : SelectedAssets)
		{
			UInputAction* IA = Cast<UInputAction>(AssetData.GetAsset());
			if (IA)
			{
				bool bAlreadyExists = false;
				for (const FEasyEIBinding& Existing : OwnerComponent->InputBindings)
				{
					if (Existing.InputAction == IA)
					{
						bAlreadyExists = true;
						break;
					}
				}

				if (!bAlreadyExists)
				{
					FEasyEIBinding NewBinding;
					NewBinding.InputAction = IA;
					NewBinding.EnabledEvents = Settings ? Settings->DefaultEnabledEvents : 0x1F;
					OwnerComponent->InputBindings.Add(NewBinding);
				}
			}
		}
	}

	return FReply::Handled();
}

FReply FEasyEIBindingsComponentDetails::GenerateStubs()
{
	if (!OwnerComponent.IsValid())
	{
		return FReply::Handled();
	}

	UBlueprintGeneratedClass* BlueprintGeneratedClass = OwnerComponent->GetTypedOuter<UBlueprintGeneratedClass>();
	AActor* Outer = OwnerComponent->GetTypedOuter<AActor>();

	UObject* CDOArchetype = BlueprintGeneratedClass ? BlueprintGeneratedClass->GetArchetypeForCDO() : nullptr;
	UClass* OwnerClass = CDOArchetype
		                     ? CDOArchetype->GetClass()
		                     : Outer
		                     ? Outer->GetClass()->GetArchetypeForCDO()->GetClass()
		                     : nullptr;

	if (!OwnerClass)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(TEXT("Failed to determine owner class for EasyEI Bindings Component.")));
		return FReply::Handled();
	}

	if (!OwnerClass->HasAnyClassFlags(CLASS_Native))
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(TEXT(
				"Owner class is a Blueprint-generated class. GenerateStubs (C++) only works on native classes.\n\n"
				"Use 'Generate BP Event Stubs' for Blueprint classes.")));
		return FReply::Handled();
	}

	FScopedTransaction Tx(LOCTEXT("GenerateInputActionStubs", "Generate Input Action Stubs"));
	GenerateCPPStubs(OwnerClass, false);

	return FReply::Handled();
}

FReply FEasyEIBindingsComponentDetails::GenerateBlueprintStubs()
{
	if (!OwnerComponent.IsValid())
	{
		return FReply::Handled();
	}

	// Get the Blueprint from the component's outer
	UBlueprintGeneratedClass* BlueprintGeneratedClass = OwnerComponent->GetTypedOuter<UBlueprintGeneratedClass>();
	if (!BlueprintGeneratedClass)
	{
		// Try getting from the actor
		AActor* Outer = OwnerComponent->GetTypedOuter<AActor>();
		if (Outer)
		{
			BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(Outer->GetClass());
		}
	}

	if (!BlueprintGeneratedClass)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(TEXT("Could not find a Blueprint class. This feature only works on Blueprint actors.")));
		return FReply::Handled();
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(BlueprintGeneratedClass->ClassGeneratedBy);
	if (!Blueprint)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(TEXT("Could not find the Blueprint asset.")));
		return FReply::Handled();
	}

	// Generate custom events in the Blueprint
	FScopedTransaction Tx(LOCTEXT("GenerateBlueprintInputEvents", "Generate Blueprint Input Events"));
	Blueprint->Modify();

	GenerateBlueprintEvents(Blueprint);

	return FReply::Handled();
}

void FEasyEIBindingsComponentDetails::GenerateBlueprintEvents(UBlueprint* Blueprint)
{
	if (!Blueprint || !OwnerComponent.IsValid())
	{
		return;
	}

	UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
	if (!EventGraph)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(TEXT("Could not find the Event Graph in the Blueprint.")));
		return;
	}

	const TPair<const TCHAR*, ETriggerEvent> Specs[] = {
		{TEXT("Triggered"), ETriggerEvent::Triggered},
		{TEXT("Started"), ETriggerEvent::Started},
		{TEXT("Ongoing"), ETriggerEvent::Ongoing},
		{TEXT("Completed"), ETriggerEvent::Completed},
		{TEXT("Canceled"), ETriggerEvent::Canceled}
	};

	int32 GeneratedCount = 0;
	int32 SkippedCount = 0;
	float NodePosY = 0.f;

	for (UEdGraphNode* Node : EventGraph->Nodes)
	{
		if (Node)
		{
			NodePosY = FMath::Max(NodePosY, Node->NodePosY + 200.f);
		}
	}

	for (const FEasyEIBinding& Binding : OwnerComponent->InputBindings)
	{
		if (!Binding.InputAction)
		{
			continue;
		}

		for (const auto& Spec : Specs)
		{
			if (!Binding.IsEventEnabled(Spec.Value))
			{
				continue;
			}

			bool bEventExists = false;
			for (UEdGraphNode* Node : EventGraph->Nodes)
			{
				if (UK2Node_EnhancedInputAction* InputActionNode = Cast<UK2Node_EnhancedInputAction>(Node))
				{
					if (InputActionNode->InputAction == Binding.InputAction)
					{
						bEventExists = true;
						break;
					}
				}
			}

			if (bEventExists)
			{
				SkippedCount++;
				continue;
			}

			UK2Node_EnhancedInputAction* NewInputActionNode = NewObject<UK2Node_EnhancedInputAction>(EventGraph);
			NewInputActionNode->InputAction = Binding.InputAction;
			NewInputActionNode->CreateNewGuid();
			NewInputActionNode->PostPlacedNewNode();
			NewInputActionNode->SetFlags(RF_Transactional);
			NewInputActionNode->AllocateDefaultPins();

			NewInputActionNode->NodePosX = 0;
			NewInputActionNode->NodePosY = NodePosY;
			NodePosY += 300.f;

			EventGraph->AddNode(NewInputActionNode, false, false);

			GeneratedCount++;

			break;
		}
	}

	if (GeneratedCount > 0)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

		FString Message = FString::Printf(
			TEXT(
				"Generated %d Enhanced Input Action node(s) in the Blueprint.\nSkipped %d existing node(s).\n\nThe nodes are ready to use in the Event Graph."),
			GeneratedCount, SkippedCount);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
	}
	else if (SkippedCount > 0)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(
				FString::Printf(
					TEXT("All %d Input Action node(s) already exist in the Blueprint. Nothing to generate."),
					SkippedCount)));
	}
	else
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(TEXT("No input bindings configured. Add some Input Actions first.")));
	}
}

void FEasyEIBindingsComponentDetails::GenerateCPPStubs(UClass* OwnerClass, bool bBlueprintImplementable)
{
	FString HeaderPath, SourcePath;
	if (!FSourceCodeNavigation::FindClassHeaderPath(OwnerClass, HeaderPath) ||
		!FSourceCodeNavigation::FindClassSourcePath(OwnerClass, SourcePath))
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(TEXT("Could not find source files for the owner class.")));
		return;
	}

	FString HeaderText;
	FFileHelper::LoadFileToString(HeaderText, *HeaderPath);
	FString SourceText;
	FFileHelper::LoadFileToString(SourceText, *SourcePath);

	const FString BeginMarker(TEXT("//Input actions"));
	const FString EndMarker(TEXT("//Input actions END"));
	int32 BeginIdx = HeaderText.Find(BeginMarker);
	int32 EndIdx = HeaderText.Find(EndMarker);

	if (BeginIdx == INDEX_NONE || EndIdx == INDEX_NONE)
	{
		int32 ProtectedIdx = HeaderText.Find(TEXT("protected:"), ESearchCase::IgnoreCase);
		if (ProtectedIdx == INDEX_NONE)
		{
			int32 BraceIdx = HeaderText.Find(TEXT("{"));
			if (BraceIdx == INDEX_NONE)
			{
				return;
			}
			ProtectedIdx = BraceIdx + 1;
			HeaderText.InsertAt(ProtectedIdx, TEXT("\nprotected:\n"));
			ProtectedIdx += FCString::Strlen(TEXT("\nprotected:\n"));
		}
		else
		{
			ProtectedIdx += FCString::Strlen(TEXT("protected:"));
		}

		const FString Block = FString::Printf(TEXT("\n\t%s\n\n\t%s\n"), *BeginMarker, *EndMarker);
		HeaderText.InsertAt(ProtectedIdx, Block);
		BeginIdx = HeaderText.Find(BeginMarker);
		EndIdx = HeaderText.Find(EndMarker);
	}

	TSet<FString> ExistingInHeader;
	{
		FString Segment = HeaderText.Mid(BeginIdx, EndIdx - BeginIdx);
		TArray<FString> Lines;
		Segment.ParseIntoArrayLines(Lines, false);
		for (const FString& L : Lines)
		{
			FString Trim = L.TrimStartAndEnd();
			if (Trim.StartsWith(TEXT("UFUNCTION")))
			{
			}
			if (Trim.StartsWith(TEXT("void IA_")))
			{
				int32 ParenIdx = Trim.Find(TEXT("("));
				if (ParenIdx != INDEX_NONE)
				{
					FString FuncName = Trim.Mid(5, ParenIdx - 5);
					ExistingInHeader.Add(FuncName);
				}
			}
		}
	}

	const TPair<const TCHAR*, ETriggerEvent> Specs[] = {
		{TEXT("Triggered"), ETriggerEvent::Triggered},
		{TEXT("Started"), ETriggerEvent::Started},
		{TEXT("Ongoing"), ETriggerEvent::Ongoing},
		{TEXT("Completed"), ETriggerEvent::Completed},
		{TEXT("Canceled"), ETriggerEvent::Canceled}
	};

	bool bDirty = false;
	int32 GeneratedCount = 0;
	int32 SkippedCount = 0;

	for (const FEasyEIBinding& Binding : OwnerComponent->InputBindings)
	{
		if (!Binding.InputAction)
		{
			continue;
		}

		FString Clean = Binding.InputAction->GetName();
		Clean.RemoveFromStart(TEXT("IA_"));

		for (const auto& Spec : Specs)
		{
			if (!Binding.IsEventEnabled(Spec.Value))
			{
				continue;
			}

			FString FuncName = FString::Printf(TEXT("IA_%s_%s"), *Clean, Spec.Key);

			if (DoesFunctionExist(OwnerClass, FuncName))
			{
				SkippedCount++;
				continue;
			}

			if (ExistingInHeader.Contains(FuncName))
			{
				SkippedCount++;
				continue;
			}

			FString Decl;
			if (bBlueprintImplementable)
			{
				Decl = FString::Printf(
					TEXT(
						"\tUFUNCTION(BlueprintImplementableEvent, Category = \"Input\")\n\tvoid %s(const FInputActionValue& Value);\n\n"),
					*FuncName);
			}
			else
			{
				Decl = FString::Printf(
					TEXT("\tUFUNCTION()\n\tvoid %s(const FInputActionValue& Value);\n\n"), *FuncName);
			}

			HeaderText.InsertAt(EndIdx, Decl);
			EndIdx += Decl.Len();
			bDirty = true;
			GeneratedCount++;

			if (!bBlueprintImplementable)
			{
				FString ClassName = OwnerClass->GetName();
				if (!ClassName.StartsWith(TEXT("A")) && !ClassName.StartsWith(TEXT("U")))
				{
					ClassName = TEXT("A") + ClassName;
				}

				FString Def = FString::Printf(
					TEXT("\nvoid %s::%s(const FInputActionValue& Value)\n{\n}\n"), *ClassName, *FuncName);

				FString FuncSignature = FString::Printf(TEXT("%s::%s"), *ClassName, *FuncName);
				if (!SourceText.Contains(FuncSignature))
				{
					SourceText += Def;
				}
			}
		}
	}

	if (bDirty)
	{
		FFileHelper::SaveStringToFile(HeaderText, *HeaderPath);
		FFileHelper::SaveStringToFile(SourceText, *SourcePath);

		FString Message = FString::Printf(
			TEXT(
				"Generated %d function stub(s). Skipped %d existing function(s).\n\nPlease rebuild the project to use the new functions."),
			GeneratedCount, SkippedCount);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
	}
	else if (SkippedCount > 0)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(
				FString::Printf(TEXT("All %d function(s) already exist. Nothing to generate."), SkippedCount)));
	}
	else
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(TEXT("No input bindings configured. Add some Input Actions first.")));
	}
}

#undef LOCTEXT_NAMESPACE
