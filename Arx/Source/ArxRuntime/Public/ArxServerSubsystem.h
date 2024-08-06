#pragma once 

#include "CoreMinimal.h"
#include "ArxCommon.h"
#include "ArxRandomArray.h"
#include "ArxServerEvent.h"
#include "ArxServerSubsystem.generated.h"

class ArxServerPlayer;

UCLASS()
class ARXRUNTIME_API UArxServerSubsystem: public UWorldSubsystem
{
    GENERATED_BODY()
public:
    static UArxServerSubsystem& Get(UWorld* World);

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
    bool ShouldCreateSubsystem(UObject* Outer) const override;
    
    void AddCommands(ArxPlayerId PlayerId, int FrameId, const TArray<uint8>& Commands);
    const TArray<uint8>& GetCommands(int FrameId);
    bool HasCommands(int FrameId);
    
    uint32 GetHash(int FrameId);
    const TArray<uint8>& GetSnapshot(int FrameId);
    int GetLatestVerifiedFrame(){return VerifiedFrame - 1;}

    int GetCurrentFrame();
    
    void Update(); 
    void Start(float Interval);
    void Step();
    void Pause();

    ArxPlayerId RegisterPlayer(ArxServerPlayer* Player);
    void UnregisterPlayer(ArxPlayerId id);
private:
    void VerifyFrames();
    void AddServerCommand(ArxServerEvent::Event Event, ArxPlayerId PId);
private:
    TArray<TArray<uint8>> Frames ;
    TRandomArray<TArray<uint8>> VerifiedSnapshots;
    TRandomArray<uint32> VerifiedHashs;
    TMap<ArxPlayerId, ArxServerPlayer*> Players;
#if ENGINE_MAJOR_VERSION >= 5
    FTSTicker::
#endif
    FDelegateHandle TickHandle;
    ArxPlayerId UniquePlayerId = 0;
    int CurrentFrame = 0;
    int VerifiedFrame = 0;
    float TotalTime = 0;
    bool bIsPaused = false;
};