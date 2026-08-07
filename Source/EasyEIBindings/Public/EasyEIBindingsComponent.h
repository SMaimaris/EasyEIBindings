// Copyright Stylianos Maimaris. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputTriggers.h"
#include "Components/ActorComponent.h"
#include "EasyEIBindingsComponent.generated.h"

class UEnhancedInputComponent;
class UInputAction;

namespace EasyEIBindings
{
	/** Trigger events that can be auto-bound. */
	inline constexpr ETriggerEvent BindableEvents[] = {
		ETriggerEvent::Triggered,
		ETriggerEvent::Started,
		ETriggerEvent::Ongoing,
		ETriggerEvent::Completed,
		ETriggerEvent::Canceled
	};

	/** Returns the handler-name suffix for a trigger event, e.g. "Triggered". */
	EASYEIBINDINGS_API const TCHAR* GetEventSuffix(ETriggerEvent Event);

	/**
	 * Builds the handler function name for an action/event pair, e.g. "IA_Jump_Triggered".
	 * The configured Input Action prefix (default "IA_") is stripped from the asset name first.
	 */
	EASYEIBINDINGS_API FString MakeHandlerName(const UInputAction* InputAction, ETriggerEvent Event);
}

/**
 * A single Input Action to auto-bind, plus the trigger events to bind for it.
 */
USTRUCT(BlueprintType)
struct FEasyEIBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
	TObjectPtr<UInputAction> InputAction = nullptr;

	/** Which trigger events to bind. ETriggerEvent values are used directly as mask bits. Default: Triggered, Completed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding",
		meta = (Bitmask, BitmaskEnum = "/Script/EnhancedInput.ETriggerEvent"))
	int32 EnabledEvents = static_cast<int32>(ETriggerEvent::Triggered) | static_cast<int32>(ETriggerEvent::Completed);

	bool IsEventEnabled(ETriggerEvent Event) const
	{
		return (EnabledEvents & static_cast<int32>(Event)) != 0;
	}

	void SetEventEnabled(ETriggerEvent Event, bool bEnabled)
	{
		if (bEnabled)
		{
			EnabledEvents |= static_cast<int32>(Event);
		}
		else
		{
			EnabledEvents &= ~static_cast<int32>(Event);
		}
	}
};

/**
 * Binds Enhanced Input actions to functions on the owning actor by naming convention.
 *
 * For each configured Input Action, handler functions named "IA_<ActionName>_<Event>"
 * (e.g. "IA_Jump_Triggered") are looked up on the owner and bound automatically on
 * BeginPlay. Handlers can be C++ UFUNCTIONs or Blueprint custom events.
 */
UCLASS(ClassGroup=(Input), meta=(BlueprintSpawnableComponent, DisplayName="Easy EI Bindings"))
class EASYEIBINDINGS_API UEasyEIBindingsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEasyEIBindingsComponent();

	/** Input Actions to bind and which trigger events to bind them for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Easy EI Bindings")
	TArray<FEasyEIBinding> InputBindings;

	/**
	 * Binds all matching handler functions on the owning actor. Called automatically on BeginPlay.
	 * If no component is passed, the owner's current InputComponent is used.
	 * Calling this repeatedly adds duplicate bindings; use RebindInputActions instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Easy EI Bindings")
	virtual void SetupInputActions(UEnhancedInputComponent* EnhancedInputComponent = nullptr);

	/** Clears all bindings made by this component, then binds again (e.g. after a possession change). */
	UFUNCTION(BlueprintCallable, Category = "Easy EI Bindings")
	virtual void RebindInputActions();

	/** Removes all bindings made by this component from the owner's Enhanced Input component. */
	UFUNCTION(BlueprintCallable, Category = "Easy EI Bindings")
	virtual void ClearInputBindings();

protected:
	virtual void BeginPlay() override;

private:
	TArray<uint32> BoundActionHandles;
};
