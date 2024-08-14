#include "ArxPhysicsSystem.h"

#include "ArxWorld.h"
#include "Rp3dWorld.h"

#include "EngineUtils.h"
#include "Rp3dShapeComponent.h"

DECLARE_CYCLE_STAT(TEXT("Physics World Update"), STAT_PhysicsWorldUpdate, STATGROUP_ArxGroup);

void ArxPhysicsSystem::Initialize(bool bIsReplicated)
{
	auto World = GetWorld().GetUnrealWorld();
	PhysicsWorld = NewObject<URp3dWorld>(World);

	FRp3dWorldSettings Settings;
	Settings.bAutoUpdate = false;
	PhysicsWorld->Initialize(Settings);
	PhysicsWorld->EnableDebugDraw(true);

	TArray<AActor*> List;
	for (TActorIterator<AActor> Iter(World); Iter; ++Iter)
	{
		auto Actor = *Iter;
		auto Root = Actor->GetRootComponent();
		if (!Root )
			continue;


		auto Comp = Actor->FindComponentByClass<URp3dShapeComponent>();
		if (Comp)
		{
			check(Root->Mobility == EComponentMobility::Static);
			List.Add(*Iter);
		}
	}

	List.StableSort([](const AActor& A, const AActor& B) 
	{
		return A.GetFullName() < B.GetFullName();
	});
	for (auto A : List)
	{
		A->FindComponentByClass<URp3dShapeComponent>()->OnCreateRp3dState(PhysicsWorld);
	}
}

void ArxPhysicsSystem::Uninitialize(bool bIsReplicated)
{
#if ENGINE_MAJOR_VERSION >= 5
	PhysicsWorld->MarkAsGarbage();
#else
	PhysicsWorld->MarkPendingKill();
#endif
	PhysicsWorld = 0;
}

void ArxPhysicsSystem::Serialize(ArxSerializer& Serializer) 
{

}

void ArxPhysicsSystem::Update(int FrameId)
{
	SCOPE_CYCLE_COUNTER(STAT_PhysicsWorldUpdate);

#if WITH_EDITOR
	static FCriticalSection Mutex;
	FScopeLock Lock(&Mutex);
#endif
	const auto SubstempTime = ArxConstants::TimeStep / ArxConstants::NumPhysicsStep;
	for (int i = 0; i < ArxConstants::NumPhysicsStep; ++i)
		PhysicsWorld->Step(FPToRp3d(SubstempTime));
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
