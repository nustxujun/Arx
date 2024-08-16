#pragma once 

#include "ArxSystem.h"
#include "ArxGamePlayCommon.h"


class ARXGAMEPLAY_API ArxPhysicsSystem : public ArxSystem, public ArxEntityRegister<ArxPhysicsSystem>
{
    GENERATED_ARX_ENTITY_BODY()

public:
    using ArxSystem::ArxSystem;
    virtual void Initialize(bool bIsReplicated) override;
    virtual void Uninitialize(bool bIsReplicated) override;

    virtual void Serialize(ArxSerializer& Serializer) override;
    virtual void Update(int FrameId) override;

    TSharedPtr<class FRp3dRigidBody> CreateRigidBody();
private:
    TSharedPtr<class FRp3dPhysicsWorld> PhysicsWorld;
};