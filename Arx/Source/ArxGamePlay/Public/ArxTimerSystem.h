#pragma once 

#include "ArxGamePlayCommon.h"
#include "ArxSystem.h"
#include "ArxEvent.h"

#define INVALID_TIMER 0

class ARXGAMEPLAY_API ArxTimerSystem: public ArxSystem, public ArxEntityRegister<ArxTimerSystem>
{
    GENERATED_ARX_ENTITY_BODY()
public:
    
    enum Event
    {
        EVENT_ON_TIMER
    };

    using ArxSystem::ArxSystem;
    virtual void Update() override;

    int AddTimer(ArxEntityId, ArxTimeDuration Delay, ArxTimeDuration Interval = 0, ArxTimeDuration Duration = 0);
    void Serialize(ArxSerializer& Serializer) override;

    void RemoveTimer(int Id);
private:
    struct FTimer
    {
        ArxTimeDuration Interval;
        ArxTimePoint End;
        ArxEntityId EntityId;
        int Id;

        inline bool IsRepeated() const { return Interval > 0; }
        inline bool IsInfinity() const { return End == 0; }

        friend inline ArxSerializer& operator << (ArxSerializer& Ser, FTimer& Timer)
        {
            Ser << Timer.Interval;
            Ser << Timer.End;
            Ser << Timer.EntityId;
            Ser << Timer.Id;

            return Ser;
        }

        friend inline FString LexToString(const FTimer& Timer)
        {
            return FString::Printf(TEXT("Timer{%f, %f, %d, %d }"), (double)Timer.Interval, (double)Timer.End, Timer.EntityId, Timer.Id);
        }
    };

    struct FTimerList
    {
        ArxTimePoint Begin;
        TSortedMap<int, FTimer> Timers;

        friend inline ArxSerializer& operator << (ArxSerializer& Serializer, FTimerList& TimerList)
        {
            ARX_SERIALIZE_MEMBER_FAST(TimerList.Begin);
            ARX_SERIALIZE_MEMBER_FAST(TimerList.Timers);
            return Serializer;
        }

        friend inline FString LexToString(const FTimerList& TimerList)
        {
            FString Ret;
            Ret += LexToString(TimerList.Begin) + TEXT("\n");
            Ret += LexToString(TimerList.Timers);
            return Ret;
        }
    };

    TSortedMap<ArxTimeDuration, FTimerList> RepeatedTimers;
    TArray<TPair<ArxTimePoint, FTimer>> DelayedTimers;
    TArray<int> RemoveList;
    TSortedMap<int, ArxTimeDuration> TimerMap;
    ArxTimePoint TotalTime = 0;
    int TotalTimerCount = 0;
};

