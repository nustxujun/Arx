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
    };

    struct FTimerList
    {
        ArxTimePoint Begin;
        TSortedMap<int, FTimer> Timers;
    };

    TSortedMap<ArxTimeDuration, FTimerList> RepeatedTimers;
    TArray<TUniquePtr<TPair<ArxTimePoint, FTimer>>> DelayedTimers;
    TArray<int> RemoveList;
    TMap<int, ArxTimeDuration> TimerMap;
    ArxTimePoint TotalTime = 0;
    int TotalTimerCount = 0;
};

