#pragma once 

#include "ArxCommon.h"
#include "ArxSystem.h"

class ARXRUNTIME_API ArxCommandSystem: public ArxBasicSystem<ArxCommandSystem>
{
public:
    using ArxBasicSystem::ArxBasicSystem;

    template<class T>
    static void RegisterCommand()
    {
        IndexOfCommands.Add(ArxTypeName<T>(), Executers.Num() );
        Executers.Add([]( ArxSerializer& Serializer)
        {
            T Com;
            Com.Serialize(Serializer);

            return [Com = MoveTemp(Com)](ArxEntity& Ent, ArxPlayerId Id)mutable{
                Com.Execute(Ent, Id);
            };
        });
    }

    template<class T>
    void SendCommand(ArxEntityId EntId, T Command)
    {
        CachedCommands.Add([this, Command = MoveTemp(Command), EntId](ArxSerializer& Serializer)mutable
        {
            Serializer << EntId;
            auto Index = IndexOfCommands[T::TypeName];
            Serializer << Index;
            Command.Serialize(Serializer);
        });
    }

    void SendAllCommands(ArxSerializer& Serializer);
    void ReceiveCommands(int Num, const TArray<uint8>* Data) { NumReceivedCmds = Num;ReceivedCommands  = Data;}

    virtual void Update() override;

    

private:
    static TMap<FName, uint32_t> IndexOfCommands;
    static TArray<TFunction<TFunction<void(ArxEntity&, ArxPlayerId)>(ArxSerializer&)>> Executers;
    TArray<TFunction<void(ArxSerializer&)>> CachedCommands;
    const TArray<uint8>* ReceivedCommands = nullptr;
    int NumReceivedCmds;
};


template<class T>
class ArxCommand
{
public:
    virtual ~ArxCommand(){};
    virtual void Serialize(ArxSerializer&) = 0;
    virtual void Execute(ArxEntity&, ArxPlayerId) = 0;

    const static FName TypeName;
};

template<class T>
const FName ArxCommand<T>::TypeName = [](){
    ArxCommandSystem::RegisterCommand<T>();
    return ArxTypeName<T>();
}();