#include "ArxEvent.h"
#include "ArxWorld.h"
ArxEventScheduler::ArxEventScheduler(ArxEntity& InEntity)
	:Owner(InEntity)
{
}

ArxEventScheduler::~ArxEventScheduler()
{
	Owner.GetWorld().GetSystem<ArxEventSystem>().UnregisterEvent(Owner.GetId());
}

void ArxEventScheduler::FireEvent(uint64 Event, uint64 Param)
{
	Owner.GetWorld().GetSystem<ArxEventSystem>().FireEvent(Owner.GetId(),Event, Param);
}

void ArxEventScheduler::RegisterEvent(uint64 Id, ArxEntityId EId)
{
	Owner.GetWorld().GetSystem<ArxEventSystem>().RegisterEvent(Owner.GetId(), Id, EId);
}

void ArxEventScheduler::RegisterEvent(uint64 Event, ArxEntityId EId, ArxEventReceiver* Receiver, ArxEventReceiver::FCallback Callback)
{
	Owner.GetWorld().GetSystem<ArxEventSystem>().RegisterEvent(Owner.GetId(), Event, EId);
	Receiver->AddCallback(Owner,Event, MoveTemp(Callback));
}

void ArxEventScheduler::UnregisterEvent(uint64 Id, ArxEntityId EId)
{
	Owner.GetWorld().GetSystem<ArxEventSystem>().UnregisterEvent(Owner.GetId(), Id, EId);
}

void ArxEventScheduler::UnregisterEvent(uint64 Event, ArxEntityId EId, ArxEventReceiver* Receiver)
{
	Owner.GetWorld().GetSystem<ArxEventSystem>().UnregisterEvent(Owner.GetId(), Event, EId);
	Receiver->RemoveCallback(Owner, Event);
}


void ArxEventSystem::Initialize(bool bIsReplicated)
{
}

void ArxEventSystem::Uninitialize(bool bIsReplicated)
{
}

void ArxEventSystem::Serialize(ArxSerializer& Serializer)
{
	ARX_SERIALIZE_MEMBER_FAST(EventMap);
}

void ArxEventSystem::FireEvent(ArxEntityId Scheduler, uint64 Event, uint64 Param)
{
	auto& Map = EventMap.FindOrAdd(Scheduler);
	auto Vec = Map.Find(Event);
	if (!Vec)
		return;

	auto& World = GetWorld();
	for (auto EId : Vec->GetArray())
	{
		World.GetEntity(EId)->OnEvent(Scheduler,Event, Param);
	}
}

void ArxEventSystem::RegisterEvent(ArxEntityId Scheduler, uint64 Event, ArxEntityId Receiver)
{
	auto& Map = EventMap.FindOrAdd(Scheduler);
	auto& Vec = Map.FindOrAdd(Event);
	Vec.Insert(Receiver);
}

void ArxEventSystem::UnregisterEvent(ArxEntityId Scheduler, uint64 Event, ArxEntityId Receiver)
{
	auto& Map = EventMap.FindOrAdd(Scheduler);
	auto& Vec = Map.FindOrAdd(Event);
	Vec.Remove(Receiver);
}

void ArxEventSystem::UnregisterEvent(ArxEntityId Scheduler, uint64 Event)
{
	auto& Map = EventMap.FindOrAdd(Scheduler);
	Map.Remove(Event);
}

void ArxEventSystem::UnregisterEvent(ArxEntityId Scheduler)
{
	EventMap.Remove(Scheduler);
}

void ArxEventReceiver::AddCallback(ArxEntity& Scheduler , uint64 Event, FCallback Cb)
{
	check(Scheduler.GetWorld().IsInitializingOrUninitalizing());
	Events.FindOrAdd(Scheduler.GetId()).Add(Event,Cb);
}

void ArxEventReceiver::RemoveCallback(ArxEntity& Scheduler , uint64 Event)
{
	check(Scheduler.GetWorld().IsInitializingOrUninitalizing());
	Events.FindOrAdd(Scheduler.GetId()).Remove(Event);
}

void ArxEventReceiver::OnReceiveEvent(ArxEntityId Scheduler, uint64 Event, uint64 Param)
{
	auto CB = Events.FindOrAdd(Scheduler).Find(Event);
	if (CB)
	{
		(*CB)(Event, Param);
	}
}
