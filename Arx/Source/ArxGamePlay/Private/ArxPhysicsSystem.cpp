#include "ArxPhysicsSystem.h"

#include "ArxWorld.h"
#include "Rp3dWorld.h"

#include "EngineUtils.h"
#include "Rp3dShapeComponent.h"


void ArxPhysicsSystem::Initialize(bool bIsReplicated)
{
	auto World = GetWorld().GetUnrealWorld();
	PhysicsWorld = NewObject<URp3dWorld>(World);

	FRp3dWorldSettings Settings;
	Settings.bAutoUpdate = false;
	PhysicsWorld->Initialize(Settings);
	PhysicsWorld->EnableDebugDraw(true);

	for (TActorIterator<AActor> Iter(World); Iter; ++Iter)
	{
		auto Comp = Iter->FindComponentByClass<URp3dShapeComponent>();
		if (Comp)
			Comp->OnCreateRp3dState(PhysicsWorld);
	}
	
}

void ArxPhysicsSystem::Uninitialize(bool bIsReplicated)
{
	PhysicsWorld = 0;
}

void ArxPhysicsSystem::Serialize(ArxSerializer& Serializer) 
{

}

void ArxPhysicsSystem::Update()
{
	constexpr auto SubstempTime = ArxConstants::TimeStep / ArxConstants::NumPhysicsStep;
	for (int i = 0; i < ArxConstants::NumPhysicsStep; ++i)
		PhysicsWorld->Step((reactphysics3d::decimal)SubstempTime);
}

void ArxPhysicsSystem::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(PhysicsWorld);
}

URp3dRigidBody* ArxPhysicsSystem::CreateRigidBody()
{
	auto Rigidbody = NewObject<URp3dRigidBody>(PhysicsWorld);
	Rigidbody->Initialize(PhysicsWorld);
	return Rigidbody;
}
