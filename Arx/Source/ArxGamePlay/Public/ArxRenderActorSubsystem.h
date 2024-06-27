#pragma once 

#include "CoreMinimal.h"
#include "ArxGamePlayCommon.h"
#include "ArxWorld.h"
#include "ArxRenderActorSubsystem.generated.h"

class AArxRenderActor;

UCLASS()
class ARXGAMEPLAY_API UArxRenderActorSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    static UArxRenderActorSubsystem& Get(UWorld* );

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void LinkActor(ArxEntity* Entity, TSubclassOf<AArxRenderActor> ActorClass);
    void UnlinkActor(ArxEntity* Entity);

private:
    TMap<ArxEntityId, AArxRenderActor*> Actors;
};