#include "ArxEntity.h"
#include "ArxWorld.h"

ArxEntity::ArxEntity(ArxWorld& InWorld, ArxEntityId Id)
	: World(InWorld), EntityId(Id)
{
}

ArxEntity::~ArxEntity()
{
}

ArxWorld& ArxEntity::GetWorld()
{
	return World;
}

