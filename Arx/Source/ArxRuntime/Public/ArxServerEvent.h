#pragma once 

#include "ArxCommon.h"

struct ArxServerEvent
{
    enum Event
    {
        PLAYER_ENTER,

        PLAYER_LEAVE,
    };

};

struct ARXRUNTIME_API ArxServerCommand
{

    int Event;
    ArxPlayerId PlayerId;

    void Serialize(ArxSerializer& Ser);
    void Execute(ArxWorld&)const;
};
