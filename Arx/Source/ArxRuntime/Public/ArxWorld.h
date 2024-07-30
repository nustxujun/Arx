#pragma once 
#include "ArxEntity.h"
#include "ArxSystem.h"
#include "ArxServerEvent.h"

class ARXRUNTIME_API ArxWorld: public ArxEntity
{
    friend class ArxEntity;
public:
    enum
    {
        PlayerEnter,
        PlayerLeave,
    };
public:
    ArxWorld(class UWorld* InWorld);
    ~ArxWorld();
    void Update();

    class UWorld* GetUnrealWorld(){return UnrealWorld;}

    template<class T>
    ArxSystem& AddSystem()
    {
        auto Sys = CreateEntity<T>();
        SystemMap.Add(ArxTypeId<T>(), Sys->GetId());
        Systems.Add(Sys->GetId());
        return *Sys;
    }

    template<class T>
    T& GetSystem()
    {
        auto Sys = SystemMap.Find(ArxTypeId<T>());
        check(Sys);
        return GetEntity<T>(*Sys);
    }

    ArxEntity* GetEntity(ArxEntityId EntId)
    {
        if (EntId == 0)
            return this;
        auto Ret = IdMap.Find(EntId);
        check(Ret)
        return Entities[*Ret];
    }

    template<class T>
    T& GetEntity(ArxEntityId EntId)
    {
        auto Ret = IdMap.Find(EntId);
        check(Ret);
        auto Ent = Entities[*Ret];
        check(Ent->GetClassName() == ArxTypeName<T>());
        return *static_cast<T*>(Ent);
    }

    template<class T>
    T* CreateEntity(ArxPlayerId PlayerId = NON_PLAYER_CONTROL)
    {
        return static_cast<T*>(CreateEntity(PlayerId,T::GetTypeName()));
    }

    ArxEntity* CreateEntity(ArxPlayerId PlayerId , FName TypeName);
    void DestroyEntity(ArxEntityId Id);
    bool IsEntityValid(ArxEntityId Id);


    void Serialize(ArxSerializer& Serializer) override;
    void Initialize(bool bReplicated) override;

    FName GetClassName(){
        static FName ClassName = TEXT("<class ArxWorld>");
        return ClassName;
    }

    inline bool IsDeterministic()const { return bDeterministic; }
    inline bool IsInitializingOrUninitalizing()const{return bInitializing;}
    void Setup(const TFunction<void(ArxWorld&)>& Callback);

    void RegisterServerEvent(ArxServerEvent::Event Event, ArxEntityId Id);
    void UnregisterServerEvent(ArxServerEvent::Event Event, ArxEntityId Id);
private:
    void AddInternalSystem();
    void ClearDeadEntities();
private:
    class UWorld* UnrealWorld;
    TArray<ArxEntityId> Systems; 
    TSortedMap<uint64_t, ArxEntityId> SystemMap;
    TSparseArray<ArxEntity*> Entities;
    TMap<ArxEntityId, uint32> IdMap;
    TArray<ArxEntityId> DeadList;
    ArxEntityId UniqueId = 1;
    bool bDeterministic = false;
    bool bInitializing = false;
public:

};


