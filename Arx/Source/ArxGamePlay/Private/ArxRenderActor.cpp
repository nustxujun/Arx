#include "ArxRenderActor.h"
#include "ArxWorld.h"

void AArxRenderActor::LinkEntity(ArxEntity* Ent)
{
	World = &Ent->GetWorld();
	EntityId = Ent->GetId();
}

void AArxRenderActor::UnlinkEntity()
{
	EntityId = INVALID_ENTITY_ID;
}

ArxEntity* AArxRenderActor::GetEntity()
{
	if (EntityId == INVALID_ENTITY_ID)
		return nullptr;
	return World->GetEntity(EntityId);
}

