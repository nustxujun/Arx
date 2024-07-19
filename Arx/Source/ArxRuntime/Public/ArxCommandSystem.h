#pragma once 

#include "ArxCommon.h"
#include "ArxSystem.h"

class ARXRUNTIME_API ArxCommandSystem: public ArxSystem, public ArxEntityRegister<ArxCommandSystem>
{
    GENERATED_ARX_ENTITY_BODY()
public:
    using ArxSystem::ArxSystem;

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

        SerializersForDebug.Add([Cmd = T{}](ArxSerializer& Serializer) mutable{
            Cmd.Serialize(Serializer);
        });
    }

    template<class T>
    void SendCommand(ArxEntityId EntId, T Command)
    {
        CachedCommands.Add([this, Command = MoveTemp(Command), EntId](ArxSerializer& Serializer)mutable
        {
            auto IndexPtr = CommandMap.FindForward(T::TypeName);
            check(IndexPtr);
            Serializer << EntId;
            Serializer << *IndexPtr;
            Command.Serialize(Serializer);
        });
    }

    void SendAllCommands(ArxSerializer& Serializer);
    void ReceiveCommands( const TArray<uint8>* Data) { ReceivedCommands  = Data;}

    virtual void Initialize(bool bReplicated) override;
    virtual void Update() override;

    FString DumpCommands(const TArray<uint8>& Commands);
private:
    static TMap<FName, uint32> IndexOfCommands;
    static TArray<TFunction<TFunction<void(ArxEntity&, ArxPlayerId)>(ArxSerializer&)>> Executers;
    static TArray<TFunction<void(ArxSerializer&)>> SerializersForDebug;
    TDoubleMap<FName, int> CommandMap;
    TArray<TFunction<void(ArxSerializer&)>> CachedCommands;
    const TArray<uint8>* ReceivedCommands = nullptr;
};


template<class T>
class ArxCommand
{
public:
    virtual ~ArxCommand(){};
    virtual void Serialize(ArxSerializer&) = 0;
    virtual void Execute(ArxEntity&, ArxPlayerId) = 0;

    const static FName TypeName;

private:
    static FName Register()
    {
        ArxCommandSystem::RegisterCommand<T>();
        return ArxTypeName<T>();
    }
};

template<class T>
const FName ArxCommand<T>::TypeName = ArxCommand<T>::Register();


/**
    Header : 

    EXPOSED_ENTITY_METHOD(MethodName, int Param1, float Param2);


    Source:
    void MethodName_Internal(ArxPlayerId PId, int Param1, float Param2)
    {
        // do something 
    }

*/

#define EXPOSED_ENTITY_METHOD(Name, ...)\
    template<class ... Args> struct Name##_Command: public ArxCommand<Name##_Command<Args ...>>{\
        std::tuple<Args...> Members;\
        template<class ... ParameterType> friend void __##Name##Serialize_Internal(ArxSerializer& Ser,ParameterType& ... Params){\
            int a[] = {(Ser << Params,0) ...};\
        }\
        friend void __##Name##Serialize_Internal(ArxSerializer& Ser) {}\
        template<class Tuple,std::size_t ... Index> void Serialize(Tuple& InTuple, ArxSerializer& Ser,std::index_sequence<Index...>const) const{\
            __##Name##Serialize_Internal(Ser, std::get<Index>(InTuple) ...); \
        }\
        void Serialize(ArxSerializer& Ser) {\
            Serialize(Members,Ser, std::make_index_sequence < std::tuple_size<decltype(Members)>{} > {});\
        }\
        template<class ... ParameterType> void Execute(Self& CurEnt, ArxPlayerId PId, const ParameterType& ... Params)const{\
            CurEnt.Name##_Internal(PId, Params...);\
        }\
        template<class Tuple, std::size_t ... Index> void Execute(Tuple& InTuple, Self& CurEnt,ArxPlayerId PId, std::index_sequence<Index...>const) const{\
            Execute(CurEnt, PId, std::get<Index>(InTuple) ...); \
        }\
        void Execute(ArxEntity& Ent, ArxPlayerId PId){\
            check(Ent.GetClassName() == Self::GetTypeName())\
            auto& CurEnt = static_cast<Self&>(Ent);\
            Execute(Members, CurEnt, PId, std::make_index_sequence < std::tuple_size<decltype(Members)>{} > {});\
        }\
    };\
    template<class ... Args> void Name(const Args& ... args){\
        Name##_Command<Args...> Cmd;\
        Cmd.Members = std::make_tuple(args ...);\
        GetWorld().GetSystem<ArxCommandSystem>().SendCommand(GetId(), MoveTemp(Cmd));\
    }\
    void Name##_Internal(ArxPlayerId PId, __VA_ARGS__);


