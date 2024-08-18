#include "ArxCommandSystem.h"
#include "ArxWorld.h"
    
TMap<FName, uint32> ArxCommandSystem::IndexOfCommands;
TArray<TSharedPtr<ArxBasicCommand>> ArxCommandSystem::Executers;
TArray<TFunction<void(ArxSerializer&)>> ArxCommandSystem::SerializersForDebug;


static constexpr int ReservedCommandCount = 1;
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
        CommandMap.Add(Commands[i],i + ReservedCommandCount);
    }
}

void ArxCommandSystem::Update(int FrameId)
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

        if (PlyId == NON_PLAYER_CONTROL)
        {
            for (int j = 0; j < Num; ++j)
            {
                ArxServerCommand Cmd;
                Cmd.Serialize(Serializer);
                FuncList.Add([this, Cmd = MoveTemp(Cmd)]() {
                    Cmd.Execute(GetWorld());
                });
            }
        }
        else
        {
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
                auto Cmd = Executers[ExtIdx];
                Cmd->Serialize(Serializer);
                FuncList.Add([this, EntId, PId = PlyId, Cmd ]() {
                    auto& Ent = *GetWorld().GetEntity(EntId);
                    checkf(Ent.GetPlayerId() == PId, 
                        TEXT("can not command an entity(type:%s ,id: %d, pid:%d) owned by other player(pid: %d)"), *Ent.GetClassName().ToString(), Ent.GetId(), Ent.GetPlayerId(), PId);
                    if (Ent.GetPlayerId() != PId) 
                        return; 
                    Cmd->Execute(Ent, PId);
                });
            
            }
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
    {
        FScopeLock Lock(&Mutex);
        CachedCommands.Append(MoveTemp(CachedCommandsFromThread));
    }
    int Num = CachedCommands.Num();
    if (Num != 0)
    {
        Serializer << Num;
        for (auto& Com: CachedCommands)
        {
            Com(Serializer);
        }
        CachedCommands.Reset();
    }


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

        if (PlyId == NON_PLAYER_CONTROL)
        {
            for (int j = 0; j < Num; ++j)
            {
                //ArxEntityId EntId;
                //int CmdIdx;
                //Serializer << EntId;
                //Serializer << CmdIdx;

                ArxServerCommand Cmd;
                Cmd.Serialize(Serializer);

                Content += FString::Printf(TEXT("\n[server command]\nEvent: %d, PlayerId: %d\n"), Cmd.Event, Cmd.PlayerId);
            }
        }
        else
        {
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
    }

    return Content;
}
