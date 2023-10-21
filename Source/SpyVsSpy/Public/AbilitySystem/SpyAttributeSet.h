// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "SpyAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 
 */
UCLASS()
class SPYVSSPY_API USpyAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:

	USpyAttributeSet();

	/** Class Overrides */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	/** for when character dies */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "SVS|Attributes|Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(USpyAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "SVS|Attributes|Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(USpyAttributeSet, MaxHealth);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Damage, Category = "SVS|Attributes|Combat")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(USpyAttributeSet, Damage);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DefensePower, Category = "SVS|Attributes|Combat")
	FGameplayAttributeData DefensePower;
	ATTRIBUTE_ACCESSORS(USpyAttributeSet, DefensePower);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower, Category = "SVS|Attributes|Combat")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(USpyAttributeSet, AttackPower);

	/** Updates attribute if its max value is changed */
	void AdjustAttributeForMaxChange(
		const FGameplayAttributeData& AffectedAttribute,
		const FGameplayAttributeData& MaxAttribute,
		float NewMaxValue,
		const FGameplayAttribute& AffectedAttributeProperty) const;

	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_Damage(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_DefensePower(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_AttackPower(const FGameplayAttributeData& OldValue);
};
