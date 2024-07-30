#include "ArxPlayer.h"
#include "ArxServerSubsystem.h"
#include "ArxWorld.h"
#include "ArxCommandSystem.h"
#include "ArxDelegates.h"

DECLARE_CYCLE_STAT(TEXT("Verify Frame"), STAT_VerifyFrame, STATGROUP_ArxGroup);
DECLARE_CYCLE_STAT(TEXT("Process Commands"), STAT_ProcessCommands, STATGROUP_ArxGroup);
DECLARE_CYCLE_STAT(TEXT("Recover from snapshot"), STAT_Reocver, STATGROUP_ArxGroup);

ArxClientPlayer::ArxClientPlayer(UWorld* InWorld, int InVerificationCycle):World(InWorld),VerificationCycle(InVerificationCycle)
{

}

void ArxClientPlayer::ResponseCommand(int FrameId, const TArray<uint8>& Command)
{
	Commands.Set(FrameId,  Command);
}

void ArxClientPlayer::ResponseRegister(ArxPlayerId Id)
{
	PlayerId = Id;

	World.Setup([this](ArxWorld& W) {
		OnRegister(W);
	});
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
	SCOPE_CYCLE_COUNTER(STAT_Reocver);
	auto RealFrameId = GetRealFrameId(FrameId);

	TargetFrame = CurrentFrame;
	CurrentFrame = RealFrameId;
	ArxReader Serializer(Snapshots.Get(RealFrameId));
	World.Serialize(Serializer);
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

void ArxClientPlayer::CreateSnapshot(TArray<uint8>& Data)
{
	ArxWriter Serializer(Data);
	World.Serialize(Serializer);
}

void ArxClientPlayer::Tick(bool bBacktrace)
{
	if (PlayerId == NON_PLAYER_CONTROL || 
		CurrentFrame >= TargetFrame )
		return;


	while ( CurrentFrame < TargetFrame)
	{
		//if (!Commands.Has(CurrentFrame))
		//	return;

		if (!bBacktrace && CurrentFrame % VerificationCycle == 0)
		{
			SCOPE_CYCLE_COUNTER(STAT_VerifyFrame);

			TArray<uint8> Data;
			CreateSnapshot(Data);
			//auto VirtualFrame = CurrentFrame / VerificationCycle;

			SendSnapshot(CurrentFrame, Data);
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

		World.Update();

		ArxDelegates::OnClientWorldStep.Broadcast(&World, GetPlayerId(), CurrentFrame);

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

		auto VerifiedHash = Server->GetHash(LatestVerifiedFrame);
		if (VerifiedHash != Hash)
		{
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



	ArxDelegates::OnClientSnapshot.Broadcast(GetPlayerId(), FrameId, Snapshot, Hash, bDiscard);
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
	ResponseRegister(PlayerId);
	
	auto FrameId = Server->GetCurrentFrame();
	SyncStep(FrameId);



	auto CurrentFrameId = Server->GetCurrentFrame();
	for (int i = 0; i < CurrentFrameId; ++i)
	{
		auto& Cmds = Server->GetCommands(i);
		ResponseCommand(i,  Cmds);
	}

	
	{
		auto VerifiedFrame = Server->GetLatestVerifiedFrame();
		auto& Snapshot = Server->GetSnapshot(VerifiedFrame);
		if (Snapshot.Num() != 0)
		{
			ResponseSnapshot(VerifiedFrame, Snapshot);
			ResponseVerifiedFrame(VerifiedFrame);
		}
	}
	


	SyncStart();
}

void ArxServerPlayer::RequestUnregister()
{
	Server->UnregisterPlayer(PlayerId);
	ResponseUnregister();
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