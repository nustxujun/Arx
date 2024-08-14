#include "ArxRenderableSystem.h"
#include "ArxRenderable.h"
#include "ArxWorld.h"

ArxRenderableSystem::~ArxRenderableSystem()
{
}


void ArxRenderableSystem::Initialize(bool bIsReplicated)
{
}

void ArxRenderableSystem::Uninitialize(bool bIsReplicated)
{
    if (bIsReplicated)
        return;

    check(IsInGameThread());

    Tick(0);
    auto World = GetWorld().GetUnrealWorld();
    for (auto& Item : Actors)
    {
        World->DestroyActor(Item.Value.Get());
    }
    Actors.Reset();
}

void ArxRenderableSystem::Serialize(ArxSerializer& Serializer)
{
}

void ArxRenderableSystem::Update(int FrameId)
{
    TArray<AActor*> List;
    {
        FScopeLock Lock(&Mutex);
        for (auto& Item: Actors)
        {
            List.Add(Item.Value.Get());
        }
    }

    for (auto Actor : List)
    {
        Cast<IArxRenderable>(Actor)->OnFrame_Async(FrameId);
    }
}

void ArxRenderableSystem::Link(ArxEntityId EId, TSubclassOf<AActor> ActorClass)
{
    check(ActorClass->IsChildOf<AActor>());
    check(ActorClass->ImplementsInterface(UArxRenderable::StaticClass()));
    Link(EId, [ActorClass]() {
        return ActorClass.Get();
    });
}

void ArxRenderableSystem::Link(ArxEntityId EId, const FString & BlueprintPath)
{
    Link(EId, [BlueprintPath](){
        return LoadObject<UClass>(nullptr, *BlueprintPath);
    });
}

void ArxRenderableSystem::Link(ArxEntityId EId, TFunction<UClass* ()> GetClass)
{
    TWeakObjectPtr<UWorld> WeakWorld = GetWorld().GetUnrealWorld();

    AddTask([WeakWorld, GetClass = MoveTemp(GetClass), EId, this]() {
        if (!WeakWorld.IsValid())
            return;

        auto Actor = GetActor(EId);
        if (Actor.IsValid(false, true))
        {
            return;
        }


        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto Class = GetClass();
        check(Class);
        if (!Class)
            return;
        auto NewActor = Cast<AActor>(WeakWorld->SpawnActor(Class, NULL, NULL, Parameters));
        check(NewActor);
        Cast<IArxRenderable>(NewActor)->LinkEntity(EId,&GetWorld());
        FScopeLock Lock(&Mutex);
        Actors.Add(EId, NewActor);
    });

}

void ArxRenderableSystem::Unlink(ArxEntityId EId)
{
    TWeakObjectPtr<UWorld> WeakWorld = GetWorld().GetUnrealWorld();
    AddTask([WeakWorld,EId, this](){
        if (!WeakWorld.IsValid())
            return;

        auto Actor = GetActor(EId);
        {
            FScopeLock Lock(&Mutex);
            Actors.Remove(EId);
        }
        if (!Actor.IsValid(false, true))
            return;

        Cast<IArxRenderable>(Actor)->UnlinkEntity();
        WeakWorld->DestroyActor(Actor.Get());
    });

}

TWeakObjectPtr<AActor> ArxRenderableSystem::GetActor(ArxEntityId EId)
{
    FScopeLock Lock(&Mutex);
    return Actors.FindRef(EId);
}

void ArxRenderableSystem::AddTask(TFunction<void()> Func)
{
    FScopeLock Lock(&TaskMutex);
    Tasks.Add(MoveTemp(Func));
}

void ArxRenderableSystem::Tick(float DeltaTime)
{
    auto Type = GetWorld().GetUnrealWorld()->WorldType;
    if (Type == EWorldType::Game || Type == EWorldType::PIE)
    { 
        FScopeLock Lock(&TaskMutex);
        for (auto& Task : Tasks)
        {
            Task();
        }
    }
    Tasks.Reset();
}