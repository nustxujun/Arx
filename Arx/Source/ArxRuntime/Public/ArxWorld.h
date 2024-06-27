#pragma once 
#include "ArxEntity.h"
#include "ArxSystem.h"

class ARXRUNTIME_API ArxWorld: public ArxEntity
{
    friend class ArxEntity;

public:
    ArxWorld(class UWorld* InWorld);
    ~ArxWorld();
    void Update();

    class UWorld* GetUnrealWorld(){return UnrealWorld;}

    template<class T>
    ArxSystem& AddSystem()
    {
        auto Sys = CreateEntity<T>();
        SystemMap.Add(ArxTypeId<T>(), Sys);
        Systems.Add(Sys);
        return *Sys;
    }

    template<class T>
    T& GetSystem()
    {
        auto Sys = SystemMap.FindRef(ArxTypeId<T>());
        check(Sys);
        return *(T*)Sys;
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
    T* CreateEntity(ArxPlayerId PlayerId = NON_PLAYER_CONTROL)
    {
        return static_cast<T*>(CreateEntity(PlayerId,T::GetTypeName()));
    }

    ArxEntity* CreateEntity(ArxPlayerId PlayerId , FName TypeName);
    void DestroyEntity(ArxEntityId Id);
    bool IsEntityValid(ArxEntityId Id);


    void Serialize(ArxSerializer& Serializer) override;
    uint32 GetHash() override;
    void Initialize(bool bReplicated) override;

    FName GetClassName(){
        static FName ClassName = TEXT("<class ArxWorld>");
        return ClassName;
    }

    inline bool IsDeterministic()const { return bDeterministic; }
    void Setup(const TFunction<void(ArxWorld&)>& Callback);

private:
    void AddInternalSystem();
    void ClearDeadEntities();
private:
    class UWorld* UnrealWorld;
    TArray<ArxSystem*> Systems; 
    TMap<uint64_t, ArxSystem*> SystemMap;
    TSparseArray<ArxEntity*> Entities;
    TMap<ArxEntityId, uint32> IdMap;
    TArray<ArxEntityId> DeadList;
    ArxEntityId UniqueId = 1;
    bool bDeterministic = false;

public:

};


