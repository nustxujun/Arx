#include "ArxWorld.h"
#include "ArxCommandSystem.h"

#include "CoreMinimal.h"

struct ScopedBoolean
{
	bool& Value;
	ScopedBoolean(bool& Val):Value(Val){ Value = true; };
	~ScopedBoolean() { Value = false; }
};

#define SCOPED_DETER() ScopedBoolean __SB(bDeterministic)


ArxWorld::ArxWorld(UWorld* InWorld)
    :ArxEntity(*this, 0), UnrealWorld(InWorld)
{
	Entities.Reserve(10000);
	AddInternalSystem();
}

ArxWorld::~ArxWorld()
{
	const int Count = Entities.Num();
	for (int i = 0; i < Count; ++i) 
	{
		Entities[i]->Uninitialize();
	}
	for (int i = 0; i < Count; ++i)
		delete Entities[i];
}

void ArxWorld::Setup(const TFunction<void(ArxWorld&)>& Callback)
{
	SCOPED_DETER();
	Callback(*this);
}

void ArxWorld::AddInternalSystem()
{
	SCOPED_DETER();

	AddSystem<ArxCommandSystem>();
}

ArxEntity* ArxWorld::CreateEntity(ArxPlayerId PId , FName TypeName)
{
	CHECK_DETER();

	auto Fac = ArxEntity::GetFactories().Find(TypeName);
	check(Fac);

	ArxEntityId Id = UniqueId++;
	auto Ent = (*Fac)(*this, Id);
	Ent->SetPlayerId(PId);
	Ent->Initialize(false);
	IdMap.Add(Id, Entities.Add(Ent));

	return Ent;
}


void ArxWorld::DestroyEntity(ArxEntityId Id)
{
	CHECK_DETER();
	if (!DeadList.Contains(Id))
		DeadList.Add(Id);
}

bool ArxWorld::IsEntityValid(ArxEntityId Id) 
{
	return IdMap.Contains(Id);
}

void ArxWorld::Update()
{
	SCOPED_DETER();
	for (auto& System : Systems)
	{
		System->PreUpdate();
	}

	for (auto& System : Systems)
	{
		System->Update();
	}

	for (auto& System : Systems)
	{
		System->PostUpdate();
	}


	ClearDeadEntities();
}
#pragma optimize("",off)

struct ClassInfo
{
	FName Class;
	uint32 IId;
	ArxPlayerId PId;

	friend static ArxBasicSerializer& operator <<(ArxBasicSerializer& Serializer, ClassInfo& CI)
	{
		//ARX_SERIALIZE_MEMBER_FAST(CI.Class);
		ARX_SERIALIZE_MEMBER_FAST(CI.IId);
		//ARX_SERIALIZE_MEMBER_FAST(CI.PId);

		return Serializer;
	}

	friend static FString LexToString(const ClassInfo& CI)
	{
		return FString::Printf(TEXT("{%s, %d, %d}"), *CI.Class.ToString(), CI.IId, CI.PId);
	}
};

void ArxWorld::Serialize(ArxSerializer& Serializer)
{
	ARX_SERIALIZE_MEMBER_FAST(UniqueId);

	if (Serializer.IsSaving())
	{
		int EntCount = IdMap.Num();
		ARX_SERIALIZE_MEMBER_FAST(EntCount);
		int Space = Entities.GetMaxIndex();
		ARX_SERIALIZE_MEMBER_FAST(Space);
		for (auto& Item : IdMap)
		{
			ARX_SERIALIZE_MEMBER_FAST(Item)
			//Serializer << Item.Key;
			//Serializer << Item.Value;

			auto Ent = Entities[Item.Value];
			auto TypeName = Ent->GetClassName();
			ARX_SERIALIZE_MEMBER_FAST(TypeName);
			auto PId = Ent->GetPlayerId();
			ARX_SERIALIZE_MEMBER_FAST(PId);
		}

	}
	else
	{
		int EntCount, Space;
		ARX_SERIALIZE_MEMBER_FAST(EntCount);
		ARX_SERIALIZE_MEMBER_FAST(Space);




		TArray<TPair<ArxEntityId, ClassInfo>> NewList;
		for (int i = 0; i < EntCount; ++i)
		{
			ArxEntityId EId; 
			ClassInfo CI;

			Serializer << EId << CI.IId << CI.Class << CI.PId;

			NewList.Add(TPair<ArxEntityId, ClassInfo>{EId, CI});
		}

		TArray<TPair<ArxEntityId, ArxEntity*>> OldList;

		for (auto& Item : IdMap)
		{
			OldList.Add(TPair<ArxEntityId, ArxEntity*>{Item.Key, Entities[Item.Value]});
		}
		IdMap.Reset();

		Entities.Reset();
		Entities.Reserve(Space);

		auto SortFunc = [](auto& a, auto& b){
			return a.Key < b.Key;
		};

		auto CreateEnt = [this](ArxEntityId EId, ArxPlayerId PId, FName Class){
			auto Fac = ArxEntity::GetFactories().Find(Class);
			check(Fac);

			auto Ent = (*Fac)(*this, EId);
			Ent->SetPlayerId(PId);
			Ent->Initialize(true);
			return Ent;
		};

		NewList.StableSort(SortFunc);
		OldList.StableSort(SortFunc);
		TArray<ArxEntity*> RemoveList;
		int Index = 0;
		while (true)
		{
			if (NewList.Num() <= Index || OldList.Num() <= Index)
			{
				break;
			}
			
			auto& New = NewList[Index];
			auto& Old = OldList[Index];

			if (New.Key == Old.Key)
			{
				//Entities[New.Value.IId] = Old.Value;
				Entities.Insert(New.Value.IId, Old.Value);
				IdMap.Add(New.Key, New.Value.IId);
				Index++;

			}
			else if (New.Key < Old.Key)
			{
				auto Ent = CreateEnt(New.Key, New.Value.PId, New.Value.Class);
				//Entities[New.Value.IId] = Ent;
				Entities.Insert(New.Value.IId, Ent);

				IdMap.Add(New.Key, New.Value.IId);

				OldList.Insert(TPair<ArxEntityId, ArxEntity*>(New.Key, Ent), Index);
				Index++;
			}
			else
			{
				RemoveList.Add(Old.Value);
				//Old.Value->Uninitialize();
				//delete Old.Value;

				OldList.RemoveAt(Index, 1, false);
			}
		}

		if (NewList.Num() <= Index)
		{
			auto Count = OldList.Num();
			for (auto i = Index; i < Count; ++i)
			{
				auto& Old = OldList[i];
				RemoveList.Add(Old.Value);
				//Old.Value->Uninitialize();
				//delete Old.Value;
			}
		}
		else if (OldList.Num() <= Index)
		{
			auto Count = NewList.Num();
			for (auto i = Index; i < Count; ++i)
			{
				auto& New = NewList[i];
				auto Ent = CreateEnt(New.Key, New.Value.PId, New.Value.Class);
				//Entities[New.Value.IId] = Ent;
				Entities.Insert(New.Value.IId, Ent);

				IdMap.Add(New.Key, New.Value.IId);
			}
		}

		for (auto Ent : RemoveList)
		{
			Ent->Uninitialize();
		}

		for (auto Ent : RemoveList)
		{
			delete Ent;
		}
	}


	for (auto Ent : Entities)
	{
		//Ent->Serialize(Serializer);
		Serializer << Ent;
	}

	Serializer << DeadList;
	//if (Serializer.IsSaving())
	//{
	//	int Count = DeadList.Num();
	//	Serializer << Count;
	//	for (int i = 0; i < Count; ++i)
	//		Serializer << DeadList[i];
	//}
	//else 
	//{
	//	int Count;
	//	Serializer << Count;
	//	DeadList.Reset();
	//	DeadList.SetNum(Count, false);
	//	for (int i = 0; i < Count; ++i)
	//		Serializer << DeadList[i];
	//}

}

void ArxWorld::ClearDeadEntities()
{
	TArray<ArxEntity*> EntList;
	for (auto Id : DeadList)
	{
		auto Ent = GetEntity(Id);

		Ent->Uninitialize();
		EntList.Add(Ent);
	}
	DeadList.Reset();

	for (auto Ent : EntList)
	{
		Entities.RemoveAt(IdMap[Ent->EntityId]);
		IdMap.Remove(Ent->EntityId);
		delete Ent;
	}
}
#pragma optimize("",on)

uint32 ArxWorld::GetHash()
{
	uint32 HashValue = 0;
	for (auto Ent: Entities)
	{
		HashValue = HashCombine(HashValue, Ent->GetHash());
	}
	return HashValue;
}

void ArxWorld::Initialize(bool bReplicated)
{
	for (auto Ent : Entities)
	{
		Ent->Initialize(bReplicated);
	}
}