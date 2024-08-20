#include "ArxPlayer.h"
#include "ArxServerSubsystem.h"
#include "ArxWorld.h"
#include "ArxCommandSystem.h"
#include "ArxDelegates.h"

DECLARE_CYCLE_STAT(TEXT("Create Snapshot"), STAT_VerifyFrame, STATGROUP_ArxGroup);
DECLARE_CYCLE_STAT(TEXT("Process Commands"), STAT_ProcessCommands, STATGROUP_ArxGroup);
DECLARE_CYCLE_STAT(TEXT("Recover from snapshot"), STAT_Recover, STATGROUP_ArxGroup);
DECLARE_CYCLE_STAT(TEXT("World Update"), STAT_WorldUpdate, STATGROUP_ArxGroup);

#define RECOVER_FROM_SNAPSHOT 0

ArxClientPlayer::ArxClientPlayer(UWorld* InWorld, int InVerificationCycle):World(InWorld),VerificationCycle(InVerificationCycle)
{

}

void ArxClientPlayer::Initalize() 
{
	World.Setup([this](ArxWorld& W) {
		OnRegister(W);
	});
}

void ArxClientPlayer::ResponseCommand(int FrameId, const TArray<uint8>& Command)
{
	Commands.Set(FrameId,  Command);
}

void ArxClientPlayer::ResponseRegister(ArxPlayerId Id)
{
	PlayerId = Id;
}

void ArxClientPlayer::ResponseUnregister()
{
}


void ArxClientPlayer::SyncStep(int FrameId)
{
	ServerFrame = FrameId;
}

void ArxClientPlayer::SyncStart()
{
	bStart = true;
}

void ArxClientPlayer::ResponseVerifiedFrame(int FrameId)
{
	SCOPE_CYCLE_COUNTER(STAT_Recover);

#if RECOVER_FROM_SNAPSHOT
	TargetFrame = CurrentFrame;
	CurrentFrame = FrameId;
	ArxReader Serializer(Snapshots.Get(FrameId));
	World.Serialize(Serializer);
#else
	TargetFrame = FMath::Max(CurrentFrame, FrameId) ;
	if (CurrentFrame != 0)
	{
		ArxReader Serializer(Snapshots.Get(0));
		World.Serialize(Serializer);
	}
	CurrentFrame = 0;
#endif

	Tick(true);

	TargetFrame = ServerFrame;
}

void ArxClientPlayer::ResponseSnapshot(int FrameId, const TArray<uint8>& Snapshot) 
{
	auto RealFrameId = GetRealFrameId(FrameId);
	//check(Commands.Has(RealFrameId));
	if (Snapshot.Num() != 0)
	{
		Snapshots.Set(RealFrameId, Snapshot);
	}
}

void ArxClientPlayer::Update()
{
	if (!bStart)
		return;

	TargetFrame = ServerFrame;
	Tick(false);
}

void ArxClientPlayer::MakeSnapshot(int FrameId)
{
	SCOPE_CYCLE_COUNTER(STAT_VerifyFrame);
	TArray<uint8> Data;
	ArxWriter Serializer(Data);
	World.Serialize(Serializer);
	SendSnapshot(FrameId, Data);
}

void ArxClientPlayer::Tick(bool bBacktrace)
{
	if (PlayerId == NON_PLAYER_CONTROL || 
		CurrentFrame >= TargetFrame )
		return;


	while ( CurrentFrame < TargetFrame && World.IsPrepared())
	{
		SCOPE_CYCLE_COUNTER(STAT_WorldUpdate);
		
		if (!bBacktrace && CurrentFrame % VerificationCycle == 0)
		{
			MakeSnapshot(CurrentFrame);
		}

		auto& ComSys = World.GetSystem<ArxCommandSystem>();

		{
			if (Commands.Has(CurrentFrame))
			{
				auto& Data = Commands.Get(CurrentFrame);

				SCOPE_CYCLE_COUNTER(STAT_ProcessCommands);
				ComSys.ReceiveCommands(&Data);
			}
		}

		World.Update(CurrentFrame);

		{
			TArray<uint8> Data;
			ArxWriter Serializer(Data);
			ComSys.SendAllCommands(Serializer);

			if (Data.Num() != 0)
				SendCommand(CurrentFrame, (Data));
		}
		CurrentFrame++;
	}
}

int ArxClientPlayer::GetRealFrameId(int FrameId)
{
	return FrameId ;
}

ArxServerPlayer::ArxServerPlayer(UArxServerSubsystem* InServer) :Server(InServer)
{

}

void ArxServerPlayer::SendSnapshot(int FrameId, const TArray<uint8>& Snapshot)
{
	auto LatestVerifiedFrame = Server->GetLatestVerifiedFrame();
	auto Hash = FCrc::MemCrc32(Snapshot.GetData(), Snapshot.Num());

	bool bDiscard = false;
	if (FrameId <= LatestVerifiedFrame)
	{
		bDiscard = true;

		auto VerifiedHash = Server->GetHash(FrameId);
		if (VerifiedHash != Hash)
		{
			// if snapshot is invalid, recover client from latest snapshot
			auto& SN = Server->GetSnapshot(LatestVerifiedFrame);
			check(SN.Num() > 0);
			ResponseSnapshot(LatestVerifiedFrame, SN);
			ResponseVerifiedFrame(LatestVerifiedFrame);
		}
	}
	else
	{
		FrameHashValues.Set(FrameId, Hash);
		Snapshots.Set(FrameId, Snapshot);
	}



	ArxDelegates::OnServerReceiveSnapshot.Broadcast(GetPlayerId(), FrameId, Snapshot, Hash, bDiscard);
}

void ArxServerPlayer::SendCommand(int FrameId, const TArray<uint8>& Command)
{
	Server->AddCommands(PlayerId, FrameId, Command);
}


void ArxServerPlayer::RequestCommand(int FrameId)
{
	if (Server->HasCommands(FrameId))
	{
		auto& Cmds = Server->GetCommands(FrameId);
		ResponseCommand(FrameId,  Cmds);
	}
}

void ArxServerPlayer::RequestRegister()
{
	PlayerId = Server->RegisterPlayer(this);
}

void ArxServerPlayer::RequestUnregister()
{
	Server->UnregisterPlayer(PlayerId);
}

void ArxServerPlayer::RequestSnapshot(int FrameId)
{
	auto& Snapshot = Server->GetSnapshot(FrameId);
	ResponseSnapshot(FrameId, Snapshot);
}

bool ArxServerPlayer::HasHash(int FrameId)
{
	return FrameHashValues.Has(FrameId);
}

uint32 ArxServerPlayer::GetHash(int FrameId)
{
	return FrameHashValues.Get(FrameId);
}

bool ArxServerPlayer::HasSnapshot(int FrameId)
{
	return Snapshots.Has(FrameId);
}

const TArray<uint8>& ArxServerPlayer::GetSnapshot(int FrameId)
{
	return Snapshots.Get(FrameId);
}

void ArxServerPlayer::RemoveSnapshot(int FrameId)
{
	Snapshots.Remove(FrameId);
}