#pragma once 

#include "ArxSystem.h"
#include "ArxGamePlayCommon.h"

class ARXGAMEPLAY_API ArxRenderableSystem: public ArxSystem, public ArxEntityRegister<ArxRenderableSystem>
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

private:
    void Link(ArxEntityId EId, TFunction<UClass*()> GetClass);

    void AddTask(TFunction<void()> Func);
private:
    TMap<ArxEntityId, TWeakObjectPtr<AActor>> Actors;

};