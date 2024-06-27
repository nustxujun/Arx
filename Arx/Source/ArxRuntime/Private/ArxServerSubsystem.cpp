#include "ArxServerSubsystem.h"

#include "ArxPlayer.h"

DECLARE_CYCLE_STAT(TEXT("Arx Server Tick"), STAT_ArxServerTick, STATGROUP_ArxGroup);

void UArxServerSubsystem::Initialize(FSubsystemCollectionBase& Collection) 
{
    Super::Initialize(Collection);
	Frames.AddDefaulted(1);
}

void UArxServerSubsystem::Deinitialize() 
{
    Super::Deinitialize();

	Frames.Reset();
	if (TickHandle.IsValid())
		FTicker::GetCoreTicker().RemoveTicker(TickHandle);
}

void UArxServerSubsystem::Start(float Interval)
{
	if (TickHandle.IsValid())
		return ;

	TickHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, Interval](auto DeltaTime) {
		SCOPE_CYCLE_COUNTER(STAT_ArxServerTick);

		TotalTime += DeltaTime;
		while (TotalTime >= Interval)
		{
			if (!bIsPaused)
				Step();
#if WITH_EDITOR
			TotalTime = 0;
#else
			TotalTime -= Interval;
#endif;
		}
		return true;
	}), 0);
}

void UArxServerSubsystem::Step()
{
	if (Players.Num() == 0)
		return;
	VerifyFrames();
	Update();
}

void UArxServerSubsystem::Pause()
{
	bIsPaused = true;
}


bool UArxServerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	//auto bServer = Cast<UGameInstance>(Outer)->GetWorldContext()->RunAsDedicated;
	//return bServer;
	return true;
}

void UArxServerSubsystem::AddCommands(ArxPlayerId PlayerId, int FrameId, const TArray<uint8>& Commands)
{
	if (Commands.Num() == 0)
		return;

	int* InNumPtr = (int*)Commands.GetData();
	if (*InNumPtr == 0)
		return;

	if (FrameId >= Frames.Num())
	{
		Frames.SetNum(FrameId + 1);
	}

	if (FrameId < CurrentFrame)
		FrameId = CurrentFrame;


	auto& CurrentCommands = Frames[FrameId];
	CurrentCommands.Key ++;
	auto& Data = CurrentCommands.Value;
	int TotalCount = Data.Num();
	int Count = Commands.Num();

	Data.SetNum(TotalCount + Count + sizeof(PlayerId));
	FMemory::Memcpy(Data.GetData() + TotalCount, &PlayerId, sizeof(PlayerId));
	FMemory::Memcpy(Data.GetData() + TotalCount + sizeof(PlayerId), Commands.GetData(), Count);
}

void UArxServerSubsystem::VerifyFrames()
{
	const int SampleCount = 1;


	if (Players.Num() < SampleCount)
		return;

	while (true)
	{
		int PreparedCount = 0;
		TSortedMap<uint32, int, FDefaultAllocator, TGreater<uint32>> SortedHash;
		for (auto Player : Players)
		{
			if ( Player.Value->HasHash(VerifiedFrame))
			{
				PreparedCount++;
				auto Hash = Player.Value->GetHash(VerifiedFrame);
				auto& Count = SortedHash.FindOrAdd(Hash);
				Count++;
			}
		}

		if (PreparedCount < SampleCount)
			break;

		auto Most = (*SortedHash.begin()).Key;
		VerifiedHashs.Set(VerifiedFrame, Most);
		for (auto Player : Players)
		{
			if (!VerifiedSnapshots.Has(VerifiedFrame) && Player.Value->HasSnapshot(VerifiedFrame))
			{
				VerifiedSnapshots.Set(VerifiedFrame, Player.Value->GetSnapshot(VerifiedFrame)); // store snapshot
				break;
			}
		}

		check(VerifiedSnapshots.Has(VerifiedFrame));
		auto& Snapshot = VerifiedSnapshots.Get(VerifiedFrame);
		for (auto Player : Players)
		{
			if (Player.Value->HasHash(VerifiedFrame) && Player.Value->GetHash(VerifiedFrame) != Most)
			{
				Player.Value->ResponseSnapshot(VerifiedFrame, Snapshot);
				Player.Value->ResponseVerifiedFrame(VerifiedFrame);
			}
		}

		VerifiedFrame ++;
	}
}

void UArxServerSubsystem::Update()
{
	auto& Data = Frames[CurrentFrame];
	// sync
	for (auto Player : Players)
	{
		Player.Value->ResponseCommand(CurrentFrame, Data.Key, Data.Value);
		Player.Value->SyncStep(CurrentFrame);
	}

	// step
	CurrentFrame++;

	if (CurrentFrame >= Frames.Num())
	{
		Frames.AddDefaulted(1);
	}

}


const TPair<int, TArray<uint8>>& UArxServerSubsystem::GetCommands(int FrameId)
{
	return Frames[FrameId];
}

int UArxServerSubsystem::GetCurrentFrame()
{
	return CurrentFrame;
}

bool UArxServerSubsystem::HasCommands(int FrameId)
{
	return FrameId < Frames.Num();
}

ArxPlayerId UArxServerSubsystem::RegisterPlayer(ArxServerPlayer* Player)
{
	auto Id = UniquePlayerId++;
	Players.Add(Id, Player);
	return Id;
}

void UArxServerSubsystem::UnregisterPlayer(ArxPlayerId Id)
{
	Players.Remove(Id);
}

uint32 UArxServerSubsystem::GetHash(int FrameId)
{
	return VerifiedHashs.Get(FrameId);
}

static const TArray<uint8> EmptySnapshot;
const TArray<uint8>& UArxServerSubsystem::GetSnapshot(int FrameId)
{
	if (FrameId < 0 || !VerifiedSnapshots.Has(FrameId))
		return EmptySnapshot;
	return VerifiedSnapshots.Get(FrameId);
}


