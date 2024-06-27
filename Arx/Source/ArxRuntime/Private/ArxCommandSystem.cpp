#include "ArxCommandSystem.h"
#include "ArxWorld.h"

    
TMap<FName, uint32_t> ArxCommandSystem::IndexOfCommands;
TArray<TFunction<TFunction<void(ArxEntity&, ArxPlayerId)>(ArxSerializer&)>> ArxCommandSystem::Executers;

void ArxCommandSystem::Update() 
{

    if (!ReceivedCommands)
        return;
    ArxReader Serializer(*ReceivedCommands);
    int GroupNum  = NumReceivedCmds;

    TArray<TFunction<void()>> FuncList;
    for (int i = 0; i < GroupNum; ++i)
    {
        ArxPlayerId PlyId;
        int Num;
        
        Serializer << PlyId;
        Serializer << Num;

        for (int j = 0; j < Num; ++j)
        {
            ArxEntityId EntId;
            uint32_t CmdIdx;

            Serializer << EntId;
            Serializer << CmdIdx;

            FuncList.Add([this, EntId, PlyId, Func = std::move(Executers[CmdIdx](Serializer))]() {
                auto Ent = GetWorld().GetEntity(EntId);
                Func(*Ent, PlyId);
            });
        }

 
    }

    ReceivedCommands = nullptr;

    for (auto& Func : FuncList)
    {
        Func();
    }
}

void ArxCommandSystem::SendAllCommands(ArxSerializer& Serializer)
{
    int Num = CachedCommands.Num();
    if (Num == 0)
        return;
    Serializer << Num;
    for (auto& Com: CachedCommands)
    {
        Com(Serializer);
    }
    CachedCommands.Reset();
}
