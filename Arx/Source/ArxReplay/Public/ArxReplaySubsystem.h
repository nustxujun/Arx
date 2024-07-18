#pragma once 

#include "CoreMinimal.h"

#include "ArxCommon.h"

#include "ArxReplaySubsystem.generated.h"

UCLASS()
class ARXREPLAY_API UArxReplaySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    void Initialize(FSubsystemCollectionBase& Collection);
    void Deinitialize();
    bool ShouldCreateSubsystem(UObject* Outer) const override;

private:
    void OnCommands(int FrameId, const TArray<uint8>& Cmds);
    void OnSnapshot(ArxPlayerId PId, int FrameId, const TArray<uint8>& Snapshot, uint32 Hash, bool bDiscard);
    void OnLocalSnapshot(ArxWorld* World, ArxPlayerId PId, int FrameId);

    bool IsServer();

    void PrepareDirectory();
private:
    FDelegateHandle CommandHandle;
    FDelegateHandle SnapshotHandle;
    FDelegateHandle LocalSnapshotHandle;
    TSharedPtr<FArchive> CommandsFile;
    TMap<ArxPlayerId, TSharedPtr<FArchive>> RemoteSnapshots;

    FString CommandDir;
    FString LocalSnapshotDir;
    FString RemoteSnapshotDir;
};