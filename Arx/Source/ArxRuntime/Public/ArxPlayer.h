#pragma once 

#include "CoreMinimal.h"
#include "ArxCommon.h"
#include "ArxRandomArray.h"

class ARXRUNTIME_API ArxPlayerChannel
{
public:
    virtual ~ArxPlayerChannel() = default;

    virtual void SendCommand(int FrameId, const TArray<uint8>& Command) = 0;
    virtual void SendHash(int FrameId, uint32 HashValue) = 0;
    virtual void SendSnapshot(int FrameId, const TArray<uint8>& Snapshot) = 0;
    virtual void RequestCommand(int FrameId) = 0;
    virtual void RequestRegister() = 0;
    virtual void RequestUnregister() = 0;
    virtual void RequestSnapshot(int FrameId) = 0;

    virtual void ResponseVerifiedFrame(int FrameId) = 0; // receive only failed
    virtual void ResponseCommand(int FrameId, int Count, const TArray<uint8>& Command) = 0;
    virtual void ResponseRegister(ArxPlayerId Id) = 0;
    virtual void ResponseUnregister() = 0;
    virtual void ResponseSnapshot(int FrameId, const TArray<uint8>& Snapshot) = 0;
    
    virtual void SyncStep(int FrameId) = 0;
    virtual void SyncStart() = 0;

    virtual void Update() = 0;

    virtual ArxPlayerId GetPlayerId() = 0;
};

class ARXRUNTIME_API ArxClientPlayer : public ArxPlayerChannel
{
public:
    ArxClientPlayer(UWorld* InWorld,int VerificationCycle);

    void ResponseCommand(int FrameId,int Count, const TArray<uint8>& Command) override final;
    void ResponseRegister(ArxPlayerId Id) override final;
    void ResponseUnregister() override final;
    void ResponseVerifiedFrame(int FrameId) override final; // receive if failed
    void ResponseSnapshot(int FrameId, const TArray<uint8>& Snapshot)  override final;
    void SyncStep(int FrameId) override final;
    void SyncStart()override final;

    virtual void Update();
    virtual void OnFrame() = 0;
    virtual void OnRegister(ArxWorld& World) = 0;
    virtual void CreateSnapshot(TArray<uint8>& Data) ;

    int GetCurrentFrameId(){return CurrentFrame;}
    ArxPlayerId GetPlayerId() override { return PlayerId; }
    ArxWorld& GetWorld(){return World;}
private:
    void Tick(bool bBacktrace);
    int GetRealFrameId(int FrameId);

private:
    ArxWorld World;
    TRandomArray<TPair<int, TArray<uint8>>> Commands;
    TRandomArray<TArray<uint8>> Snapshots;

    ArxPlayerId PlayerId = NON_PLAYER_CONTROL;
    int CurrentFrame = 0;
    int ServerFrame = 0;
    int TargetFrame = 0;
    int VerificationCycle = 1;
    bool bStart = false;
};

class UArxServerSubsystem;
class ARXRUNTIME_API ArxServerPlayer : public ArxPlayerChannel
{
public:
    ArxServerPlayer(UArxServerSubsystem* Server);

    void SendSnapshot(int FrameId, const TArray<uint8>& Snapshot) override final;
    void SendCommand(int FrameId, const TArray<uint8>& Command) override final;
    void SendHash(int FrameId, uint32 HashValue) override final;
    void RequestCommand(int FrameId) override final;
    void RequestRegister() override final;
    void RequestUnregister() override final;
    void RequestSnapshot(int FrameId) override final;

    bool HasHash(int FrameId);
    uint32 GetHash(int FrameId);

    bool HasSnapshot(int FrameId);
    const TArray<uint8>& GetSnapshot(int FrameId);
    void RemoveSnapshot(int FrameId);

    void Update() override {};
    virtual ArxPlayerId GetPlayerId() {return PlayerId;};

private:
    TRandomArray<uint32> FrameHashValues;
    TRandomArray<TArray<uint8>> Snapshots;
    UArxServerSubsystem* Server;
    ArxPlayerId PlayerId = NON_PLAYER_CONTROL;

};


