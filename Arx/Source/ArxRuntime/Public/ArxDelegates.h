#pragma once 

#include "CoreMinimal.h"
#include "ArxCommon.h"

class ARXRUNTIME_API ArxDelegates
{
public:
    DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnClientWorldStep, ArxWorld* World, ArxPlayerId, int);
    static FOnClientWorldStep OnClientWorldStep;

    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnServerCommands, int FrameId, const TArray<uint8>& Commands);
    static FOnServerCommands OnServerCommands;

    DECLARE_MULTICAST_DELEGATE_FiveParams(FOnClientSnapshot, ArxPlayerId PId, int FrameId,const TArray<uint8>& Data, uint32 , bool);
    static FOnClientSnapshot OnClientSnapshot;


};