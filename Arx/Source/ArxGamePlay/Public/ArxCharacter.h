#pragma once 

#include "CoreMinimal.h"
#include "ArxEntity.h"
#include "ArxCommandSystem.h"
#include "Rp3dCommon.h"
#include "ArxEvent.h"

class ARXGAMEPLAY_API ArxCharacter : public ArxEntity, public ArxEntityRegister<ArxCharacter>
{
	GENERATED_ARX_ENTITY_BODY()
public:
	ArxCharacter(ArxWorld& World, ArxEntityId Id);

	void Initialize(bool bIsReplicated = false) override;
	void Uninitialize(bool bIsReplicated) override;
	void Serialize(ArxSerializer& Serializer) override;
	void Spawn()override;

	void OnEvent(ArxEntityId Sender, uint64 Type, uint64 Param) override;
	void Update();
	
	const Rp3dTransform& GetTransform();

	URp3dRigidBody* GetRigidBody();

	void MoveDirectly(const Rp3dVector3& Dir);


	EXPOSED_ENTITY_METHOD(Move, const Rp3dVector3& Dir)


public:

	REFLECT_BEGIN()
	REFLECT_FIELD(int, Timer)
	REFLECT_FIELD(FString, CharacterBlueprint)
	REFLECT_END()

private:

	struct FContainer: public FGCObject
	{
		virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

		URp3dRigidBody* RigidBody = nullptr;
		TSharedPtr<Rp3dCollisionShape> CollisionShape ;
	};
	
	FContainer Container;
	Rp3dVector3 MoveVel;
	Rp3dVector3 Gravity;
};