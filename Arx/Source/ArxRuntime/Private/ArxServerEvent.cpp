#include "ArxServerEvent.h"
#include "ArxEvent.h"
#include "ArxWorld.h"

void ArxServerCommand::Serialize(ArxSerializer& Ser)
{
    Ser << Event << PlayerId;
}

void ArxServerCommand::Execute(ArxWorld& World) const
{
    World.GetSystem<ArxEventSystem>().FireEvent(0,Event, PlayerId);
}
