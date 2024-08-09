#include "ArxTimerSystem.h"
#include "ArxWorld.h"



const auto CompairTimer = [](const auto& a, const auto& b) {return a.Key < b.Key;};

int ArxTimerSystem::AddTimer(ArxEntityId InEntityId, ArxTimeDuration Delay, ArxTimeDuration Interval , ArxTimeDuration Duration)
{
    FTimer Timer = {Interval, Duration == 0 ? 0 : TotalTime + Duration , InEntityId,++TotalTimerCount};
    if (Delay != 0)
    {
        DelayedTimers.HeapPush(TPair<ArxTimePoint, FTimer>( TotalTime + Delay,Timer), CompairTimer);
    }
    else
    {
        RepeatedTimers.FindOrAdd(Interval).Timers.Add(Timer.Id, (Timer));
    }

    TimerMap.Add(Timer.Id, Duration);
    return Timer.Id;
}

void ArxTimerSystem::RemoveTimer(int Id)
{
    if (TimerMap.Contains(Id) && !RemoveList.Contains(Id))
        RemoveList.Add(Id);
}

void ArxTimerSystem::Update()
{
    TotalTime += ArxConstants::TimeStep;
    while (DelayedTimers.Num() > 0)
    {
        auto& Timer = DelayedTimers.HeapTop();
        if (TotalTime < Timer.Key)
            break;

        if (Timer.Value.IsRepeated())
        {
            RepeatedTimers.FindOrAdd(Timer.Value.Interval).Timers.Add(Timer.Value.Id, MoveTemp(Timer.Value));
        }
        else
        {
            GetWorld().GetEntity(Timer.Value.EntityId)->OnEvent(GetId(), EVENT_ON_TIMER, Timer.Value.Id);
        }
        DelayedTimers.HeapPopDiscard(CompairTimer, false);
    }

    for (auto& List : RepeatedTimers)
    {
        if (TotalTime < List.Value.Begin)
            continue;

        List.Value.Begin = TotalTime + List.Key;
        for (auto& Item : List.Value.Timers)
        {
            auto& Timer = Item.Value;
            //Timer.Callback(Item.Key);
            GetWorld().GetEntity(Timer.EntityId)->OnEvent(GetId(), EVENT_ON_TIMER, Timer.Id);
            if (!Timer.IsInfinity() && Timer.End >= List.Value.Begin)
            {
                RemoveList.Add(Timer.Id);
            }
        }
    }

    for (auto& Id : RemoveList)
    {
        bool bSkip = false;

        auto Dur = TimerMap[Id];
        TimerMap.Remove(Id);

        const auto Count = DelayedTimers.Num();
        for (int i = 0; i < Count; ++i)
        {
            auto& Timer = DelayedTimers[i];
            if (Timer.Value.Id != Id)
                continue;

            DelayedTimers.HeapRemoveAt(i, CompairTimer,false);
            bSkip = true;
            break;
        }

        if (bSkip || Dur == 0)
            continue;

        auto& List = RepeatedTimers[Dur];
        List.Timers.Remove(Id);
        if (List.Timers.Num() == 0)
        {
            RepeatedTimers.Remove(Dur);
        }
    }
    RemoveList.Reset();
}

void ArxTimerSystem::Serialize(ArxSerializer& Serializer)
{
    ARX_SERIALIZE_MEMBER_FAST(TotalTimerCount)
    ARX_SERIALIZE_MEMBER_FAST(TotalTime)
    ARX_SERIALIZE_MEMBER_FAST(RepeatedTimers);
    ARX_SERIALIZE_MEMBER_FAST(DelayedTimers);
    ARX_SERIALIZE_MEMBER_FAST(RemoveList);
    ARX_SERIALIZE_MEMBER_FAST(TimerMap);
}