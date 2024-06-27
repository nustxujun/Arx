#pragma once 

#include "CoreMinimal.h"
#include "ArxEntity.h"
#include "Rp3dCommon.h"

class ARXGAMEPLAY_API ArxCharacter : public ArxBasicEntity<ArxCharacter>
{
public:
	ArxCharacter(ArxWorld& World, ArxEntityId Id);

	void Initialize(bool bIsReplicated = false) override;
	void Uninitialize() override;
	void Serialize(ArxSerializer& Serializer) override;

	void OnEvent(uint64 Type, uint64 Param) override;
	uint32 GetHash() override;
	void Update();
	
	const Rp3dTransform& GetTransform();

	URp3dRigidBody* GetRigidBody();
private:

	REFLECT_BEGIN();
	REFLECT_FIELD(int, Timer);
	REFLECT_END();

private:

	struct FContainer: public FGCObject
	{
		virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

		URp3dRigidBody* RigidBody = nullptr;
		URp3dCollisionShape* CollisionShape = nullptr;
	};
	
	FContainer Container;

};