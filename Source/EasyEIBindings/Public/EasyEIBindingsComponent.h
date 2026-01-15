// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "InputTriggers.h"
#include "Components/ActorComponent.h"
#include "EasyEIBindingsComponent.generated.h"


class UInputAction;
class UInputMappingContext;

/**
 * Specifies which trigger events to bind for an Input Action.
 */
USTRUCT(BlueprintType)
struct FEasyEIBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
	TObjectPtr<UInputAction> InputAction = nullptr;

	// Bitmask of which trigger events to bind. Default: Triggered, Started, Completed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding",
		meta = (Bitmask, BitmaskEnum = "/Script/EnhancedInput.ETriggerEvent"))
	int32 EnabledEvents = 17;

	bool IsEventEnabled(ETriggerEvent Event) const
	{
		return (EnabledEvents & (1 << static_cast<int32>(Event))) != 0;
	}

	void SetEventEnabled(ETriggerEvent Event, bool bEnabled)
	{
		if (bEnabled)
		{
			EnabledEvents |= (1 << static_cast<int32>(Event));
		}
		else
		{
			EnabledEvents &= ~(1 << static_cast<int32>(Event));
		}
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class EASYEIBINDINGS_API UEasyEIBindingsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEasyEIBindingsComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Easy EI Bindings")
	TArray<FEasyEIBinding> InputBindings;

	UFUNCTION(BlueprintCallable, Category = "Easy EI Bindings")
	virtual void SetupInputActions(UEnhancedInputComponent* EnhancedInputComponent = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Easy EI Bindings")
	virtual void RebindInputActions();

	UFUNCTION(BlueprintCallable, Category = "Easy EI Bindings")
	virtual void ClearInputBindings();

protected:
	virtual void BeginPlay() override;

private:
	TArray<uint32> BoundActionHandles;
};
