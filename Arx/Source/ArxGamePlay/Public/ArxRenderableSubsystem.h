#pragma once 

#include "CoreMinimal.h"
#include "ArxGamePlayCommon.h"
#include "ArxWorld.h"
#include "ArxRenderableSubsystem.generated.h"


UCLASS()
class ARXGAMEPLAY_API UArxRenderableSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    static UArxRenderableSubsystem& Get(UWorld* );

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void Link(ArxEntity* Entity, TSubclassOf<AActor> ActorClass);
    void Unlink(ArxEntity* Entity);

    AActor* GetActor(ArxEntity* Entty);

    void OnFrame(ArxWorld* World,ArxPlayerId PId, int FrameId);

    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const{return true;}

private:
    bool IsInGame()const;
private:
    TMap<ArxEntityId, TWeakObjectPtr<AActor>> Actors;
    FDelegateHandle StepHandle;
};