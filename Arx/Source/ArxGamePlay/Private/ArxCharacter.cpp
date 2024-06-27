#include "ArxCharacter.h"
#include "ArxGamePlayCommon.h"

#include "ArxWorld.h"
#include "ArxTimerSystem.h"
#include "ArxRenderActorSubsystem.h"

#include "Rp3dRigidBody.h"
#include "Rp3dCollisionShape.h"
#include "Rp3dUtils.h"


ArxCharacter::ArxCharacter(ArxWorld& World, ArxEntityId Id)
	:ArxBasicEntity(World, Id)
{

}

void ArxCharacter::FContainer::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (RigidBody)
		Collector.AddReferencedObject(RigidBody);
	if (CollisionShape)
		Collector.AddReferencedObject(CollisionShape);
}

void ArxCharacter::Initialize(bool bIsReplicated)
{
	FWorldContext* WorldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport);
	auto UnrealWorld = WorldContext->World();

	Container.CollisionShape = URp3dCollisionShape::CreateCapsuleShape(40, 100);
	Container.RigidBody = NewObject<URp3dRigidBody>(UnrealWorld);
	auto Collider = Container.RigidBody->AddCollisionShape(Container.CollisionShape);
	Container.RigidBody->UpdateMassPropertiesFromColliders();
	Container.RigidBody->SetIsDebugEnabled(true);


	auto Pos = FVector(- 290.000000, -180.000000, 454.645386 + GetId() * 200 -300);
	auto Rot = FRotator(0,0,90);
	auto Trans = FTransform(Rot, Pos);
	Container.RigidBody->SetTransform(UE_TO_RP3D(Trans));
	//Container.RigidBody->SetAngularLockAxisFactor({0,0,1});
	//Container.RigidBody->EnableGravity(false);
	Collider.SetBounciness(0.5);

	if (bIsReplicated)
	{
		
	}
	else
	{ 
		auto& TimerSys = World.GetSystem<ArxTimerSystem>();
		Timer = TimerSys.AddTimer(GetId(), 5);
	}

	auto Blueprint = Cast<UBlueprint>(StaticLoadObject(UObject::StaticClass(), NULL, TEXT("/Game/ThirdPersonCPP/Blueprints/RenderCharacter.RenderCharacter")));
	UArxRenderActorSubsystem::Get(GetWorld().GetUnrealWorld()).LinkActor(this, Blueprint->GeneratedClass.Get());
}
#pragma optimize("",off)
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
#pragma optimize("",on)


void ArxCharacter::Uninitialize()
{
	auto& TimerSys = GetWorld().GetSystem<ArxTimerSystem>();
	TimerSys.RemoveTimer(Timer);

	if (Container.RigidBody)
		Container.RigidBody->RemoveFromWorld();

	UArxRenderActorSubsystem::Get(GetWorld().GetUnrealWorld()).UnlinkActor(this);

}



void ArxCharacter::OnEvent(uint64 Type, uint64 Param)
{
	switch (Type)
	{
	case ArxEntity::ON_TIMER:
		Update();
		break;
	default:
		break;
	}
}

void ArxCharacter::Update()
{
	//GetWorld().DestroyEntity(GetId());
}

URp3dRigidBody* ArxCharacter::GetRigidBody()
{
	return Container.RigidBody;
}

uint32 ArxCharacter::GetHash()
{
	uint32 Hash = GetTypeHash(GetTransform());


	return Hash;
}

const Rp3dTransform& ArxCharacter::GetTransform()
{
	return Container.RigidBody->GetTransform();
}


//#include "Rp3dCapsuleComponent.h"
//#include "Components/SkinnedMeshComponent.h"
//#include "ArxCharacterMovementComponent.h"
//
//#include "reactphysics3d/body/RigidBody.h"
//
//static FName CharacterCollider(TEXT("CharacterCollider"));
//static FName CharacterMoveComp(TEXT("CharacterMoveComp"));
//static FName CharacterMeshComp(TEXT("CharacterMeshComp"));
//
//AArxCharacter::AArxCharacter(const FObjectInitializer& ObjectInitializer)
//{
//	CapsuleComponent = CreateDefaultSubobject<URp3dCapsuleComponent>(CharacterCollider);
//	RootComponent = CapsuleComponent;
//
//	CharacterMovement = CreateDefaultSubobject<UArxCharacterMovementComponent>(CharacterMoveComp);
//	CharacterMovement->SetCapsuleComponent(CapsuleComponent);
//
//	Mesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(CharacterMeshComp);
//	if (Mesh)
//	{
//		Mesh->AlwaysLoadOnClient = true;
//		Mesh->AlwaysLoadOnServer = true;
//		Mesh->bOwnerNoSee = false;
//		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
//		Mesh->bCastDynamicShadow = true;
//		Mesh->bAffectDynamicIndirectLighting = true;
//		Mesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
//		Mesh->SetupAttachment(CapsuleComponent);
//		static FName MeshCollisionProfileName(TEXT("CharacterMesh"));
//		Mesh->SetCollisionProfileName(MeshCollisionProfileName);
//		Mesh->SetGenerateOverlapEvents(false);
//		Mesh->SetCanEverAffectNavigation(false);
//	}
//}
//
//void AArxCharacter::BeginPlay()
//{
//	Super::BeginPlay();
//
//	CapsuleComponent->SetAngularLockAxisFactor({0,0,1});
//	CapsuleComponent->SetBounciness(0);
//}
//
//void AArxCharacter::GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const
//{
//	CollisionRadius = CapsuleComponent->GetRadius();
//	CollisionHalfHeight = CapsuleComponent->GetHalfHeight();
//}
//
//
//void AArxCharacter::MoveForward(float Value)
//{
//	if ((Controller != nullptr) && (Value != 0.0f))
//	{
//		// find out which way is forward
//		const FRotator Rotation = Controller->GetControlRotation();
//		const FRotator YawRotation(0, Rotation.Yaw, 0);
//
//		// get forward vector
//		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
//		AddMovementInput(Direction, Value);
//	}
//}
