#include "ArxEntity.h"
#include "ArxWorld.h"

ArxEntity::ArxEntity(ArxWorld& InWorld, ArxEntityId Id)
	: WorldEntity(InWorld), EntityId(Id)
{
}

ArxEntity::~ArxEntity()
{
}

ArxWorld& ArxEntity::GetWorld()
{
	return WorldEntity;
}

void ArxEntity::AddFactory(FName Name, TFunction<ArxEntity* (ArxWorld&, ArxEntityId)> Factory)
{
	GetFactories().Add(Name, MoveTemp(Factory));
}