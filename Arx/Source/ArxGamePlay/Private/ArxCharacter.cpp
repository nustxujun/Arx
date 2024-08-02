#include "ArxCharacter.h"
#include "ArxGamePlayCommon.h"

#include "ArxWorld.h"
#include "ArxTimerSystem.h"
#include "ArxRenderableSubsystem.h"
#include "ArxPhysicsSystem.h"

#include "Rp3dRigidBody.h"
#include "Rp3dCollisionShape.h"
#include "Rp3dUtils.h"

ArxCharacter::ArxCharacter(ArxWorld& World, ArxEntityId Id)
	:ArxEntity(World, Id)
{

}

void ArxCharacter::FContainer::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (RigidBody)
		Collector.AddReferencedObject(RigidBody);
}

void ArxCharacter::Initialize(bool bIsReplicated)
{
	FWorldContext* WorldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport);
	auto UnrealWorld = WorldContext->World();

	//Container.CollisionShape = URp3dCollisionShape::CreateCapsuleShape(50,100);
	Container.CollisionShape = Rp3dCollisionShape::CreateSphereShape(50);


	auto& PhysicsSys = GetWorld().GetSystem<ArxPhysicsSystem>();
	Container.RigidBody = PhysicsSys.CreateRigidBody();
	auto Collider = Container.RigidBody->AddCollisionShape(Container.CollisionShape);
	Collider.SetBounciness(0.5);
	Collider.SetFriction(0.0);

	Container.RigidBody->UpdateMassPropertiesFromColliders();
	Container.RigidBody->SetIsDebugEnabled(true);


	auto Pos = FVector(- 290.000000, -180.000000, 454.645386 + GetId() * 200 -300);
	auto Rot = FRotator(0,0,90);
	auto Trans = FTransform(Rot, Pos);
	Container.RigidBody->SetTransform(UE_TO_RP3D(Trans));
	//Container.RigidBody->SetAngularLockAxisFactor({0,0,0});
	//Container.RigidBody->EnableGravity(false);

	Gravity = {0,0,0};
	MoveVel = {0,0,0};

	if (bIsReplicated)
	{
		
	}
	else
	{ 
		auto& TimerSys = GetWorld().GetSystem<ArxTimerSystem>();
		Timer = TimerSys.AddTimer(GetId(),0,ArxConstants::TimeStep);

	}
}

void ArxCharacter::Uninitialize(bool bIsReplicated)
{
	if (!bIsReplicated)
	{
		auto& TimerSys = GetWorld().GetSystem<ArxTimerSystem>();
		TimerSys.RemoveTimer(Timer);

	}

	if (Container.RigidBody)
		Container.RigidBody->RemoveFromWorld();

	Container.CollisionShape.Reset();

	UArxRenderableSubsystem::Get(GetWorld().GetUnrealWorld()).Unlink(this);

}


void ArxCharacter::Spawn()
{
	auto Class = LoadObject<UClass>(nullptr, *CharacterBlueprint);
	//auto Blueprint = Cast<UBlueprint>(StaticLoadObject(UObject::StaticClass(), NULL, *CharacterBlueprint));
	check(Class);
	if (Class)
		UArxRenderableSubsystem::Get(GetWorld().GetUnrealWorld()).Link(this, Class);
}

void ArxCharacter::Serialize(ArxSerializer& Serializer)
{
	if (Serializer.IsSaving())
	{
		Rp3dTransform Trans = Container.RigidBody->GetTransform();
		auto LinearVel = Container.RigidBody->GetLinearVelocity();
		auto AngularVel = Container.RigidBody->GetAngularVelocity();
		ARX_SERIALIZE_MEMBER_FAST(Trans);
		ARX_SERIALIZE_MEMBER_FAST(LinearVel);
		ARX_SERIALIZE_MEMBER_FAST(AngularVel);
	}
	else
	{
		Rp3dTransform Trans;
		Rp3dVector3 LVel, AVel;
		Serializer << Trans << LVel << AVel;
		Container.RigidBody->SetTransform(Trans);
		Container.RigidBody->SetLinearVelocity(LVel);
		Container.RigidBody->SetAngularVelocity(AVel);
	}


	ARX_ENTITY_SERIALIZE_IMPL(Serializer);
}


void ArxCharacter::OnEvent(ArxEntityId Sender, uint64 Type, uint64 Param)
{
	if (GetWorld().GetSystem<ArxTimerSystem>().GetId() == Sender && Type == ArxTimerSystem::EVENT_ON_TIMER )
	{
		Update();
	}
}

void ArxCharacter::Update()
{
	//Gravity.z = Container.RigidBody->GetLinearVelocity().z - UE_TO_RP3D(1000) * (reactphysics3d::decimal)ArxConstants::TimeStep;
	//Container.RigidBody->SetLinearVelocity(MoveVel + Gravity);
}

URp3dRigidBody* ArxCharacter::GetRigidBody()
{
	return Container.RigidBody;
}

const Rp3dTransform& ArxCharacter::GetTransform()
{
	return Container.RigidBody->GetTransform();
}


void ArxCharacter::MoveDirectly(const Rp3dVector3& Dir)
{
	MoveVel = Dir;
}

void ArxCharacter::Move_Internal(ArxPlayerId PId, const Rp3dVector3& Dir)
{
	MoveDirectly(Dir);
}

