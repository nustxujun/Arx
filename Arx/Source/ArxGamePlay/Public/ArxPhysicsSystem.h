#pragma once 

#include "ArxSystem.h"
#include "ArxGamePlayCommon.h"


class ARXGAMEPLAY_API ArxPhysicsSystem : public ArxSystem, public ArxEntityRegister<ArxPhysicsSystem>,public FGCObject
{
    GENERATED_ARX_ENTITY_BODY()

public:
    using ArxSystem::ArxSystem;
    virtual void Initialize(bool bIsReplicated) override;
    virtual void Uninitialize(bool bIsReplicated) override;

    virtual void Serialize(ArxSerializer& Serializer) override;
    virtual void Update() override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    FString GetReferencerName(void) const{return TEXT("ArxPhysicsSystem"); }

    URp3dRigidBody* CreateRigidBody();
private:
    class URp3dWorld* PhysicsWorld;
};