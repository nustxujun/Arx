#include "ArxReplaySubsystem.h"
#include "ArxDelegates.h"
#include "ArxWorld.h"


bool UArxReplaySubsystem::ShouldCreateSubsystem(UObject* Outer)const
{
	auto Type = Cast<UWorld>(Outer)->WorldType;
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}

bool UArxReplaySubsystem::IsServer()
{
	auto NetMode = GetWorld()->GetNetMode();
	return NetMode == NM_DedicatedServer || NetMode == NM_ListenServer;
}


void UArxReplaySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	CommandHandle = ArxDelegates::OnServerCommands.AddUObject(this, &UArxReplaySubsystem::OnCommands);
	SnapshotHandle = ArxDelegates::OnClientSnapshot.AddUObject(this, &UArxReplaySubsystem::OnSnapshot);
	LocalSnapshotHandle = ArxDelegates::OnClientWorldStep.AddUObject(this, &UArxReplaySubsystem::OnLocalSnapshot);




}

void UArxReplaySubsystem::PrepareDirectory()
{
	if (!CommandDir.IsEmpty())
		return;
	auto Path = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), TEXT("Arx"));
	auto DateTime = FDateTime::Now().ToString(TEXT("%Y_%m_%d-%H_%M_%S"));
	Path = FPaths::Combine(Path, DateTime);
	Path = FPaths::Combine(Path, FString::Printf(TEXT("%s"), *GetWorld()->GetName()));

	auto& FileMgr = IFileManager::Get();
	FileMgr.MakeDirectory(*Path, true);


	CommandDir = FPaths::Combine(Path, TEXT("commands"));

	LocalSnapshotDir = FPaths::Combine(Path, TEXT("Snapshots/Local"));
	RemoteSnapshotDir = FPaths::Combine(Path, TEXT("Snapshots/Remote"));
}

void UArxReplaySubsystem::Deinitialize()
{
	ArxDelegates::OnServerCommands.Remove(CommandHandle);
	ArxDelegates::OnClientSnapshot.Remove(SnapshotHandle);
	ArxDelegates::OnClientSnapshot.Remove(LocalSnapshotHandle);
	
	CommandsFile.Reset();
	RemoteSnapshots.Reset();
}

void UArxReplaySubsystem::OnCommands(int FrameId, const TArray<uint8>& Cmds)
{
	if (!IsServer())
		return;

	if (!CommandsFile)
	{
		PrepareDirectory();
		auto& FileMgr = IFileManager::Get();
		CommandsFile = MakeShareable<FArchive>(FileMgr.CreateFileWriter(*CommandDir, FILEWRITE_Append | FILEWRITE_AllowRead));
		check(CommandsFile);
	}

	(*CommandsFile) << FrameId;

	int Len = Cmds.Num();
	*CommandsFile << Len ;
	CommandsFile->Serialize((void*)Cmds.GetData(),Len);
	CommandsFile->Flush();
}

void UArxReplaySubsystem::OnSnapshot(ArxPlayerId PId, int FrameId, const TArray<uint8>& Snapshot, uint32 Hash, bool bDiscard)
{
	if (!IsServer())
		return;
	auto File = RemoteSnapshots.FindRef(PId);
	if (!File)
	{
		PrepareDirectory();

		IFileManager::Get().MakeDirectory(*RemoteSnapshotDir,true);
		auto Path = FPaths::Combine(RemoteSnapshotDir, FString::Printf(TEXT("%d"), PId));
		File = MakeShareable<FArchive>(IFileManager::Get().CreateFileWriter(*Path, FILEWRITE_AllowRead));
		RemoteSnapshots.Add(PId, File);
	}
	check(Hash != 0)
	int Count = Snapshot.Num();
	static int CurFrameId = 0;
	*File << FrameId;
	*File << Hash;
	*File << bDiscard;
	*File << Count;
	File->Serialize((void*)Snapshot.GetData(), Count);
	File->Flush();
}

void UArxReplaySubsystem::OnLocalSnapshot(ArxWorld* World, ArxPlayerId PId, int FrameId)
{
	if (IsServer())
		return;

	PrepareDirectory();

	auto Path = FPaths::Combine(LocalSnapshotDir, FString::Printf(TEXT("%d"), PId));
	IFileManager::Get().MakeDirectory(*Path, true);
	auto File = IFileManager::Get().CreateFileWriter(*FPaths::Combine(Path, LexToString(FrameId)), FILEWRITE_AllowRead);
	
	TArray<uint8> DebugData;
	ArxDebugSerializer Ser(DebugData);
	World->Serialize(Ser);

	int Count = DebugData.Num();
	*File << Count;
	File->Serialize(DebugData.GetData(),Count);
	delete File;
}
