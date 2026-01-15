// Copyright Stylianos Maimaris. All Rights Reserved.


#include "EasyEIBindingsComponent.h"

#include "EasyEIBindings.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"


UEasyEIBindingsComponent::UEasyEIBindingsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEasyEIBindingsComponent::SetupInputActions(UEnhancedInputComponent* EnhancedInputComponent)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogEasyEIBindings, Warning, TEXT("%hs: No owner actor found."), __FUNCTION__);
		return;
	}

	if (!EnhancedInputComponent)
	{
		EnhancedInputComponent = Cast<UEnhancedInputComponent>(Owner->InputComponent);
	}

	if (!EnhancedInputComponent)
	{
		UE_LOG(LogEasyEIBindings, Warning, TEXT("%hs: No Enhanced Input Component passed or found on owner."), __FUNCTION__);
		return;
	}

	struct FBindSpec
	{
		const TCHAR* Suffix;
		ETriggerEvent Event;
	} Specs[] = {
		{TEXT("Triggered"), ETriggerEvent::Triggered},
		{TEXT("Started"), ETriggerEvent::Started},
		{TEXT("Ongoing"), ETriggerEvent::Ongoing},
		{TEXT("Completed"), ETriggerEvent::Completed},
		{TEXT("Canceled"), ETriggerEvent::Canceled}
	};

	BoundActionHandles.Empty();

	for (const FEasyEIBinding& Binding : InputBindings)
	{
		if (!Binding.InputAction)
		{
			continue;
		}

		FString ActionName = Binding.InputAction->GetName();
		ActionName.RemoveFromStart(TEXT("IA_"));

		for (const FBindSpec& Spec : Specs)
		{
			if (!Binding.IsEventEnabled(Spec.Event))
			{
				continue;
			}

			FString FuncStr = FString::Printf(TEXT("IA_%s_%s"), *ActionName, Spec.Suffix);
			FName FuncName(*FuncStr);

			if (Owner->FindFunction(FuncName))
			{
				FEnhancedInputActionEventBinding& ActionBinding = EnhancedInputComponent->BindAction(
					Binding.InputAction, Spec.Event, Owner, FuncName);
				BoundActionHandles.Add(ActionBinding.GetHandle());
			}
		}
	}
}

void UEasyEIBindingsComponent::RebindInputActions()
{
	ClearInputBindings();
	SetupInputActions();
}

void UEasyEIBindingsComponent::ClearInputBindings()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(Owner->InputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	for (uint32 Handle : BoundActionHandles)
	{
		EnhancedInputComponent->RemoveBindingByHandle(Handle);
	}
	BoundActionHandles.Empty();
}

void UEasyEIBindingsComponent::BeginPlay()
{
	Super::BeginPlay();
	SetupInputActions();
}
