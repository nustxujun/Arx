#include "ArxBlackboard.h"
#include "ArxSerializer.h"
#include "ArxWorld.h"


TMap<FString, TFunction<TSharedPtr<ArxBlackboard::InternalType>()>> ArxBlackboard::FactoryByName;





ArxBlackboard::ArxBlackboard(ArxWorld& InWorld, ArxEntityId Id) :
	ArxSystem(InWorld, Id)
{

}


void ArxBlackboard::Initialize(bool bIsReplicated)
{
	for (auto& Item : FactoryByName)
	{
		Factories.Add(Factories.Num(), Item.Value);
		IndicesOfType.Add( FName(*Item.Key), IndicesOfType.Num());
	}
}

void ArxBlackboard::Serialize(ArxSerializer& Ser)
{

	if (Ser.IsSaving())
	{
#if UE_BUILD_DEVELOPMENT
		{
			int Count = IndicesOfType.Num();
			Ser << Count;
			for (auto& Item : IndicesOfType)
			{
				Ser << Item;
			}
		}
#endif
		int Count = Containers.Num();
		Ser << Count;
		
		for (auto& C: Containers)
		{
			Ser << C.Key;
			{
				int Num = C.Value.Variables.Num();
				Ser << Num;
				for (auto& Item : C.Value.Variables)
				{
					Ser << Item.Key;

					auto Type = Item.Value->GetTypeName();
					auto Index = IndicesOfType[Type];
					Ser << Index;
					Item.Value->Serialize(Ser);
				}
			}

			Ser << C.Value.Listeners;
		}
	}
	else
	{
#if UE_BUILD_DEVELOPMENT
		{
			int Count;
			Ser << Count;

			for (int i = 0; i < Count;++i)
			{
				FName Name;
				int Index;
				Ser << Name << Index;
				check(IndicesOfType.Contains(Name) && IndicesOfType[Name] == Index);
			}
		}
#endif
		Containers.Reset();
		int Count = 0;
		Ser << Count;
		for (int i = 0; i < Count; ++i)
		{
			ArxPlayerId PId;
			Ser << PId;
			Containers.Add(PId);
			int Num;
			Ser << Num;
			auto& Variables = Containers[PId].Variables;
			for (int j = 0; j < Num; ++j)
			{
				ArxBlackboardId BId;
				Ser << BId;
				int Index;
				Ser << Index;
				auto Var = Factories[Index]();
				Var->Serialize(Ser);
				Variables.Add(BId, Var);
			}


			Ser << Containers[PId].Listeners;
		}

	}
}

void ArxBlackboard::RegisterListener(ArxEntityId EntId, ArxBlackboardId BId)
{
	Containers.FindOrAdd(GetWorld().GetEntity(EntId)->GetPlayerId()).Listeners.FindOrAdd(BId).AddUnique(EntId);
}

void ArxBlackboard::UnregisterListener(ArxEntityId EntId, ArxBlackboardId BId)
{
	Containers.FindOrAdd(GetWorld().GetEntity(EntId)->GetPlayerId()).Listeners.FindOrAdd(BId).RemoveSingle(EntId);
}

void ArxBlackboard::UnregisterAllListener(ArxEntityId EntId)
{
	for (auto& L : Containers.FindOrAdd(GetWorld().GetEntity(EntId)->GetPlayerId()).Listeners)
	{
		L.Value.RemoveSingle(EntId);
	}
}