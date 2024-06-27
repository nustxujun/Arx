#include "ArxRenderActorSubsystem.h"
#include "ArxRenderActor.h"

#include "ArxEntity.h"


void UArxRenderActorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{

}

void UArxRenderActorSubsystem::Deinitialize() 
{
}

UArxRenderActorSubsystem& UArxRenderActorSubsystem::Get(UWorld* World)
{
    return *World->GetSubsystem<UArxRenderActorSubsystem>();
}

void UArxRenderActorSubsystem::LinkActor(ArxEntity* Entity, TSubclassOf<AArxRenderActor> ActorClass)
{
    check( ActorClass->IsChildOf<AArxRenderActor>() );
    auto Actor = Actors.FindRef(Entity->GetId());
    if (Actor)
    {
        if (!Actor->IsA(ActorClass))
        {
            GetWorld()->DestroyActor(Actor);
            Actor = nullptr;
        }
        else
            return;
    }
    
    if (!Actor)
    {
        Actor = Cast<AArxRenderActor>(GetWorld()->SpawnActor(ActorClass));
        check(Actor);
        Actors.Add(Entity->GetId(), Actor);
    }

    Actor->LinkEntity(Entity);
}


void UArxRenderActorSubsystem::UnlinkActor(ArxEntity* Entity)
{
    auto Actor = Actors.FindRef(Entity->GetId());
    if (!Actor)
        return;

    Actors.Remove(Entity->GetId());
    Actor->UnlinkEntity();
    GetWorld()->DestroyActor(Actor);
}


