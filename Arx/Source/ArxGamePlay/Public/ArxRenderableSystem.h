#pragma once 

#include "ArxSystem.h"
#include "ArxGamePlayCommon.h"

class ARXGAMEPLAY_API ArxRenderableSystem: public ArxSystem, public ArxEntityRegister<ArxRenderableSystem>, public FTickableGameObject
{
    GENERATED_ARX_ENTITY_BODY()
public:
    using ArxSystem::ArxSystem;
    ~ArxRenderableSystem();
    virtual void Initialize(bool bIsReplicated) override;
    virtual void Uninitialize(bool bIsReplicated) override;

    virtual void Serialize(ArxSerializer& Serializer) override;
    virtual void Update(int FrameId) override;

    void Link(ArxEntityId EId, TSubclassOf<AActor> ActorClass);
    void Link(ArxEntityId EId, const FString& BlueprintPath);
    void Unlink(ArxEntityId EId);
    TWeakObjectPtr<AActor> GetActor(ArxEntityId EId);

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(ArxRenderableSystem, STATGROUP_Tickables); }

private:
    void Link(ArxEntityId EId, TFunction<UClass*()> GetClass);

    void AddTask(TFunction<void()> Func);
private:
    TMap<ArxEntityId, TWeakObjectPtr<AActor>> Actors;
    FCriticalSection Mutex;

    TArray<TFunction<void()>> Tasks;
    FCriticalSection TaskMutex;
};