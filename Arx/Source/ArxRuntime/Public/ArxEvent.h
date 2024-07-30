#pragma once 

#include "ArxCommon.h"
#include "ArxSystem.h"

// must be registered in initialize method
class ARXRUNTIME_API ArxEventReceiver
{
public:
    using FCallback = TFunction<void(uint64, uint64)>;

    void AddCallback(ArxEntity& Scheduler, uint64 Event, FCallback Cb);
    void RemoveCallback(ArxEntity& Scheduler, uint64 Event);

    void OnReceiveEvent(ArxEntityId Scheduler, uint64 Event, uint64 Param);
private:
    TSortedMap<ArxEntityId, TSortedMap<uint64, FCallback>> Events;
};

class ARXRUNTIME_API ArxEventScheduler
{
protected:
    ArxEventScheduler(ArxEntity& InEntity);
    virtual ~ArxEventScheduler();
public :
    void FireEvent( uint64 Event, uint64 Param);
    void RegisterEvent(uint64 Event, ArxEntityId EId);
    void UnregisterEvent(uint64 Event, ArxEntityId EId);


    void RegisterEvent(uint64 Event, ArxEntityId EId, ArxEventReceiver* Receiver, ArxEventReceiver::FCallback Callback);
    void UnregisterEvent(uint64 Event, ArxEntityId EId, ArxEventReceiver* Receiver);
private:
    ArxEntity& Owner;
};



class ARXRUNTIME_API ArxEventSystem: public ArxSystem, public ArxEntityRegister<ArxEventSystem>
{
    GENERATED_ARX_ENTITY_BODY()
public:
    friend class ArxEventScheduler;
public:
    using ArxSystem::ArxSystem;

    virtual void Initialize(bool bIsReplicated) override;
    virtual void Uninitialize(bool bIsReplicated) override;
    virtual void Serialize(ArxSerializer& Serializer) override;

    void FireEvent(ArxEntityId Scheduler, uint64 Event, uint64 Param);
    void RegisterEvent(ArxEntityId Scheduler, uint64 Event, ArxEntityId Receiver);
    void UnregisterEvent(ArxEntityId Scheduler, uint64 Event, ArxEntityId Receiver);
    void UnregisterEvent(ArxEntityId Scheduler, uint64 Event);
    void UnregisterEvent(ArxEntityId Scheduler);

private:
    TSortedMap<ArxEntityId, TSortedMap<uint64, TOrderedArray<ArxEntityId, true>>> EventMap;
};

#define ARX_COMPOSITE_EVENT_ID(First, Second) (First << 32 | Second)