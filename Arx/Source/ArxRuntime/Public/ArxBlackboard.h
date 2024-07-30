#pragma once 

#include "CoreMinimal.h"
#include "ArxCommon.h"
#include "ArxSystem.h"
#include "ArxCommandSystem.h"
#include "ArxWorld.h"

using ArxBlackboardId = uint32;

class ARXRUNTIME_API ArxBlackboard: public ArxSystem, public ArxEntityRegister<ArxBlackboard>
{
    GENERATED_ARX_ENTITY_BODY()
public:
    enum
    {
        EVENT_ON_BLACKBOARD_VALUE_CHANGED
    };


    ArxBlackboard(ArxWorld& InWorld, ArxEntityId Id);

    void Initialize(bool bIsReplicated ) override;
    void Serialize(ArxSerializer& Ser) override;

    ArxBlackboardId AutoId(uint32 Count = 1)
    {
        auto Begin = UniqueId;
        UniqueId += Count;
        return Begin;
    }

    struct InternalType
    {
        virtual FName GetTypeName()const = 0;
        virtual void Serialize(ArxSerializer& Ser) = 0;
    };

    template<class T>
    struct AnyType: public InternalType
    {
        virtual FName GetTypeName()const override
        {
            return TypeName;
        }


        void Serialize(ArxSerializer& Ser) override
        {
            Ser << Value;
        }


        T Value;

    private:
        static FName Register()
        {
            auto Name = ArxTypeName<T>();

            FactoryByName.Add(Name.ToString(), []() {
                return MakeShared<AnyType<T>>();
            });
            return Name;
        }


        static const FName TypeName;
    };


    template<class T>
    struct WriteBlackboardCommand : public ArxCommand<WriteBlackboardCommand<T>>
    {
        ArxBlackboardId Id;
        T Value;

        void Serialize(ArxSerializer& Ser) { Ser << Id << Value;}
        virtual void Execute(ArxEntity& Ent, ArxPlayerId PId) 
        {
            auto& Sys = Ent.GetWorld().GetSystem<ArxBlackboard>();
            Sys.WriteInternal(PId,Id, Value);
        }
    };

    void RegisterListener(ArxEntityId EntId, ArxBlackboardId BId);
    void UnregisterListener(ArxEntityId EntId, ArxBlackboardId BId);
    void UnregisterAllListener(ArxEntityId EntId);
public:
    template<class T>
    void Write(ArxBlackboardId Id, const T& Value)
    {
        auto& Sys = GetWorld().GetSystem<ArxCommandSystem>();
        
        WriteBlackboardCommand<T> Cmd;
        Cmd.Id = Id;
        Cmd.Value = Value;
        Sys.SendCommand(GetWorld().GetId(), MoveTemp(Cmd));
    }


    template<class T>
    const T& Read(ArxPlayerId PId, ArxBlackboardId Id)
    {
        auto Var = Containers.FindOrAdd(PId).Variables.FindRef(Id);
        
        ensure(Var && Var->GetTypeName() == ArxTypeName<T>());
        return StaticCastSharedPtr<AnyType<T>>(Var)->Value;
    }

    template<class T>
    bool Read(ArxPlayerId PId, ArxBlackboardId Id, T& Value)
    {
        auto Var = Containers.FindOrAdd(PId).Variables.FindRef(Id);
        if (Var && Var->GetTypeName() == ArxTypeName<T>())
        {
            Value = StaticCastSharedPtr<AnyType<T>>(Var)->Value;
            return true;
        }
        else
        { 
            return false;
        }

    }

    bool Has(ArxPlayerId PId, ArxBlackboardId Id)
    {
        return Containers.FindOrAdd(PId).Variables.Contains(Id);
    }

    template<class T>
    bool IsType(ArxPlayerId PId, ArxBlackboardId Id)
    {
        auto Var = Containers.FindOrAdd(PId).Variables.FindRef(Id);
        if (!Var)
            return false;

        return Var->GetTypeName() == ArxTypeName<T>();
    }

private:
    template<class T>
    void WriteInternal(ArxPlayerId PId, ArxBlackboardId Id, const T& Value)
    {
        auto Var = Get(PId, Id, [this](auto Var)->TSharedPtr<InternalType>
        {
            if (Var && Var->GetTypeName() == ArxTypeName<T>())
                return Var;
            return MakeShared<AnyType<T>>();
        });

        StaticCastSharedPtr<AnyType<T>>(Var)->Value = Value;

        auto List = Containers[PId].Listeners.Find(Id);
        auto& World = GetWorld();

        if (List)
        {
            for (auto EntId : *List)
            {
                auto Ent = World.GetEntity(EntId);
                Ent->OnEvent(GetId(), ArxEntity::EVENT_ON_BLACKBOARD_VALUE_CHANGED, Id);
            }
        }
    }

    TSharedPtr<InternalType> Get(ArxPlayerId PId, ArxBlackboardId Id,const TFunction<TSharedPtr<InternalType>(TSharedPtr<InternalType>)>& Converter)
    {
        auto& Cont = Containers.FindOrAdd(PId);
        auto Var = Cont.Variables.FindRef(Id);
        auto New = Converter(Var);
        if (Var != New)
        {
            Cont.Variables.Add(Id, New);
        }

        return New;
    }

private:
    static TMap<FString, TFunction<TSharedPtr<ArxBlackboard::InternalType>()>> FactoryByName;
    TMap<int, TFunction<TSharedPtr<ArxBlackboard::InternalType>()>> Factories;
    TSortedMap<FName, int,FDefaultAllocator, FNameFastLess> IndicesOfType;

    struct Container
    {
        TMap<ArxBlackboardId, TSharedPtr<InternalType>> Variables;
        TSortedMap<ArxBlackboardId, TArray<ArxEntityId>> Listeners;
    };

    TSortedMap<ArxPlayerId, Container> Containers;

    ArxBlackboardId UniqueId = 0;
};

template<class T>
const FName ArxBlackboard::AnyType<T>::TypeName = ArxBlackboard::AnyType<T>::Register();