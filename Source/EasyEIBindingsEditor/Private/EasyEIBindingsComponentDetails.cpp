// Copyright Stylianos Maimaris. All Rights Reserved.

#include "EasyEIBindingsComponentDetails.h"

#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EasyEIBindingsComponent.h"
#include "EasyEIBindingsDeveloperSettings.h"
#include "Editor.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "IContentBrowserSingleton.h"
#include "InputAction.h"
#include "K2Node_EnhancedInputAction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "ScopedTransaction.h"
#include "SourceCodeNavigation.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "EasyEIBindingsComponentDetails"

static bool PromptForInputActionSavePath(FString& OutPackagePath, FString& OutAssetName)
{
	const UEasyEIBindingsDeveloperSettings* Settings = UEasyEIBindingsDeveloperSettings::Get();

	FSaveAssetDialogConfig SaveConfig;
	SaveConfig.DialogTitleOverride = LOCTEXT("CreateInputActionTitle", "Create New Input Action");
	SaveConfig.DefaultPath = Settings ? Settings->DefaultInputActionPath.Path : TEXT("/Game/Input");
	SaveConfig.DefaultAssetName = Settings ? Settings->InputActionPrefix : TEXT("IA_");
	SaveConfig.AssetClassNames.Add(UInputAction::StaticClass()->GetClassPathName());
	SaveConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;

	FContentBrowserModule& CB = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	const FString ObjectPath = CB.Get().CreateModalSaveAssetDialog(SaveConfig);

	if (ObjectPath.IsEmpty())
	{
		return false;
	}

	const FString PackageName = FPackageName::ObjectPathToPackageName(ObjectPath);
	OutPackagePath = FPackageName::GetLongPackagePath(PackageName);
	OutAssetName = FPackageName::GetShortName(PackageName);
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

	OwnerComponent = EditedObjects.Num() == 1 ? Cast<UEasyEIBindingsComponent>(EditedObjects[0].Get()) : nullptr;
	CachedOwnerClass = ResolveOwnerClass();

	const TSharedRef<IPropertyHandle> InputBindingsArray = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UEasyEIBindingsComponent, InputBindings));

	IDetailCategoryBuilder& Cat = DetailBuilder.EditCategory("Easy EI Bindings");
	Cat.AddProperty(InputBindingsArray);

	const UEasyEIBindingsDeveloperSettings* Settings = UEasyEIBindingsDeveloperSettings::Get();
	if (Settings && Settings->bShowBindingStatus && OwnerComponent.IsValid() && CachedOwnerClass)
	{
		AddBindingStatusWidget(Cat);
	}

	Cat.AddCustomRow(LOCTEXT("EasyEIButtonsFilter", "EasyEIButtons"))
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
				.Text(LOCTEXT("NewInputAction", "+ New Input Action..."))
				.OnClicked(FOnClicked::CreateSP(this, &FEasyEIBindingsComponentDetails::OnCreateInputAction))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SNew(SButton)
				.Text(LOCTEXT("GenerateStubs", "Generate Function Stubs"))
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
				.Text(LOCTEXT("GenerateBPStubs", "Generate BP Event Stubs"))
				.OnClicked(FOnClicked::CreateSP(this, &FEasyEIBindingsComponentDetails::GenerateBlueprintStubs))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SNew(SButton)
				.Text(LOCTEXT("AddFromFolder", "Add IAs from Folder..."))
				.OnClicked(FOnClicked::CreateSP(this, &FEasyEIBindingsComponentDetails::OnAddFromFolder))
			]
		]
	];
}

UClass* FEasyEIBindingsComponentDetails::ResolveOwnerClass() const
{
	if (!OwnerComponent.IsValid())
	{
		return nullptr;
	}

	if (const UBlueprintGeneratedClass* BlueprintGeneratedClass = OwnerComponent->GetTypedOuter<UBlueprintGeneratedClass>())
	{
		if (const UObject* CDOArchetype = BlueprintGeneratedClass->GetArchetypeForCDO())
		{
			return CDOArchetype->GetClass();
		}
	}

	if (const AActor* OwnerActor = OwnerComponent->GetTypedOuter<AActor>())
	{
		return OwnerActor->GetClass()->GetArchetypeForCDO()->GetClass();
	}

	return nullptr;
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

	const FString StatusText = FString::Printf(TEXT("Binding Status: %d bound, %d missing"), BoundCount, MissingCount);
	const FSlateColor StatusColor = MissingCount > 0 ? FSlateColor(FLinearColor::Yellow) : FSlateColor(FLinearColor::Green);

	Category.AddCustomRow(LOCTEXT("BindingStatusFilter", "BindingStatus"))
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

	for (const FEasyEIBinding& Binding : OwnerComponent->InputBindings)
	{
		if (!Binding.InputAction)
		{
			continue;
		}

		for (const ETriggerEvent Event : EasyEIBindings::BindableEvents)
		{
			FBindingStatus Status;
			Status.FunctionName = EasyEIBindings::MakeHandlerName(Binding.InputAction, Event);
			Status.Event = Event;
			Status.bIsEnabled = Binding.IsEventEnabled(Event);
			Status.bExists = CachedOwnerClass->FindFunctionByName(FName(*Status.FunctionName)) != nullptr;
			OutStatuses.Add(Status);
		}
	}
}

bool FEasyEIBindingsComponentDetails::DoesFunctionExist(UClass* OwnerClass, const FString& FunctionName)
{
	return OwnerClass && OwnerClass->FindFunctionByName(FName(*FunctionName)) != nullptr;
}

FReply FEasyEIBindingsComponentDetails::OnCreateInputAction()
{
	FString PackagePath, AssetName;
	if (!PromptForInputActionSavePath(PackagePath, AssetName))
	{
		return FReply::Handled();
	}

	UObject* NewAsset = FAssetToolsModule::GetModule().Get().CreateAsset(
		AssetName, PackagePath, UInputAction::StaticClass(), nullptr);

	if (NewAsset)
	{
		if (OwnerComponent.IsValid())
		{
			const UEasyEIBindingsDeveloperSettings* Settings = UEasyEIBindingsDeveloperSettings::Get();

			FScopedTransaction Tx(LOCTEXT("AddInputActionBinding", "Add Input Action Binding"));
			OwnerComponent->Modify();

			FEasyEIBinding NewBinding;
			NewBinding.InputAction = Cast<UInputAction>(NewAsset);
			if (Settings)
			{
				NewBinding.EnabledEvents = Settings->DefaultEnabledEvents;
			}
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

	const UEasyEIBindingsDeveloperSettings* Settings = UEasyEIBindingsDeveloperSettings::Get();

	FOpenAssetDialogConfig Config;
	Config.DialogTitleOverride = LOCTEXT("SelectInputActionsTitle", "Select Input Actions");
	Config.DefaultPath = Settings ? Settings->DefaultInputActionPath.Path : TEXT("/Game/Input");
	Config.bAllowMultipleSelection = true;
	Config.AssetClassNames.Add(UInputAction::StaticClass()->GetClassPathName());

	TArray<FAssetData> SelectedAssets = ContentBrowserModule.Get().CreateModalOpenAssetDialog(Config);

	if (SelectedAssets.Num() > 0 && OwnerComponent.IsValid())
	{
		FScopedTransaction Tx(LOCTEXT("AddInputActionsFromFolder", "Add Input Actions from Folder"));
		OwnerComponent->Modify();

		for (const FAssetData& AssetData : SelectedAssets)
		{
			UInputAction* InputAction = Cast<UInputAction>(AssetData.GetAsset());
			if (!InputAction)
			{
				continue;
			}

			const bool bAlreadyExists = OwnerComponent->InputBindings.ContainsByPredicate(
				[InputAction](const FEasyEIBinding& Existing) { return Existing.InputAction == InputAction; });

			if (!bAlreadyExists)
			{
				FEasyEIBinding NewBinding;
				NewBinding.InputAction = InputAction;
				if (Settings)
				{
					NewBinding.EnabledEvents = Settings->DefaultEnabledEvents;
				}
				OwnerComponent->InputBindings.Add(NewBinding);
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

	UClass* OwnerClass = ResolveOwnerClass();
	if (!OwnerClass)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			LOCTEXT("NoOwnerClass", "Failed to determine owner class for EasyEI Bindings Component."));
		return FReply::Handled();
	}

	if (!OwnerClass->HasAnyClassFlags(CLASS_Native))
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			LOCTEXT("NotNativeClass",
			        "Owner class is a Blueprint-generated class. Generating C++ stubs only works on native classes.\n\n"
			        "Use 'Generate BP Event Stubs' for Blueprint classes."));
		return FReply::Handled();
	}

	GenerateCPPStubs(OwnerClass, false);

	return FReply::Handled();
}

FReply FEasyEIBindingsComponentDetails::GenerateBlueprintStubs()
{
	if (!OwnerComponent.IsValid())
	{
		return FReply::Handled();
	}

	// Get the Blueprint from the component's outer, falling back to the owning actor's class
	UBlueprintGeneratedClass* BlueprintGeneratedClass = OwnerComponent->GetTypedOuter<UBlueprintGeneratedClass>();
	if (!BlueprintGeneratedClass)
	{
		if (const AActor* OwnerActor = OwnerComponent->GetTypedOuter<AActor>())
		{
			BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(OwnerActor->GetClass());
		}
	}

	if (!BlueprintGeneratedClass)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			LOCTEXT("NoBlueprintClass", "Could not find a Blueprint class. This feature only works on Blueprint actors."));
		return FReply::Handled();
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(BlueprintGeneratedClass->ClassGeneratedBy);
	if (!Blueprint)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			LOCTEXT("NoBlueprintAsset", "Could not find the Blueprint asset."));
		return FReply::Handled();
	}

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
			LOCTEXT("NoEventGraph", "Could not find the Event Graph in the Blueprint."));
		return;
	}

	int32 GeneratedCount = 0;
	int32 SkippedCount = 0;

	// Place new nodes below everything that already exists in the graph
	float NodePosY = 0.f;
	for (const UEdGraphNode* Node : EventGraph->Nodes)
	{
		if (Node)
		{
			NodePosY = FMath::Max(NodePosY, Node->NodePosY + 200.f);
		}
	}

	for (const FEasyEIBinding& Binding : OwnerComponent->InputBindings)
	{
		// One event node per action covers all trigger events, so only the action itself matters here
		if (!Binding.InputAction || Binding.EnabledEvents == 0)
		{
			continue;
		}

		const bool bNodeExists = EventGraph->Nodes.ContainsByPredicate(
			[&Binding](const UEdGraphNode* Node)
			{
				const UK2Node_EnhancedInputAction* InputActionNode = Cast<UK2Node_EnhancedInputAction>(Node);
				return InputActionNode && InputActionNode->InputAction == Binding.InputAction;
			});

		if (bNodeExists)
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
	}

	if (GeneratedCount > 0)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

		const FString Message = FString::Printf(
			TEXT("Generated %d Enhanced Input Action node(s) in the Blueprint.\nSkipped %d existing node(s).\n\nThe nodes are ready to use in the Event Graph."),
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
			LOCTEXT("NoBindingsConfiguredBP", "No input bindings configured. Add some Input Actions first."));
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
			LOCTEXT("NoSourceFiles", "Could not find source files for the owner class."));
		return;
	}

	FString HeaderText;
	FFileHelper::LoadFileToString(HeaderText, *HeaderPath);
	FString SourceText;
	FFileHelper::LoadFileToString(SourceText, *SourcePath);

	// Stubs are inserted between marker comments so repeated runs keep them in one place
	const FString BeginMarker(TEXT("//Input actions"));
	const FString EndMarker(TEXT("//Input actions END"));
	int32 BeginIdx = HeaderText.Find(BeginMarker);
	int32 EndIdx = HeaderText.Find(EndMarker);

	if (BeginIdx == INDEX_NONE || EndIdx == INDEX_NONE)
	{
		int32 ProtectedIdx = HeaderText.Find(TEXT("protected:"), ESearchCase::IgnoreCase);
		if (ProtectedIdx == INDEX_NONE)
		{
			const int32 BraceIdx = HeaderText.Find(TEXT("{"));
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

	// Collect declarations already inside the marker block, so unsaved-to-compile stubs aren't duplicated
	TSet<FString> ExistingInHeader;
	{
		const FString Segment = HeaderText.Mid(BeginIdx, EndIdx - BeginIdx);
		TArray<FString> Lines;
		Segment.ParseIntoArrayLines(Lines, false);
		for (const FString& Line : Lines)
		{
			const FString Trimmed = Line.TrimStartAndEnd();
			if (Trimmed.StartsWith(TEXT("void ")))
			{
				const int32 ParenIdx = Trimmed.Find(TEXT("("));
				if (ParenIdx != INDEX_NONE)
				{
					ExistingInHeader.Add(Trimmed.Mid(5, ParenIdx - 5));
				}
			}
		}
	}

	bool bDirty = false;
	int32 GeneratedCount = 0;
	int32 SkippedCount = 0;

	const FString ClassName = FString(OwnerClass->GetPrefixCPP()) + OwnerClass->GetName();

	for (const FEasyEIBinding& Binding : OwnerComponent->InputBindings)
	{
		if (!Binding.InputAction)
		{
			continue;
		}

		for (const ETriggerEvent Event : EasyEIBindings::BindableEvents)
		{
			if (!Binding.IsEventEnabled(Event))
			{
				continue;
			}

			const FString FuncName = EasyEIBindings::MakeHandlerName(Binding.InputAction, Event);

			if (DoesFunctionExist(OwnerClass, FuncName) || ExistingInHeader.Contains(FuncName))
			{
				SkippedCount++;
				continue;
			}

			FString Decl;
			if (bBlueprintImplementable)
			{
				Decl = FString::Printf(
					TEXT("\tUFUNCTION(BlueprintImplementableEvent, Category = \"Input\")\n\tvoid %s(const FInputActionValue& Value);\n\n"),
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
				const FString FuncSignature = FString::Printf(TEXT("%s::%s"), *ClassName, *FuncName);
				if (!SourceText.Contains(FuncSignature))
				{
					SourceText += FString::Printf(
						TEXT("\nvoid %s(const FInputActionValue& Value)\n{\n}\n"), *FuncSignature);
				}
			}
		}
	}

	if (bDirty)
	{
		FFileHelper::SaveStringToFile(HeaderText, *HeaderPath);
		FFileHelper::SaveStringToFile(SourceText, *SourcePath);

		const FString Message = FString::Printf(
			TEXT("Generated %d function stub(s). Skipped %d existing function(s).\n\nPlease rebuild the project to use the new functions."),
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
			LOCTEXT("NoBindingsConfiguredCPP", "No input bindings configured. Add some Input Actions first."));
	}
}

#undef LOCTEXT_NAMESPACE
