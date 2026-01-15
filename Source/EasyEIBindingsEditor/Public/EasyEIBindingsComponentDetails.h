// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class UEasyEIBindingsComponent;
class IDetailCategoryBuilder;
class UBlueprint;
struct FBindingStatus;

/**
 * Custom details panel for EasyEIBindingsComponent.
 * Provides buttons for creating input actions, generating function stubs, and managing bindings.
 */
class FEasyEIBindingsComponentDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FEasyEIBindingsComponentDetails);
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	FReply OnCreateInputAction();
	FReply OnAddFromFolder();
	FReply GenerateStubs();
	FReply GenerateBlueprintStubs();

	void GenerateCPPStubs(UClass* OwnerClass, bool bBlueprintImplementable);

	void GenerateBlueprintEvents(UBlueprint* Blueprint);

	void AddBindingStatusWidget(IDetailCategoryBuilder& Category);

	void GatherBindingStatuses(TArray<FBindingStatus>& OutStatuses);

	static bool DoesFunctionExist(UClass* OwnerClass, const FString& FunctionName);

	TWeakObjectPtr<UEasyEIBindingsComponent> OwnerComponent;
	UClass* CachedOwnerClass = nullptr;
};
