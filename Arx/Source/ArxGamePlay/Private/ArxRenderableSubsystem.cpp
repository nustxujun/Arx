#include "ArxRenderableSubsystem.h"
#include "ArxRenderable.h"
#include "ArxDelegates.h"
#include "ArxEntity.h"


void UArxRenderableSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    if (IsInGame())
        StepHandle = ArxDelegates::OnClientWorldStep.AddUObject(this, &UArxRenderableSubsystem::OnFrame);
}

void UArxRenderableSubsystem::Deinitialize() 
{
    ArxDelegates::OnClientWorldStep.Remove(StepHandle);
}

UArxRenderableSubsystem& UArxRenderableSubsystem::Get(UWorld* World)
{
    return *World->GetSubsystem<UArxRenderableSubsystem>();
}

bool UArxRenderableSubsystem::IsInGame()const
{
    auto Type = GetWorld()->WorldType;
    return Type == EWorldType::Game || Type == EWorldType::PIE;
}

void UArxRenderableSubsystem::Link(ArxEntity* Entity, TSubclassOf<AActor> ActorClass)
{
    if (!IsInGame())
        return;

    check(Entity->GetWorld().GetUnrealWorld() == GetWorld())
    check( ActorClass->IsChildOf<AActor>() );
    check(ActorClass->ImplementsInterface(UArxRenderable::StaticClass()))
    auto Actor = GetActor(Entity);
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
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        Actor = Cast<AActor>(GetWorld()->SpawnActor(ActorClass,NULL,NULL, Parameters));
        check(Actor);
        Actors.Add(Entity->GetId(), Actor);
    }

    Cast<IArxRenderable>(Actor)->LinkEntity(Entity);
}


void UArxRenderableSubsystem::Unlink(ArxEntity* Entity)
{
    if (!IsInGame())
        return;

    auto Actor = GetActor(Entity);
    if (!Actor)
        return;

    Actors.Remove(Entity->GetId());
    Cast<IArxRenderable>(Actor)->UnlinkEntity();
    GetWorld()->DestroyActor(Actor);
}


AActor* UArxRenderableSubsystem::GetActor(ArxEntity* Entity)
{
    auto Actor = Actors.FindRef(Entity->GetId());
    if (Actor.IsValid())
        return Actor.Get();
    return nullptr;
}

void UArxRenderableSubsystem::OnFrame(ArxWorld* World, ArxPlayerId PId, int FrameId)
{
    if (World->GetUnrealWorld() != GetWorld())
        return;

    for (auto& Item : Actors)
    {
        if (Item.Value.IsValid())
            Cast<IArxRenderable>(Item.Value)->OnFrame(FrameId);
    }
}

