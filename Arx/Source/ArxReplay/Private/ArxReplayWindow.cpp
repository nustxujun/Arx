#include "ArxReplayWindow.h"
#include "ArxWorld.h"
#include "ArxCommandSystem.h"

#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Brushes/SlateColorBrush.h"
#include "Fonts/FontMeasure.h"
#include "EngineUtils.h" 
#include "HAL/PlatformApplicationMisc.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#if WITH_EDITOR
#include "Editor.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "DesktopPlatformModule.h"
#endif

#define LOCTEXT_NAMESPACE "ArxReplay"


static int RegisterReplayCommand = []() {
	static TAutoConsoleVariable<int> ConsoleVariable(TEXT("arx.showreplay"), 0, TEXT(""));
	ConsoleVariable->SetOnChangedCallback(FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* CVar) {
		static TWeakPtr<SWidget> WeakRef;
		if (!WeakRef.IsValid())
		{
			auto Widget = SNew(SBorder)
				[SNew(ArxReplayWindow)];
			WeakRef = Widget;
			if (GEngine->GameViewport)
				GEngine->GameViewport->AddViewportWidgetContent(Widget, 1000);
		}
#if PLATFORM_DESKTOP
		if (GEngine->GameViewport)
		{
			FWorldContext* WorldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport);
			auto World = WorldContext->World();
			GEngine->GetFirstLocalPlayerController(World)->SetShowMouseCursor(true);
		}
#endif

		}));
	return 0;
}();


void ArxReplayFrameTrack::Construct(const FArguments& InArgs)
{
	Reset();
}


void ArxReplayFrameTrack::Reset()
{
	Freq = 1;
	//BeginFrame = 0;
	BeginX = FrameBeginX;
	bIsViewportDirty = true;
	Tracks.Reset();
}

void ArxReplayFrameTrack::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (ThisGeometry != AllottedGeometry || bIsViewportDirty)
	{
		bIsViewportDirty = false;
		ViewWidth = AllottedGeometry.GetLocalSize().X - 10;
		ViewHeight = AllottedGeometry.GetLocalSize().Y;
		//Viewport.SetSize(ViewWidth, ViewHeight);
		bIsStateDirty = true;
	}

	ThisGeometry = AllottedGeometry;
}


static void DrawBox(const FGeometry& Geometry, int LayerId, FSlateWindowElementList& OutDrawElements, float X, float Y, float W, float H, const ESlateDrawEffect InDrawEffects, const FSlateBrush* Brush, const FLinearColor& Color)
{
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry.ToPaintGeometry(FVector2D(W, H), FSlateLayoutTransform(1.0f, FVector2D(X, Y))), Brush, InDrawEffects, Color);
}

static void DrawText(const FGeometry& Geometry, int LayerId, FSlateWindowElementList& OutDrawElements, float X, float Y, const FString& Text, const FSlateFontInfo& Font, const ESlateDrawEffect InDrawEffects, const FLinearColor& Color)
{
	FSlateDrawElement::MakeText(OutDrawElements, LayerId, Geometry.ToPaintGeometry(FSlateLayoutTransform(1.0f, FVector2D(X, Y))), Text, Font, InDrawEffects, Color);
}

static void DrawLine(const FGeometry& Geometry, int LayerId, FSlateWindowElementList& OutDrawElements, float X1, float Y1, float X2, float Y2, const ESlateDrawEffect InDrawEffects, float Thickness, const FLinearColor& Color)
{
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), TArray<FVector2D>{ {X1, Y1}, { X2,Y2 }}, InDrawEffects, Color, false, Thickness);
}

static void DrawLines(const FGeometry& Geometry, int LayerId, FSlateWindowElementList& OutDrawElements, const TArray<FVector2D>& Lines, const ESlateDrawEffect InDrawEffects, float Thickness, const FLinearColor& Color)
{
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), Lines, InDrawEffects, Color, false, Thickness);
}

int32 ArxReplayFrameTrack::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const bool bEnabled = ShouldBeEnabled(bParentEnabled);
	//const ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	const auto DrawEffects = ESlateDrawEffect::NoPixelSnapping;
	auto Brush = FSlateColorBrush(FLinearColor::White);
	FSlateFontInfo SummaryFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);


	const auto BackGroundLayerId = LayerId;
	const auto Frame1LayerId = BackGroundLayerId + 1;
	const auto Frame2LayerId = Frame1LayerId + 1;
	const auto Frame3LayerId = Frame2LayerId + 1;
	const auto Frame4LayerId = Frame3LayerId + 1;
	const auto LineLayerId = Frame4LayerId + 1;
	const auto TextLayerId = LineLayerId + 1;
	const auto HighlightLayerId = TextLayerId + 1;

	LayerId = HighlightLayerId + 1;

	auto DrawBoxLocal = [&](float x, float y, float w, float h, const FLinearColor& Color, auto LayerId)
	{
		DrawBox(AllottedGeometry, LayerId, OutDrawElements, x, y, w, h, DrawEffects, &Brush, Color);
	};

	auto DrawTextLocal = [&](float x, float y, const auto& Text, const auto& Color, auto LayerId)
	{
		DrawText(AllottedGeometry, LayerId, OutDrawElements, x, y, Text, SummaryFont, DrawEffects, Color);
	};

	auto DrawLineLocal = [&](auto x1, auto y1, auto x2, auto y2, const auto& Color, auto thickness, auto LayerId)
	{
		DrawLine(AllottedGeometry, LayerId, OutDrawElements, x1, y1, x2, y2, DrawEffects, thickness, Color);
	};

	auto DrawLinesLocal = [&](const TArray<FVector2D>& Lines, const auto& Color, auto thickness, auto LayerId)
	{
		DrawLines(AllottedGeometry, LayerId, OutDrawElements, Lines, DrawEffects, thickness, Color);
	};

	const auto RealFrameWidth = FrameWidth / Freq;
	const bool bThin = RealFrameWidth < 3;
	const float RealBeginX = BeginX - FMath::Fmod(BeginX, RealFrameWidth);

	const FLinearColor BackgroundGray = { 0.015996,0.015996 ,0.015996 };
	const auto BorderColor = BackgroundGray;

	const auto MaxNumFrames = GetMaxNumFrames();
	const float HintMapping = (ViewWidth - FrameBeginX) / MaxNumFrames;
	const float HintWidth = FMath::Max(1.0f, HintMapping);


	// scale
	{
		const float ScaleStep = FrameWidth;
		auto FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		int Index = 0;
		int IndexScale = Freq;
		for (float X = RealBeginX; X < ViewWidth; X += ScaleStep, Index += 1)
		{
			if (X < FrameBeginX)
				continue;
			if (Index % 5 == 0)
			{
				if (Index % 10 == 0)
					DrawLineLocal(X, 0, X, 10, FLinearColor::Gray, 2, LineLayerId);
				else
					DrawLineLocal(X, 0, X, 7, FLinearColor::Gray, 1, LineLayerId);

				const FString Text = LexToString(Index * IndexScale);
				const FVector2D TextSize = FontMeasureService->Measure(Text, SummaryFont);

				DrawTextLocal(X - TextSize.X / 2, 12, Text, FLinearColor::Gray, TextLayerId);
			}
			else
			{
				DrawLineLocal(X, 0, X, 4, FLinearColor::Gray, 1, LineLayerId);
			}

		}
	}

	float MaxLen = FrameBeginX;
	int MinFrame = MaxNumFrames;
	int MaxFrame = 0;
	// frame
	{
		int Index = 0;
		for (auto& Track : Tracks)
		{
			const auto TrackY = FrameBeginY + Index * FrameHeight;
			DrawTextLocal(0, TrackY, Track.Name, FLinearColor::White, TextLayerId);

			int Count = Track.Frames.Num();
			for (int FrameId = 0; FrameId < Count; FrameId++)
			{
				auto X = RealBeginX + FrameId * RealFrameWidth;


				auto& Frame = Track.Frames[FrameId];


				int FrameLayerId;
				FLinearColor FrameColor;
				bool bDrawHint = false;
				if (Frame.State == FReplayFrame::E_NORMAL)
				{
					FrameLayerId = Frame2LayerId;
					FrameColor = FLinearColor::Green;
				}
				else if (Frame.State == FReplayFrame::E_DIFF)
				{
					FrameLayerId = Frame3LayerId;
					FrameColor = FLinearColor::Yellow;
					bDrawHint = true;
				}
				else if (Frame.State == FReplayFrame::E_ERROR)
				{
					FrameLayerId = Frame4LayerId;
					FrameColor = FLinearColor::Red;
					bDrawHint = true;
				}
				else
				{
					FrameColor = FLinearColor::Gray;
					FrameLayerId = Frame1LayerId;
				}

				if (bDrawHint)
					DrawBoxLocal(FrameBeginX + HintMapping * FrameId, HintBeginY + 2, HintWidth, HintHeight - 4, FrameColor, FrameLayerId);

				if (X < FrameBeginX)
					continue;

				MinFrame = FMath::Min(FrameId, MinFrame);

				if (X > ViewWidth || TrackY > ViewHeight)
					continue;

				MaxFrame = FMath::Max(FrameId, MaxFrame);


				DrawBoxLocal(X, TrackY, RealFrameWidth, FrameHeight, FrameColor, FrameLayerId);
				MaxLen = FMath::Max(MaxLen, X + RealFrameWidth);

				if (bThin)
					continue;

				//X += 1;
				auto Y = TrackY;

				auto W = RealFrameWidth;
				auto H = FrameHeight;

				DrawLinesLocal({ {X, Y}, {X + W, Y}, {X + W, Y + H}, {X, Y + H}, {X,Y } }, BorderColor, 1, LineLayerId);
			}

			Index++;
		}
	}

	auto CurFrame = GetFrameByPos(MousePosition.X, MousePosition.Y);
	// rod
	if (CurFrame.Value >= 0)
	{
		DrawLineLocal(MousePosition.X, 0, MousePosition.X, ViewHeight, FLinearColor::Gray * 0.5, 1, LineLayerId);
		DrawTextLocal(MousePosition.X + 5, FrameBeginY - 15, LexToString(CurFrame.Value), FLinearColor::Gray, TextLayerId);
	}

	// highlight
	if (CurFrame.Key >= 0 && CurFrame.Value >= 0 && CurFrame.Key < Tracks.Num())
	{
		auto& Track = Tracks[CurFrame.Key];
		if (CurFrame.Value < Track.Frames.Num())
		{
			do
			{
				auto X = RealBeginX + CurFrame.Value * RealFrameWidth;
				auto Y = FrameBeginY + CurFrame.Key * FrameHeight;

				if (X < FrameBeginX)
					break;

				auto W = RealFrameWidth + 1;
				auto H = FrameHeight + 1;

				DrawLinesLocal({ {X, Y}, {X + W, Y}, {X + W, Y + H}, {X, Y + H}, {X,Y} }, FLinearColor::White, 2, HighlightLayerId);
			} while (0);
		}
	}

	// background
	auto BGX = FMath::Max(FrameBeginX, RealBeginX);
	auto BGLen = FMath::Max(0.0f, MaxLen - BGX);
	DrawBoxLocal(BGX, 0, BGLen, HintBeginY, FLinearColor::Red * 0.25f, BackGroundLayerId);
	DrawBoxLocal(FrameBeginX, HintBeginY, BGLen, HintHeight, BackgroundGray, BackGroundLayerId);

	// select
	for (auto& Selected : Selecteds)
	{
		auto X = RealBeginX + Selected.Value * RealFrameWidth;
		auto Y = FrameBeginY + Selected.Key * FrameHeight;

		if (X < FrameBeginX || X >= BGX + BGLen || Y < FrameBeginY || Y + FrameHeight > HintBeginY)
			break;

		auto W = RealFrameWidth + 1;
		auto H = FrameHeight + 1;

		DrawLinesLocal({ {X, Y}, {X + W, Y}, {X + W, Y + H}, {X, Y + H}, {X,Y } }, FLinearColor::White, 2, HighlightLayerId);
	};


	{
		int Lower = FMath::RoundToInt(RealBeginX / RealFrameWidth);
		float X = FMath::Max(FrameBeginX, FrameBeginX + MinFrame * HintMapping);
		float Y = HintBeginY;
		float H = HintHeight;
		float W = FMath::Max(0, MaxFrame - MinFrame + 1) * HintMapping;
		//DrawLinesLocal({ {X, Y},{X + W, Y}, {X + W, Y + H}, {X, Y + H}, {X,Y } }, FLinearColor::White, 1, HighlightLayerId);
		DrawBoxLocal(FrameBeginX, Y, ViewWidth - FrameBeginX, H, FLinearColor::Gray * 0.5f, BackGroundLayerId);
		DrawBoxLocal(X, Y, W, H, FLinearColor::White, BackGroundLayerId);

	}




	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled && IsEnabled());
}


void ArxReplayFrameTrack::ScrollTrack(const FVector2D& Pos)
{
	auto RelX = Pos.X - FrameBeginX;

	const float RegionWidth = ViewWidth - FrameBeginX;
	const float RealHintWidth = RegionWidth / GetMaxNumFrames();
	const auto RealFrameWidth = FrameWidth / Freq;
	const float NumFrames = RegionWidth / RealFrameWidth;
	const auto FrameId = RelX / RealHintWidth;

	BeginX = -RealFrameWidth * (FrameId - NumFrames * 0.5f) + FrameBeginX;
}


FReply ArxReplayFrameTrack::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	auto Reply = FReply::Unhandled();
	MousePosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());


	if (HasMouseCapture() && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		if (Region == 2)
		{
			ScrollTrack(MousePosition);
		}
		else if (!MouseEvent.GetCursorDelta().IsZero() && Region == 1)
		{
			auto Offset = PressedMousePosition - MousePosition;
			PressedMousePosition = MousePosition;

			BeginX -= Offset.X;
			bMouseIsMoving = true;
		}
	}
	return Reply;
}

FReply ArxReplayFrameTrack::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	auto Reply = FReply::Unhandled();

	const float Delta = MouseEvent.GetWheelDelta();

	float Len = (MousePosition.X - BeginX) * Freq;
	if (Delta > 0)
		Freq = FMath::Min(1 << 16, Freq + 1);
	else
		Freq = FMath::Max(1, Freq - 1);


	BeginX = MousePosition.X - Len / Freq;


	return Reply;
}

FReply ArxReplayFrameTrack::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = FReply::Unhandled();

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		Reply = FReply::Handled().CaptureMouse(SharedThis(this));
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		Reply = FReply::Handled().CaptureMouse(SharedThis(this));
	}

	PressedMousePosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	bMouseIsMoving = false;

	const auto& Pos = PressedMousePosition;
	if (Pos.X < FrameBeginX || Pos.X > ViewWidth || Pos.Y < 0 || Pos.Y > ViewHeight)
		Region = 0;
	else if (Pos.Y < HintBeginY)
		Region = 1;
	else
	{
		Region = 2;
		ScrollTrack(PressedMousePosition);
	}
	return Reply;
}
FReply ArxReplayFrameTrack::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = FReply::Unhandled();
	int Index = 0;
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		Reply = FReply::Handled().ReleaseMouseCapture();
		Index = 0;
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		Reply = FReply::Handled().ReleaseMouseCapture();
		Index = 1;
	}

	auto Pos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	if (!bMouseIsMoving && (Pos - PressedMousePosition).Size() < 0.1f)
	{
		auto& Selected = Selecteds[Index];
		Selected = GetFrameByPos(Pos.X, Pos.Y);
		if (Tracks.IsValidIndex(Selected.Key) && Tracks[Selected.Key].Frames.IsValidIndex(Selected.Value))
		{
			auto Getter = [this](auto FrameId, auto Func)->const FReplayFrame*
			{
				for (auto& Track : Tracks)
				{
					if (Track.Frames.IsValidIndex(FrameId) && Func(Track.Name, Track.Frames[FrameId]))
						return &Track.Frames[FrameId];
				}
				return nullptr;
			};
			OnFrameChanged(Getter, Tracks[Selected.Key].Name, Tracks[Selected.Key].Frames[Selected.Value], Index);
		}
	}

	return Reply;
}
void ArxReplayFrameTrack::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{

}
void ArxReplayFrameTrack::OnMouseLeave(const FPointerEvent& MouseEvent)
{

}

TPair<int, int> ArxReplayFrameTrack::GetFrameByPos(float X, float Y)const
{
	auto RealWidth = FrameWidth / Freq;
	float RealBeginX = BeginX - FMath::Fmod(BeginX, RealWidth);

	X = X - RealBeginX;
	Y -= FrameBeginY;
	int FrameId = FMath::Floor(X / RealWidth);
	int Index = FMath::Floor(Y / FrameHeight);
	return TPair<int, int>{Index, FrameId};
}

int ArxReplayFrameTrack::GetMaxNumFrames()const
{
	int MaxNum = 0;
	for (auto& Track : Tracks)
	{
		if (!Track.bNeedCompare)
			continue;
		MaxNum = FMath::Max(Track.Frames.Num(), MaxNum);
	}
	return MaxNum;
}


void ArxReplayFrameTrack::AddTrack(const FString& Name, const TArray<FReplayFrame>& Frames, bool bCmp)
{
	Tracks.Add({ Name, Frames,bCmp });

	HintBeginY = FrameBeginY + Tracks.Num() * FrameHeight + 10;
	RequiredHeight = HintBeginY + HintHeight + 10;
}

static const FTableRowStyle* GetStyle(int State)
{
	static const FSlateColorBrush Red(FLinearColor(0.1, 0, 0));
	static const FSlateColorBrush Green(FLinearColor(0, 0.1, 0));
	static const FSlateColorBrush Black(FLinearColor::Black);
	static const FSlateColorBrush Selected(FLinearColor::Gray);

	auto Get = [&](const FSlateColorBrush& Brush) {
		auto Style = FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row");
		Style.SetEvenRowBackgroundBrush(Brush);
		Style.SetOddRowBackgroundBrush(Brush);
		return Style;
	};

	switch (State)
	{
	case 1:
	{
		static const auto Style = Get(Red);
		return &Style;
	}
	case 2:
	{
		static const auto Style = Get(Green);
		return &Style;
	}
	default:
	{
		static const auto Style = Get(Black);
		return &Style;
	}

	}
}

void ArxReplayWindow::InitWorld(UWorld* InWorld)
{
	DummyWorld.Reset();
	DummyWorld = MakeShared<FDummyWorld>(InWorld);

	for (auto& Item : TextViews)
	{
		Item.Refresh({});
		Item.Widget->RequestListRefresh();
	}
}

void ArxReplayWindow::Construct(const FArguments&)
{
	static const auto DefaultPath = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), TEXT("Arx"));
	Workspace = DefaultPath;

	auto RefreshFileList = [this]() {
		auto& FileMgr = IFileManager::Get();
		FileSource.Reset();
		if (FPaths::DirectoryExists(Workspace))
		{
			FileMgr.IterateDirectory(*Workspace, [this](auto Name, bool bDir) {
				if (bDir)
				{
					auto DirName = FPaths::GetBaseFilename(Name);
					FileSource.Add(MakeShared<FString>(DirName));
				}
				return true;
				});
		}
		FileSource.StableSort([](auto& a, auto& b) {
			return *a > *b;
			});
		FileList->RequestListRefresh();
	};


	auto MakeSimulationEvent = [this](int Index) {
		return [this, Index]() {
			auto Reply = FReply::Handled();
			if (DummyWorld)
			{
				DummyWorld->World = MakeShared<ArxWorld>(DummyWorld->UnrealWorld);
				int FrameId = TextViews[Index].FrameId;
				auto PId = TextViews[Index].PlayerId;

				ReplayInfo* Replay = nullptr;
				for (auto& Item : LevelTrackSource)
				{
					if (Item->LevelName == SelectedLevelName)
					{
						Replay = Item.Get();
					}
				}

				if (!Replay)
					return Reply;

				ReplayInfo::PlayerInfo* PlayerInfo = nullptr;
				for (auto& Item : Replay->PlayerTrackSource)
				{
					if (Item.PId == PId)
					{
						PlayerInfo = &Item;
					}
				}

				if (!PlayerInfo)
					return Reply;



				int Begin = 0;
				for (auto& Frame : PlayerInfo->FrameSource)
				{
					if (Frame.Data.Num() == 0)
					{
						Begin++;
						continue;
					}
					ArxReader Reader(Frame.Data);
					DummyWorld->Serialize(Reader);
					break;
				}
				SimulationTrack.Reset();
				SimulationTrack.Reserve(FrameId + 1);
				SimulationTrack.SetNum(Begin , false);

				for (int i = Begin; i <= FrameId; ++i)
				{


					auto& Data = SimulationTrack.AddDefaulted_GetRef().Data;
					ArxWriter Writer(Data);
					DummyWorld->World->Serialize(Writer);

					auto Hash = FCrc::MemCrc32(Data.GetData(), Data.Num());
					int State = FReplayFrame::E_UNKNOWN;
					auto NextFrameId = i + 1;
					for (auto& Item : Replay->PlayerTrackSource)
					{
						if (Item.FrameSource.IsValidIndex(i))
						{
							auto& Frame = Item.FrameSource[i];
							if (Frame.Hash == 0)
								continue;
							else if (Frame.Hash == Hash)
							{
								State = Frame.State;
								break;
							}
							else
							{
								State = FReplayFrame::E_ERROR;
							}
						}
					}
					SimulationTrack[i].State = State;
					SimulationTrack[i].FrameId = i;

					auto& ComSys = DummyWorld->World->GetSystem<ArxCommandSystem>();
					if (i < Replay->Commands.Num())
						ComSys.ReceiveCommands(&Replay->Commands[i].Data);

					DummyWorld->World->Update();
				}

			}
			return Reply;
		};
	};


	auto MakeStepEvent = [this](int Index) {
		return [this, Index]() {
			auto Reply = FReply::Handled();
			if (DummyWorld)
			{
				DummyWorld->World = MakeShared<ArxWorld>(DummyWorld->UnrealWorld);
				int FrameId = TextViews[Index].FrameId;
				auto PId = TextViews[Index].PlayerId;

				ReplayInfo* Replay = nullptr;
				for (auto& Item : LevelTrackSource)
				{
					if (Item->LevelName == SelectedLevelName)
					{
						Replay = Item.Get();
					}
				}

				if (!Replay)
					return Reply;

				TArray<FReplayFrame>* Frames = nullptr;
				for (auto& Item : Replay->PlayerTrackSource)
				{
					if (Item.PId == PId)
					{
						Frames = &Item.FrameSource;
					}
				}

				if (PId == -1)
				{
					Frames = &Replay->Commands;
				}

				if (!Frames)
					return Reply;

				auto NexrtFrameId = FrameId + 1;
				if (SimulationTrack.Num() <= NexrtFrameId)
				{
					SimulationTrack.AddDefaulted(NexrtFrameId - SimulationTrack.Num() + 1);
				}
				SimulationTrack[NexrtFrameId].FrameId = NexrtFrameId;


				if ((*Frames)[FrameId].Data.Num() == 0)
				{
					return Reply;
				}


				ArxReader Reader((*Frames)[FrameId].Data);
				DummyWorld->Serialize(Reader);


				auto& ComSys = DummyWorld->World->GetSystem<ArxCommandSystem>();
				if (FrameId < Replay->Commands.Num())
					ComSys.ReceiveCommands(&Replay->Commands[FrameId].Data);

				DummyWorld->World->Update();


				auto& Data = SimulationTrack[NexrtFrameId].Data;
				Data.Reset();
				ArxWriter Writer(Data);
				DummyWorld->World->Serialize(Writer);

				auto Hash = FCrc::MemCrc32(Data.GetData(), Data.Num());
				int State = FReplayFrame::E_UNKNOWN;
				for (auto& Item : Replay->PlayerTrackSource)
				{
					if (Item.FrameSource.IsValidIndex(NexrtFrameId))
					{
						auto& Frame = Item.FrameSource[NexrtFrameId];
						if (Frame.Hash == 0)
							continue;
						else if (Frame.Hash == Hash)
						{
							State = Frame.State;
							break;
						}
						else
						{
							State = FReplayFrame::E_ERROR;
						}
					}
				}
				SimulationTrack[NexrtFrameId].State = State;
			}
			return Reply;
		};
	};


	auto MakeCopyEvent = [this](int Index) {
		return [this, Index]() {
			auto Reply = FReply::Handled();
			FString Content;
			for (auto Item : TextViews[Index].Source)
			{
				Content += Item->Content + "\n";
			}


			FPlatformApplicationMisc::ClipboardCopy(*Content);

			return Reply;
		};
	};


	auto MakeSaveEvent = [this](int Index) {
		return [this, Index]() {
			auto Reply = FReply::Handled();
#if WITH_EDITOR
			FString FilePath;
			if (!FDesktopPlatformModule::Get()->OpenDirectoryDialog(nullptr, TEXT("open directory"), DefaultPath, FilePath))
				return Reply;

			int FrameId = TextViews[Index].FrameId;
			auto PId = TextViews[Index].PlayerId;


			auto& FileMgr = IFileManager::Get();


			FilePath = FPaths::Combine(FilePath, FString::Printf(TEXT("level_%s_frame_%d_pid_%d"), *SelectedLevelName, FrameId, PId));
			auto OutputFile = FileMgr.CreateFileWriter(*FilePath, 0);
			check(OutputFile);

			auto& Data = TextViews[Index].SnapshotData;
			OutputFile->Serialize(Data.GetData(), Data.Num());

			delete OutputFile;
			UE_LOG(LogCore, Display, TEXT("save current frame to %s"), *FilePath);
#endif
			return Reply;
		};
	};



	static auto CustomStyle = FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row");
	static auto const CustomBrush = FSlateColorBrush(FLinearColor(0.427, 0.701, 0.003));
	CustomStyle.SetActiveBrush(CustomBrush);  // selected but not hovered
	CustomStyle.SetActiveHoveredBrush(CustomBrush); // selected and hovered


	auto MakeTextView = [StylePtr = &CustomStyle, this](FTextView& View) {
		// frame content
		SAssignNew(View.Widget, FTextView::ListView).ListItemsSource(&View.Source)
			.OnGenerateRow_Lambda([StylePtr, this](auto Item, auto& Tab) {
			return SNew(STableRow<FTextView::TextItem>, Tab).Style(GetStyle(Item->State))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0)
				[
					// line number
					SNew(SBox).WidthOverride(50).HAlign(EHorizontalAlignment::HAlign_Right)
					[
						SNew(STextBlock).ColorAndOpacity(FLinearColor::White * 0.5).Text(FText::FromString(LexToString(Item->Index)))
					]
				]
			+ SHorizontalBox::Slot()
				[
					// text line
					SNew(STextBlock)
					.Text(FText::FromString(Item->Content))
				]
				];
				})
			.OnMouseButtonClick_Lambda([&, this](auto Item) {
					if (&TextViews[0] == &View)
					{
						if (TextViews[1].Source.IsValidIndex(Item->Index))
							TextViews[1].Widget->SetSelection(TextViews[1].Source[Item->Index]);
						else
							TextViews[1].Widget->ClearSelection();
					}
					else
					{
						if (TextViews[0].Source.IsValidIndex(Item->Index))
							TextViews[0].Widget->SetSelection(TextViews[0].Source[Item->Index]);
						else
							TextViews[0].Widget->ClearSelection();
					}

				});

				return View.Widget.ToSharedRef();
	};

	auto MakeText = [this](int Index){
		return 
		[this, Index](){
			int FrameId = TextViews[Index].FrameId;
			auto PId = TextViews[Index].PlayerId;

			ReplayInfo* Replay = nullptr;
			for (auto& Item : LevelTrackSource)
			{
				if (Item->LevelName == SelectedLevelName)
				{
					Replay = Item.Get();
				}
			}

			if (!Replay)
				return FText();

			TArray<FReplayFrame>* Frames = nullptr;
			for (auto& Item : Replay->PlayerTrackSource)
			{
				if (Item.PId == PId)
				{
					Frames = &Item.FrameSource;
				}
			}

			if (PId == -1)
			{
				Frames = &Replay->Commands;
			}

			if (!Frames || !Frames->IsValidIndex(FrameId))
				return FText();

		
			return FText::FromString(FString::Printf(TEXT("%x"), (*Frames)[FrameId].Hash));
		};
	};
	ChildSlot
		[
			SNew(SBorder)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
		[
			SNew(SVerticalBox).Clipping(EWidgetClipping::ClipToBounds)
			+ SVerticalBox::Slot().AutoHeight()
		[
			// tool bar
			SNew(SHorizontalBox)
#if WITH_EDITOR
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton).Text(FText::FromString(TEXT("Open Directory"))).OnClicked(FOnClicked::CreateLambda([this, RefreshFileList]() {
		auto Reply = FReply::Handled();
		if (!FDesktopPlatformModule::Get()->OpenDirectoryDialog(nullptr, TEXT("open directory"), DefaultPath, Workspace))
			return Reply;
		RefreshFileList();

		return Reply;
				}))
		]
#endif
	+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton).Text(FText::FromString(TEXT("Refresh"))).OnClicked(FOnClicked::CreateLambda([this, RefreshFileList]() {
		auto Reply = FReply::Handled();

		RefreshFileList();

		return Reply;
				}))
		]

		]
#if WITH_EDITOR
	+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SButton)
			.HAlign(EHorizontalAlignment::HAlign_Center)
		.ButtonColorAndOpacity_Lambda([this]() {
		if (!DummyWorld)
			return FLinearColor::Red;
		else
		{
			auto PathName = DummyWorld->UnrealWorld->GetOutermost()->GetPathName();
			auto LevelName = FPaths::GetBaseFilename(PathName);
			if (LevelName == SelectedLevelName)
				return FLinearColor::Green;
			else
				return FLinearColor::Yellow;
		}


			})
		.OnClicked(FOnClicked::CreateLambda([this]() {
				auto Reply = FReply::Handled();

				const FVector2D AssetPickerSize(600.0f, 586.0f);
				auto ActualWidget = SNew(ArxReplayOpenAssetDialog, AssetPickerSize, this);

				auto Parent = FSlateApplication::Get().GetActiveTopLevelWindow();
				auto Window = SNew(SWindow).MinWidth(586.0f).MinHeight(600.0f)
					[
						ActualWidget
					];

				ActualWidget->OnSelected = [this, Window](UWorld* Package) {
					InitWorld(Package);
					FSlateApplication::Get().RequestDestroyWindow(Window);
				};

				FSlateApplication::Get().AddModalWindow(Window, Parent);

				return Reply;
			}))
				.Text_Lambda([this]() {
				if (!DummyWorld)
					return FText::FromString(TEXT("Select Map"));
				else
					return FText::FromString(DummyWorld->UnrealWorld->GetName());

					})
		]
#endif
	+ SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().Padding(2, 2, 10, 2).AutoWidth()
		[
			// file list 
			SAssignNew(FileList, SListView<FileItem>).ListItemsSource(&FileSource)
			.OnGenerateRow_Lambda([](auto Item, auto& Tab) {
		return SNew(STableRow<FileItem>, Tab).Padding(2.0f)
			[
				SNew(STextBlock).Text(FText::FromString(*Item))
			];
				})
		.OnMouseButtonClick_Lambda([this](auto Item) {
					auto ContentPath = FPaths::Combine(Workspace, *Item);
					SetContent(ContentPath);
			})
		]
	+ SHorizontalBox::Slot().Padding(2, 2, 10, 2).AutoWidth()
		[
			// level list
			SAssignNew(LevelList, SListView<LevelTrack>).ListItemsSource(&LevelTrackSource)
			.OnGenerateRow_Lambda([this](auto Item, auto& Tab) {
		return SNew(STableRow<LevelTrack>, Tab).Padding(2.0f)
			[
				SNew(STextBlock).Text(FText::FromString(Item->LevelName)) // level name
			];
				})
		.OnMouseButtonClick_Lambda([this](auto Item) {
					//InitWorld();
					if (!DummyWorld)
						InitWorld();

					SelectedLevelName = Item->LevelName;
					SimulationTrack.Reset();
					TrackWidget->Reset();
					TrackWidget->AddTrack(TEXT("Commands"), Item->Commands, false);

					for (auto& Player : Item->PlayerTrackSource)
					{
						TrackWidget->AddTrack(LexToString(Player.PId), Player.FrameSource, true);
					}

					TrackWidget->AddTrack(TEXT("Simulation"), SimulationTrack, false);
			})
		]
	+ SHorizontalBox::Slot().Padding(2)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
		[
			// frame track
			SNew(SBox)
			.HeightOverride_Lambda([this]()->FOptionalSize {
		if (TrackWidget)
			return TrackWidget->GetRequiredHeight();
		return 200.0f;
				})
		[
			SAssignNew(TrackWidget, ArxReplayFrameTrack)
		]
		]
	+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
		[
			// frame tool
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[SNew(STextBlock).Text_Lambda(MakeText(0))]
			+ SHorizontalBox::Slot().AutoWidth()
		[SNew(SButton).Text(FText::FromString("Simulate")).OnClicked_Lambda(MakeSimulationEvent(0))]
	+ SHorizontalBox::Slot().AutoWidth()
		[SNew(SButton).Text(FText::FromString("Step")).OnClicked_Lambda(MakeStepEvent(0))]
	+ SHorizontalBox::Slot().AutoWidth()
		[SNew(SButton).Text(FText::FromString("Copy")).OnClicked_Lambda(MakeCopyEvent(0))]
	+ SHorizontalBox::Slot().AutoWidth()
		[SNew(SButton).Text(FText::FromString("Save as Binary")).OnClicked_Lambda(MakeSaveEvent(0))]

		]
	+ SHorizontalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
		[SNew(STextBlock).Text_Lambda(MakeText(1))]
			+ SHorizontalBox::Slot().AutoWidth()
		[SNew(SButton).Text(FText::FromString("Simulate")).OnClicked_Lambda(MakeSimulationEvent(1))]
	+ SHorizontalBox::Slot().AutoWidth()
		[SNew(SButton).Text(FText::FromString("Step")).OnClicked_Lambda(MakeStepEvent(0))]
	+ SHorizontalBox::Slot().AutoWidth()
		[SNew(SButton).Text(FText::FromString("Copy")).OnClicked_Lambda(MakeCopyEvent(1))]
	+ SHorizontalBox::Slot().AutoWidth()
		[SNew(SButton).Text(FText::FromString("Save as Binary")).OnClicked_Lambda(MakeSaveEvent(1))]
		]
		]
	+ SVerticalBox::Slot()
		[
			// frame viewr
			SNew(SBorder)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.5)
		[
			MakeTextView(TextViews[0])
		]
	+ SHorizontalBox::Slot().FillWidth(0.5)
		[
			MakeTextView(TextViews[1])
		]
		]
			]
		]

		]

		]
		]
		]
			]
		];

	TrackWidget->SetFrameListener([this](auto Getter, auto& Name, auto& Frame, int Index) {

		if (Name == TEXT("Commands"))
		{
			ArxCommandSystem Sys(*DummyWorld->World, 1);
			Sys.Initialize(false);
			auto Content = Sys.DumpCommands(Frame.Data);
			TArray<uint8> Data;
			ArxDebugSerializer DebugView(Data);
			DebugView << Content;
			TextViews[Index].Refresh(Data);
			TextViews[Index].Widget->RequestListRefresh();

			TextViews[Index].FrameId = Frame.FrameId;
		}
		else
		{
			TArray<uint8> Data;
			if (Frame.Data.Num() > 0)
			{
				ArxReader Reader(Frame.Data);
				DummyWorld->Serialize(Reader);

				ArxDebugSerializer DebugView(Data);
				DummyWorld->Serialize(DebugView);
			}
			TextViews[Index].Refresh(Data);
			TextViews[Index].SnapshotData = Data;
			TextViews[Index].FrameId = Frame.FrameId;
			TextViews[Index].PlayerId = NON_PLAYER_CONTROL;
			if (FCString::IsNumeric(*Name))
				LexFromString(TextViews[Index].PlayerId, *Name);

			auto StdFrame = Getter(Frame.FrameId + 1, [](auto& Name, auto& Frame) {
				return Name != TEXT("Commands") && Frame.State == FReplayFrame::E_NORMAL;
				});


			auto Cmds = Getter(Frame.FrameId, [](auto& Name, auto& Frame) {
				return Name == TEXT("Commands");
				});


			auto ResetItem = [](auto& Item, auto State) {
				if (Item->State == State)
					return;

				Item = MakeShared<FTextView::FLine>(MoveTemp(*Item));
				Item->State = State;
			};

			for (int i = 0; ; ++i)
			{
				if (i >= TextViews[0].Source.Num())
				{
					for (int j = i; j < TextViews[1].Source.Num(); ++j)
					{
						TextViews[1].Source[j]->State = FTextView::FLine::E_DIFF;
					}
					break;
				}
				else if (i >= TextViews[1].Source.Num())
				{
					for (int j = i; j < TextViews[1].Source.Num(); ++j)
					{
						TextViews[0].Source[j]->State = FTextView::FLine::E_DIFF;
					}
					break;
				}

				auto& Item0 = TextViews[0].Source[i];
				auto& Item1 = TextViews[1].Source[i];

				if (Item0->Content != Item1->Content)
				{
					ResetItem(Item0, FTextView::FLine::E_DIFF);
					ResetItem(Item1, FTextView::FLine::E_DIFF);
				}
				else
				{
					ResetItem(Item0, FTextView::FLine::E_SAME);
					ResetItem(Item1, FTextView::FLine::E_SAME);
				}
			}

			for (auto& Item : TextViews)
			{
				Item.Widget->RequestListRefresh();
			}
		}
		});

	RefreshFileList();

}
#pragma optimize("",on)

void ArxReplayWindow::SetContent(const FString& ParentPath)
{
	SimulationTrack.Reset();
	TrackWidget->Reset();


	auto& FileMgr = IFileManager::Get();
	LevelTrackSource.Reset();
	FileMgr.IterateDirectory(*ParentPath, [&](auto Name, bool bDir) {
		if (!bDir)
			return true;
		auto DirName = FPaths::GetBaseFilename(Name);

		auto& Track = LevelTrackSource.Add_GetRef(MakeShared<ReplayInfo>());
		Track->LevelName = DirName;


		// get commands
		auto CmdFile = FileMgr.CreateFileReader(*FPaths::Combine(Name, TEXT("commands")), FILEREAD_AllowWrite);
		if (CmdFile)
		{

			while (!CmdFile->AtEnd())
			{
				int FrameId, Len;
				(*CmdFile) << FrameId << Len;

				TArray<uint8> Data;
				Data.SetNumUninitialized(Len);
				CmdFile->Serialize((void*)Data.GetData(), Len);

				while (Track->Commands.Num() < FrameId)
				{
					Track->Commands.AddDefaulted();
				}

				auto& Frame = Track->Commands.AddDefaulted_GetRef();
				Frame.Data = MoveTemp(Data);
				Frame.FrameId = FrameId;
				Frame.State = Frame.Data.Num() == 0 ? FReplayFrame::E_UNKNOWN : FReplayFrame::E_NORMAL;
			}
			delete CmdFile;


		}


		// get snapshots
		auto SnapshotPath = FPaths::Combine(Name, TEXT("Snapshots/Remote"));

		FileMgr.IterateDirectory(*SnapshotPath, [&](auto Name, bool bDir) {
			if (bDir)
				return true;
			auto PlayerIdStr = FPaths::GetBaseFilename(Name);

			auto& Player = Track->PlayerTrackSource.AddDefaulted_GetRef();

			Player.PId = NON_PLAYER_CONTROL;
			Player.bCompared = true;
			LexFromString(Player.PId, *PlayerIdStr);
			auto File = TSharedPtr<FArchive>(FileMgr.CreateFileReader(Name, FILEREAD_AllowWrite));
			check(File)
				auto Total = File->TotalSize();
			if (Total <= 8) // id + num
				return true;

			while (!File->AtEnd())
			{
				int FrameId, Num;
				bool bDiscard;
				uint32 Hash;
				*File << FrameId << Hash << bDiscard << Num;
				TArray<uint8> Data;
				Data.SetNumUninitialized(Num);
				File->Serialize(Data.GetData(), Num);
				check(FrameId >= Player.FrameSource.Num())
					while (FrameId - 1 >= Player.FrameSource.Num())
					{
						auto& Frame = Player.FrameSource.AddDefaulted_GetRef();
						Frame.FrameId = Player.FrameSource.Num() - 1;
					}
				auto& Frame = Player.FrameSource.AddDefaulted_GetRef();
				Frame.FrameId = FrameId;
				Frame.Data = MoveTemp(Data);

				check(Hash == FCrc::MemCrc32(Frame.Data.GetData(),Frame.Data.Num()));

				Frame.Hash = Hash;
			}
			return true;
			});

		for (int FrameId = 0; ; FrameId++)
		{
			TMap<uint32, int> CompareList;
			bool bBreak = true;
			for (auto& PlayerTrack : Track->PlayerTrackSource)
			{
				if (FrameId >= PlayerTrack.FrameSource.Num())
					continue;
				bBreak = false;
				auto& Frame = PlayerTrack.FrameSource[FrameId];
				CompareList.FindOrAdd(Frame.Hash)++;
			}

			if (bBreak)
				break;

			uint32 StdHash = 0;
			int HashCount = 0;
			for (auto& Item : CompareList)
			{
				if (Item.Value > HashCount && Item.Key != 0)
				{
					HashCount = Item.Value;
					StdHash = Item.Key;
				}
			}


			for (auto& PlayerTrack : Track->PlayerTrackSource)
			{
				if (FrameId >= PlayerTrack.FrameSource.Num())
					continue;
				auto& Frame = PlayerTrack.FrameSource[FrameId];
				if (Frame.Hash == 0)
				{
					Frame.State = FReplayFrame::E_UNKNOWN;
				}
				else if (StdHash == Frame.Hash)
				{
					Frame.State = FReplayFrame::E_NORMAL;
				}
				else
				{
					Frame.State = FReplayFrame::E_DIFF;
				}

			}

		}

		return true;
		});

	LevelList->RequestListRefresh();
}



ArxReplayWindow::FDummyWorld::FDummyWorld(UWorld* InWorld)
{
	Reset();
	if (InWorld)
	{
		UnrealWorld = InWorld;
		if (InWorld->WorldType == EWorldType::Inactive)
		{
			UnrealWorld->WorldType = EWorldType::Editor;
			UnrealWorld->ClearWorldComponents();
			UnrealWorld->InitializeSubsystems();
			bNeedDestroy = true;
		}
#if ENGINE_MAJOR_VERSION >= 5
		else if (!InWorld->HasEverBeenInitialized())
		{
			InWorld->InitWorld();
			bNeedDestroy = true;
		}
#endif
	}
	else
	{
		FWorldContext* WorldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport);
		auto CurWorld = WorldContext->World();
		if (CurWorld->WorldType == EWorldType::Game)
			UnrealWorld = CurWorld;
		else
		{
			UnrealWorld = UWorld::CreateWorld(EWorldType::Editor, false, TEXT("ArxDummyWorld"), GetTransientPackage(), false);
			bNeedDestroy = true;
		}

	}
	check(UnrealWorld);
	World = MakeShared<ArxWorld>(UnrealWorld);
}

void ArxReplayWindow::FDummyWorld::Reset()
{
	World.Reset();
	if (UnrealWorld)
	{
		if (bNeedDestroy)
			UnrealWorld->DestroyWorld(false);
		UnrealWorld = nullptr;
	}
}

ArxReplayWindow::FDummyWorld::~FDummyWorld()
{
	Reset();
}

void ArxReplayWindow::FDummyWorld::Serialize(ArxSerializer& Ser)
{
	World->Serialize(Ser);
}

void ArxReplayWindow::FDummyWorld::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(UnrealWorld);
}



void ArxReplayWindow::FTextView::Refresh(const TArray<uint8>& Data)
{
	Source.Reset();

	if (Data.Num() > 0)
	{
		TCHAR* Begin = (TCHAR*)Data.GetData();
		TCHAR* End = (TCHAR*)(Data.GetData() + Data.Num());

		FString Content(Data.Num() / sizeof(TCHAR), (TCHAR*)Data.GetData());
		TArray<FString> Lines;
		Content.ParseIntoArrayLines(Lines, false);
		Source.Reserve(Lines.Num());

		int Index = 0;
		for (auto& Line : Lines)
		{
			FTextView::FLine Item = { MoveTemp(Line), 0,Index++ };
			Source.Add(MakeShared<FTextView::FLine>(MoveTemp(Item)));
		}
	}
}



void ArxReplayOpenAssetDialog::Construct(const FArguments& InArgs, FVector2D InSize, ArxReplayWindow* InParent)
{
#if WITH_EDITOR
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.OnAssetDoubleClicked = FOnAssetSelected::CreateSP(this, &ArxReplayOpenAssetDialog::OnAssetSelectedFromPicker);
	AssetPickerConfig.OnAssetEnterPressed = FOnAssetEnterPressed::CreateSP(this, &ArxReplayOpenAssetDialog::OnPressedEnterOnAssetsInPicker);
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.bShowBottomToolbar = true;
	AssetPickerConfig.bAutohideSearchBar = false;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.SaveSettingsName = TEXT("AssetPicker");
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1 // 5.1
	AssetPickerConfig.Filter.ClassPaths.Add(UWorld::StaticClass()->GetClassPathName());
#else
	AssetPickerConfig.Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
#endif
	ChildSlot
		[
			SNew(SBox)
			.WidthOverride(InSize.X)
		.HeightOverride(InSize.Y)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		]
		]
		];
#endif
}


FReply ArxReplayOpenAssetDialog::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		FSlateApplication::Get().DismissAllMenus();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void ArxReplayOpenAssetDialog::OnAssetSelectedFromPicker(const FAssetData& AssetData)
{
#if ENGINE_MAJOR_VERSION >=5 
	UWorld::WorldTypePreLoadMap.Add(AssetData.PackageName, EWorldType::Editor);
#endif
	auto World = Cast<UWorld>(AssetData.GetAsset());
	check(World);
	if (OnSelected)
		OnSelected(World);
}

void ArxReplayOpenAssetDialog::OnPressedEnterOnAssetsInPicker(const TArray<FAssetData>& SelectedAssets)
{
	for (auto& AssetData : SelectedAssets)
	{
		OnAssetSelectedFromPicker(AssetData);

		return;
	}
}


#undef LOCTEXT_NAMESPACE