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
    AddTask([FrameId,this]( ){
        for (auto Actor : Actors)
        {
            Cast<IArxRenderable>(Actor.Value)->OnFrame(FrameId);
        }
    });
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
            
        Actors.Remove(EId);
        
        if (!Actor.IsValid(false, true))
            return;

        Cast<IArxRenderable>(Actor)->UnlinkEntity();
        WeakWorld->DestroyActor(Actor.Get());
    });

}

TWeakObjectPtr<AActor> ArxRenderableSystem::GetActor(ArxEntityId EId)
{
    check(IsInGameThread());
    return Actors.FindRef(EId);
}

void ArxRenderableSystem::AddTask(TFunction<void()> Func)
{
    GetWorld().RequestAccessInGameThread([Func = MoveTemp(Func)](const ArxWorld& World) {
        Func();
    });
}

