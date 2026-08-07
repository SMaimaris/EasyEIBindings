// Copyright Stylianos Maimaris. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"

class UEasyEIBindingsComponent;
class IDetailCategoryBuilder;
class UBlueprint;
struct FBindingStatus;

/**
 * Custom details panel for UEasyEIBindingsComponent.
 * Adds buttons for creating Input Actions, generating handler stubs (C++ and Blueprint),
 * and shows a bound/missing summary for the configured bindings.
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

	/** Resolves the class the component's handler functions live on (native class or Blueprint-generated class). */
	UClass* ResolveOwnerClass() const;

	static bool DoesFunctionExist(UClass* OwnerClass, const FString& FunctionName);

	TWeakObjectPtr<UEasyEIBindingsComponent> OwnerComponent;
	UClass* CachedOwnerClass = nullptr;
};
