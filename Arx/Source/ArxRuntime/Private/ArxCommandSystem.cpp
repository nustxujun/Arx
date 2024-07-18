#include "ArxCommandSystem.h"
#include "ArxWorld.h"

    
TMap<FName, uint32> ArxCommandSystem::IndexOfCommands;
TArray<TFunction<TFunction<void(ArxEntity&, ArxPlayerId)>(ArxSerializer&)>> ArxCommandSystem::Executers;
TArray<TFunction<void(ArxSerializer&)>> ArxCommandSystem::SerializersForDebug;

void ArxCommandSystem::Initialize(bool bReplicated)
{
    TArray<FName> Commands;
    for (auto& Item : IndexOfCommands)
    {
        Commands.Add(Item.Key);
    }

    Commands.Sort([](auto& a, auto& b){
        return a.ToString() < b.ToString();
    });
    
    CommandMap.Reset();

    for (int i = 0; i < Commands.Num(); ++i)
    {
        CommandMap.Add(Commands[i],i);
    }
}

void ArxCommandSystem::Update() 
{
    if (!ReceivedCommands)
        return;
    ArxReader Serializer(*ReceivedCommands);

    TArray<TFunction<void()>> FuncList;
    while (!Serializer.AtEnd())
    {
        ArxPlayerId PlyId;
        int Num;
        
        Serializer << PlyId;
        Serializer << Num;

        for (int j = 0; j < Num; ++j)
        {
            ArxEntityId EntId;
            int CmdIdx;
            Serializer << EntId;
            Serializer << CmdIdx;

            auto Name = CommandMap.FindBackward(CmdIdx);
            check(Name);
            uint32 ExtIdx = IndexOfCommands[*Name];

            check(Executers.IsValidIndex(ExtIdx));

            FuncList.Add([this, EntId, PlyId, Func = std::move(Executers[ExtIdx](Serializer))]() {
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

FString ArxCommandSystem::DumpCommands(const TArray<uint8>& Commands)
{
    ArxReader Serializer(Commands);
    FString Content;

    while (!Serializer.AtEnd())
    {
        ArxPlayerId PlyId;
        int Num;

        Serializer << PlyId;
        Serializer << Num;

        Content += FString::Printf(TEXT("\n[ playerid: %d, count: %d ]\n"), PlyId, Num);

        for (int j = 0; j < Num; ++j)
        {
            ArxEntityId EntId;
            int CmdIdx;
            Serializer << EntId;
            Serializer << CmdIdx;

            auto Name = CommandMap.FindBackward(CmdIdx);
            check(Name);
            uint32 ExtIdx = IndexOfCommands[*Name];

            check(Executers.IsValidIndex(ExtIdx));

            SerializersForDebug[ExtIdx](Serializer);

            TArray<uint8> CmdData;
            ArxDebugSerializer Writer(CmdData);
            SerializersForDebug[ExtIdx](Writer);


            Content += FString::Printf(TEXT("\n[ %s ]\nEntId: %d\n"), *Name->ToString(), EntId);
            Content.AppendChars((TCHAR*)CmdData.GetData(), CmdData.Num() / sizeof(TCHAR));
        }
    }

    return Content;
}