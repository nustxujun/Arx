#pragma once 

#include "ArxSystem.h"
#include "ArxCommandSystem.h"

class ARXGAMEPLAY_API ArxPlayerController : public ArxSystem, public ArxEntityRegister<ArxPlayerController>
{
    GENERATED_ARX_ENTITY_BODY()
public:
    ArxPlayerController(ArxWorld& InWorld, ArxEntityId Id);

    void Initialize(bool bIsReplicated ) override;
    void Serialize(ArxSerializer& Ser) override;

    EXPOSED_ENTITY_METHOD(CreateCharacter, FName EntityType, FString ClassPath);
    EXPOSED_ENTITY_METHOD(Move, Rp3dVector3);

    AActor* GetLinkedActor(ArxPlayerId Id);
private:
    TSortedMap<ArxPlayerId, ArxEntityId> EntityIds;
};