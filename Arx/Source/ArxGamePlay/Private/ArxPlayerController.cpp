#include "ArxPlayerController.h"
#include "ArxWorld.h"
#include "ArxCharacter.h"
#include "ArxRenderableSubsystem.h"

ArxPlayerController::ArxPlayerController(ArxWorld& InWorld, ArxEntityId Id):ArxSystem(InWorld, Id)
{

}

void ArxPlayerController::Initialize(bool bIsReplicated)
{
}

void ArxPlayerController::Serialize(ArxSerializer& Serializer) 
{
	ARX_SERIALIZE_MEMBER_FAST(EntityIds);
}


void ArxPlayerController::CreateCharacter_Internal(ArxPlayerId PId,FName EntityType, FString ClassPath)
{
	auto Ent =  GetWorld().CreateEntity(PId, EntityType);
	auto Chara = static_cast<ArxCharacter*>(Ent);

	Chara->CharacterBlueprint = ClassPath;

	Chara->Spawn();
	auto EId = EntityIds.FindRef(PId);
	if (EId != 0)
	{
		GetWorld().DestroyEntity(EId);
	}

	EntityIds.Add(PId, Ent->GetId());
}

void ArxPlayerController::Move_Internal(ArxPlayerId PId, Rp3dVector3 Vec)
{
	auto& Chara = GetWorld().GetEntity<ArxCharacter>(EntityIds[PId]);
	//auto Chara = static_cast<ArxCharacter*>(GetWorld().GetEntity(EntityId));
	Chara.Move(Vec);
}

AActor* ArxPlayerController::GetLinkedActor(ArxPlayerId PId)
{
	auto EId = EntityIds.FindRef(PId);
	if (EId == 0)
		return nullptr;
	auto Actor = GetWorld().GetUnrealWorld()->GetSubsystem<UArxRenderableSubsystem>()->GetActor(GetWorld().GetEntity(EId));
	return Actor;
}