#include "ArxRenderCharacter.h"

#include "ArxWorld.h"
#include "ArxCharacter.h"

AArxRenderCharacter::AArxRenderCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
}

void AArxRenderCharacter::LinkEntity(ArxEntity* Ent)
{
	Super::LinkEntity(Ent);
	check(Ent->GetClassName() == ArxCharacter::GetTypeName())
	auto Char = static_cast<ArxCharacter*>(Ent);
	auto& Trans = Char->GetTransform();
	CachedTrans = Trans;
	SetActorTransform(RP3D_TO_UE(CachedTrans));

	SetActorTickEnabled(true);
}

void AArxRenderCharacter::UnlinkEntity()
{
	Super::UnlinkEntity();

	SetActorTickEnabled(false);
}

void AArxRenderCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	auto Ent = GetEntity();
	check(Ent)
	auto Char = static_cast<ArxCharacter*>(Ent);

	auto& Trans = Char->GetTransform();
	if (CachedTrans == Trans)
		return;
	CachedTrans = Trans;
	SetActorTransform(RP3D_TO_UE(CachedTrans));
}
