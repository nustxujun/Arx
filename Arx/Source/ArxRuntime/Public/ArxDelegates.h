#pragma once 

#include "CoreMinimal.h"
#include "ArxCommon.h"

namespace ArxDelegates
{
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnServerCommands, int FrameId, const TArray<uint8>& Commands);
    extern ARXRUNTIME_API FOnServerCommands OnServerCommands;

    DECLARE_MULTICAST_DELEGATE_FiveParams(FOnServerReceiveSnapshot, ArxPlayerId PId, int FrameId,const TArray<uint8>& Data, uint32 , bool);
    extern ARXRUNTIME_API FOnServerReceiveSnapshot OnServerReceiveSnapshot;
};