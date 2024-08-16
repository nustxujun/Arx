#include "ArxWorld.h"
#include "ArxCommandSystem.h"
#include "ArxTimerSystem.h"
#include "ArxBlackboard.h"
#include "ArxEvent.h"

#include "CoreMinimal.h"

struct ScopedBoolean
{
	bool& Value;
	bool OldValue;
	ScopedBoolean(bool& Val):Value(Val){ OldValue = Value; Value = true; };
	~ScopedBoolean() { Value = OldValue; }
};

#define SCOPED_BOOLEAN(BOOLVAL) ScopedBoolean __SB(BOOLVAL)


ArxWorld::ArxWorld(UWorld* InWorld)
    :ArxEntity(*this, 0), Accessor(*this),UnrealWorld(InWorld)
{
	Entities.Reserve(10000);
}

ArxWorld::~ArxWorld()
{
	TArray<ArxEntity*> List;
	for (auto Ent : Entities)
	{
		List.Add(Ent);
	}

	List.Sort([](const ArxEntity& a,const ArxEntity& b){
		return a.GetId() > b.GetId();
	});

	{
		SCOPED_BOOLEAN(bInitializing);
		for (auto Ent : List)
		{
			Ent->Uninitialize();
		}
	}

	for (auto Ent : List)
	{
		delete Ent;
	}
}

bool ArxWorld::IsPrepared()
{
	return Accessor.Release();
}

void ArxWorld::Setup(const TFunction<void(ArxWorld&)>& Callback)
{
	SCOPED_BOOLEAN(bDeterministic);
	AddInternalSystem();
	Callback(*this);
}

void ArxWorld::RegisterServerEvent(ArxServerEvent::Event Event, ArxEntityId Id)
{
	GetSystem<ArxEventSystem>().RegisterEvent(0,Event,Id);
}

void ArxWorld::UnregisterServerEvent(ArxServerEvent::Event Event, ArxEntityId Id)
{
	GetSystem<ArxEventSystem>().UnregisterEvent(0, Event, Id);
}

void ArxWorld::RequestAccessInGameThread(TFunction<void(const ArxWorld&)>&& Func)
{
	Accessor.Request(MoveTemp(Func));
}



void ArxWorld::AddInternalSystem()
{
	AddSystem<ArxEventSystem>();
	AddSystem<ArxCommandSystem>();
	AddSystem<ArxBlackboard>();
	AddSystem<ArxTimerSystem>();
}

ArxEntity* ArxWorld::CreateEntity(ArxPlayerId PId , FName TypeName)
{
	CHECK_DETER();

	auto Fac = ArxEntity::GetFactories().Find(TypeName);
	check(Fac);

	ArxEntityId Id = UniqueId++;
	auto Ent = (*Fac)(*this, Id);
	Ent->SetPlayerId(PId);
	IdMap.Add(Id, Entities.Add(Ent));

	SCOPED_BOOLEAN(bInitializing);
	Ent->Initialize(false);

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

void ArxWorld::Update(int FrameId)
{
	checkf(Accessor.Release(), TEXT("need to prepare world before updating"));

	SCOPED_BOOLEAN(bDeterministic);
	for (auto Id : Systems)
	{
		auto Sys = GetEntity(Id);
		static_cast<ArxSystem*>(Sys)->PreUpdate(FrameId);
	}

	for (auto& Id : Systems)
	{
		auto Sys = GetEntity(Id);
		static_cast<ArxSystem*>(Sys)->Update(FrameId);
	}

	for (auto& Id : Systems)
	{
		auto Sys = GetEntity(Id);
		static_cast<ArxSystem*>(Sys)->PostUpdate(FrameId);
	}


	ClearDeadEntities();

	Accessor.Acquire();
}

struct ClassInfo
{
	FName Class;
	uint32 IId;
	ArxPlayerId PId;

	friend inline ArxBasicSerializer& operator <<(ArxBasicSerializer& Serializer, ClassInfo& CI)
	{
		//ARX_SERIALIZE_MEMBER_FAST(CI.Class);
		ARX_SERIALIZE_MEMBER_FAST(CI.IId);
		//ARX_SERIALIZE_MEMBER_FAST(CI.PId);

		return Serializer;
	}

	friend inline FString LexToString(const ClassInfo& CI)
	{
		return FString::Printf(TEXT("{%s, %d, %d}"), *CI.Class.ToString(), CI.IId, CI.PId);
	}
};

void ArxWorld::Serialize(ArxSerializer& Serializer)
{
	ARX_SERIALIZE_MEMBER_FAST(UniqueId);
	ARX_SERIALIZE_MEMBER_FAST(Systems);
	ARX_SERIALIZE_MEMBER_FAST(SystemMap);

	if (Serializer.IsSaving())
	{
		int EntCount = IdMap.Num();
		ARX_SERIALIZE_MEMBER_FAST(EntCount);
		int Space = Entities.GetMaxIndex();
		ARX_SERIALIZE_MEMBER_FAST(Space);
		for (auto& Item : IdMap)
		{
			//ARX_SERIALIZE_MEMBER_FAST(Item)
			Serializer << Item;

			auto Ent = Entities[Item.Value];
			auto TypeName = Ent->GetClassName();
			Serializer << TypeName;
			//ARX_SERIALIZE_MEMBER_FAST(TypeName);
			auto PId = Ent->GetPlayerId();
			Serializer << PId;
			//ARX_SERIALIZE_MEMBER_FAST(PId);
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
			checkf(Fac, TEXT("Can not find entity type : %s"), *Class.ToString());

			auto Ent = (*Fac)(*this, EId);
			Ent->SetPlayerId(PId);

			SCOPED_BOOLEAN(bInitializing);
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
			}
		}
		else if (OldList.Num() <= Index)
		{
			auto Count = NewList.Num();
			for (auto i = Index; i < Count; ++i)
			{
				auto& New = NewList[i];
				auto Ent = CreateEnt(New.Key, New.Value.PId, New.Value.Class);
				Entities.Insert(New.Value.IId, Ent);

				IdMap.Add(New.Key, New.Value.IId);
			}
		}

		{
			SCOPED_BOOLEAN(bInitializing);
			for (auto Ent : RemoveList)
			{
				Ent->Uninitialize(true);
			}
		}

		for (auto Ent : RemoveList)
		{
			delete Ent;
		}
	}

	const bool bIsSaving = Serializer.IsSaving();
	for (auto Ent : Entities)
	{
		Serializer << Ent;
		if (!bIsSaving)
			Ent->Spawn();
	}

	Serializer << DeadList;

}

void ArxWorld::ClearDeadEntities()
{
	TArray<ArxEntity*> EntList;
	{
		SCOPED_BOOLEAN(bInitializing);

		for (auto Id : DeadList)
		{
			auto Ent = GetEntity(Id);

			Ent->Uninitialize();
			EntList.Add(Ent);
		}
	}
	DeadList.Reset();

	for (auto Ent : EntList)
	{
		Entities.RemoveAt(IdMap[Ent->EntityId]);
		IdMap.Remove(Ent->EntityId);
		delete Ent;
	}
}


void ArxWorld::Initialize(bool bReplicated)
{
	SCOPED_BOOLEAN(bInitializing);
	for (auto Ent : Entities)
	{
		Ent->Initialize(bReplicated);
	}

}


ArxWorld::FAccessor::FAccessor(ArxWorld& InWorld) :
	World(InWorld), bAccessable(false)
{
}


void ArxWorld::FAccessor::Tick(float DeltaTime)
{
	if (!bAccessable.load())
		return;

	check(IsInGameThread())
	TArray<TFunction<void(const ArxWorld&)>> List;
	{
		FScopeLock Lock(&Mutex);
		Swap(List, Callbacks);
	}

	for (auto& Callback : List)
	{
		Callback(World);
	}


	bAccessable.store(false);
}

void ArxWorld::FAccessor::Request(TFunction<void(const ArxWorld&)>&& Func)
{
	if (IsInGameThread())
	{
		Func(World);
	}
	else
	{
		FScopeLock Lock(&Mutex);
		Callbacks.Add(MoveTemp(Func));
	}
}

bool ArxWorld::FAccessor::Release()
{
	return IsInGameThread() || bAccessable.load() == false;
}
void ArxWorld::FAccessor::Acquire()
{
	bAccessable.store(true);
}
