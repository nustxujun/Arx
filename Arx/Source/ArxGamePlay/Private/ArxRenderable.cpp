#include "ArxRenderable.h"
#include "ArxWorld.h"

void IArxRenderable::LinkEntity(ArxEntityId EId, ArxWorld* InWorld)
{
	EntityId = EId;
	World = InWorld;
}

void IArxRenderable::UnlinkEntity()
{
	EntityId = INVALID_ENTITY_ID;
}

ArxEntityId IArxRenderable::GetEntityId()
{
	return EntityId;
}

AActor* IArxRenderable::GetActor()
{
	return Cast<AActor>(this);
}
