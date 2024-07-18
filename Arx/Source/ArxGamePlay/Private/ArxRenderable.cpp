#include "ArxRenderable.h"
#include "ArxWorld.h"

void IArxRenderable::LinkEntity(ArxEntity* Ent)
{
	World = &Ent->GetWorld();
	EntityId = Ent->GetId();
}

void IArxRenderable::UnlinkEntity()
{
	EntityId = INVALID_ENTITY_ID;
}

ArxEntity* IArxRenderable::GetEntity()
{
	if (EntityId == INVALID_ENTITY_ID)
		return nullptr;
	return World->GetEntity(EntityId);
}

AActor* IArxRenderable::GetActor()
{
	return Cast<AActor>(this);
}
