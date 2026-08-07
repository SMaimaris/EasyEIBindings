// Copyright Stylianos Maimaris. All Rights Reserved.

#include "EasyEIBindingsComponent.h"

#include "EasyEIBindings.h"
#include "EasyEIBindingsDeveloperSettings.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "GameFramework/Actor.h"

namespace EasyEIBindings
{
	const TCHAR* GetEventSuffix(ETriggerEvent Event)
	{
		switch (Event)
		{
		case ETriggerEvent::Triggered: return TEXT("Triggered");
		case ETriggerEvent::Started: return TEXT("Started");
		case ETriggerEvent::Ongoing: return TEXT("Ongoing");
		case ETriggerEvent::Completed: return TEXT("Completed");
		case ETriggerEvent::Canceled: return TEXT("Canceled");
		default: return TEXT("");
		}
	}

	FString MakeHandlerName(const UInputAction* InputAction, ETriggerEvent Event)
	{
		FString ActionName = InputAction ? InputAction->GetName() : FString();
		if (const UEasyEIBindingsDeveloperSettings* Settings = UEasyEIBindingsDeveloperSettings::Get())
		{
			ActionName.RemoveFromStart(Settings->InputActionPrefix);
		}
		return FString::Printf(TEXT("IA_%s_%s"), *ActionName, GetEventSuffix(Event));
	}
}

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

	for (const FEasyEIBinding& Binding : InputBindings)
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

			const FName HandlerName(*EasyEIBindings::MakeHandlerName(Binding.InputAction, Event));
			if (Owner->FindFunction(HandlerName))
			{
				const FEnhancedInputActionEventBinding& ActionBinding = EnhancedInputComponent->BindAction(
					Binding.InputAction, Event, Owner, HandlerName);
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

	for (const uint32 Handle : BoundActionHandles)
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
