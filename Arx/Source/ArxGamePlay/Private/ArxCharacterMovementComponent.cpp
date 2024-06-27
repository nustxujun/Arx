#include "ArxCharacterMovementComponent.h"
#include "ArxCharacter.h"

#include "Rp3dCapsuleComponent.h"

UArxCharacterMovementComponent::UArxCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UArxCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const auto InputVector = ConsumeInputVector();
}

void UArxCharacterMovementComponent::SetCapsuleComponent(URp3dCapsuleComponent* Capsule)
{
	CapsuleComponent = Capsule;
}