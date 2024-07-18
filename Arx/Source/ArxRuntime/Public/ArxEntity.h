#pragma once 

#include "ArxCommon.h"
#include "ArxReflection.h"

class ArxWorld;
class ARXRUNTIME_API ArxEntity
{
    friend class ArxWorld;
public:
    enum EventType
    {
        ON_TIMER,
        ON_BLACKBOARD_VALUE_CHANGED,
    };


public:
    ArxEntity(ArxWorld& World, ArxEntityId Id);
    ArxEntity() = delete;
    ArxEntity(const ArxEntity&) = delete;
    virtual ~ArxEntity();


    ArxWorld& GetWorld();
    ArxEntityId GetId() const {return EntityId;}
    ArxPlayerId GetPlayerId()const{return PlayerId;}

    virtual FName GetClassName() = 0;
    virtual void Serialize(ArxSerializer& Serializer)  {};
    virtual void Initialize(bool bIsReplicated = false) {};
    virtual void Uninitialize(bool bIsReplicated = false){};
    virtual void Spawn() {};
    virtual void OnEvent(uint64 Type, uint64 Param){};
    virtual uint32 GetHash(){return 0;}

    static TMap<FName, TFunction<ArxEntity* (ArxWorld&, ArxEntityId)>>& GetFactories()
    {
        static TMap<FName, TFunction<ArxEntity* (ArxWorld&, ArxEntityId)>> Factories;
        return Factories;
    }

    static void AddFactory(FName Name, TFunction<ArxEntity* (ArxWorld&, ArxEntityId)> Factory);

private:
    void SetPlayerId(ArxPlayerId Id) {PlayerId = Id;} // called by world only

private:
    ArxWorld& WorldEntity;
    ArxEntityId EntityId = INVALID_ENTITY_ID;
    ArxPlayerId PlayerId = NON_PLAYER_CONTROL;
};


// register type

template<class T>
class ArxEntityRegister
{
public:
    using Self = T;

    static TFunction<ArxEntity* (ArxWorld&, ArxEntityId)> GetFactory()
    {
        return [](ArxWorld& InWorld, ArxEntityId Id)->ArxEntity*
        {
            auto Inst = new T(InWorld, Id);
            return Inst;
        };
    }

    static FName GetTypeName()
    {
        return TypeName;
    }


private:
    static FName Register()
    {
        auto Name = ArxTypeName<T>();
        ArxEntity::AddFactory(Name, T::GetFactory());
        return Name;
    }
    static const FName TypeName;
};

template<class T>
const FName ArxEntityRegister<T>::TypeName = ArxEntityRegister<T>::Register();

#define GENERATED_ARX_ENTITY_BODY() public: virtual FName GetClassName() override {return GetTypeName();}


/// serialize

inline ArxBasicSerializer& operator << (ArxBasicSerializer& Ser, ArxEntity& Entity)
{
    if (Ser.GetTypeName() == ArxDebugSerializer::TypeName)
    {
        ArxEntityId EntId = Entity.GetId();
        FName TypeName = Entity.GetClassName();

        ARX_SERIALIZE_MEMBER(Ser, TypeName);
        ARX_SERIALIZE_MEMBER(Ser, EntId);

    }
    Entity.Serialize(Ser);
    return Ser;
}

inline ArxBasicSerializer& operator << (ArxBasicSerializer& Ser, ArxEntity* Entity)
{
    return Ser << *Entity;
}

#define ARX_ENTITY_SERIALIZE_IMPL(Serializer) Reflection::Visit(*this, [&Serializer](auto& Val, auto Name){ARX_SERIALIZE_MEMBER_EX(Serializer, Val, ANSI_TO_TCHAR(Name));});
#define ARX_ENTITY_SERIALIZE_METHOD() virtual void Serialize(ArxSerializer& Ser) override { ARX_ENTITY_SERIALIZE_IMPL(Ser);}