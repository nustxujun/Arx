#pragma once 

#include "CoreMinimal.h"

#include "ArxCommon.h"
#include "Widgets/SWidget.h"

#include "ArxReplayWindow.generated.h"


struct FReplayFrame
{
    enum
    {
        E_UNKNOWN = 0,
        E_NORMAL = 1,
        E_DIFF,
        E_ERROR
    };

    int State = 0;
    int FrameId = -1;
    uint32 Hash = 0;
    TArray<uint8> Data;
};

class ArxReplayFrameTrack : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(ArxReplayFrameTrack)
    {}


    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void Reset();


    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

    virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

    void AddTrack(const FString& Name, TArray<FReplayFrame> Frames, bool bCmp);
    float GetRequiredHeight(){return FMath::Max(50.0f, RequiredHeight);}

    using FrameListener = TFunction<void(const TArray<TPair<FString, const FReplayFrame&>>&, const FString&, const FReplayFrame&, int)>;
    void SetFrameListener(FrameListener  Callback){ OnFrameChanged  = MoveTemp(Callback); };
private:
    TPair<int, int> GetFrameByPos(float X, float Y)const ;

private:
    FGeometry ThisGeometry;
    struct Track
    {
        FString Name;
        TArray<FReplayFrame> Frames;
        bool bNeedCompare;
    };
    TArray<Track> Tracks;
    TPair<int, int> Selecteds[2] = {TPair<int, int>{-1,-1},TPair<int, int>{-1,-1} };

    FVector2D MousePosition;
    FVector2D PressedMousePosition;
    FrameListener OnFrameChanged;

    float BeginX = 0;
    //int BeginFrame = 0;

    const float TextHeight = 20;
    const float FrameBeginX = 70;
    const float FrameBeginY = 50;
    const float FrameWidth = 10;
    const float FrameHeight = 20;
    const float HintHeight = 5;

    float ViewWidth = 0;
    float ViewHeight = 0;
    float RequiredHeight = 0;
    float HintBeginY = 0;
    //float Scale = 1.0f;
    int Freq = 1;
    

    bool bIsViewportDirty = true;
    bool bIsStateDirty = true;
    bool bMouseIsMoving = false;


};

class ARXREPLAY_API ArxReplayWindow: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(ArxReplayWindow)
    {}


	SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    void SetContent(const FString& ParentPath);

private:
    using FileItem = TSharedPtr<FString>;
    TSharedPtr<SListView<FileItem>> FileList;
    TArray<FileItem> FileSource;



    using FrameTrack = TSharedPtr<FString>;
    

    struct ReplayInfo
    {
        FString LevelName;

        struct PlayerInfo
        {
            ArxPlayerId PId;

            using FrameItem = FReplayFrame;
            TArray<FrameItem> FrameSource;
            bool bCompared = false;
        };

        using PlayerItem = PlayerInfo;
        TArray<PlayerItem> PlayerTrackSource;
        TArray<FReplayFrame> Commands;
    };
    using LevelTrack = TSharedPtr<ReplayInfo>;
    TArray<LevelTrack> LevelTrackSource;
    TSharedPtr<SListView<LevelTrack>> LevelList;
    TSharedPtr<ArxReplayFrameTrack> TrackWidght;
    

    struct FDummyWorld : public FGCObject
    {
        UWorld* UnrealWorld = nullptr;
        TSharedPtr<ArxWorld> World ;
        FDummyWorld();
        ~FDummyWorld();

        void AddReferencedObjects(FReferenceCollector& Collector) override;
        virtual FString GetReferencerName() const override
        {
            return "DummyWorld";
        }


        void Serialize(ArxSerializer& Ser);
    };
    TSharedPtr<FDummyWorld> DummyWorld;

    struct FTextView
    {
        struct FLine
        {
            enum
            {
                E_SAME = 0,
                E_DIFF = 1,
            };
            FString Content;
            int State;
            int Index;
            bool operator == (const FLine& Other)const
            {
                return Index == Other.Index;
            }
        };

        using TextItem = TSharedPtr<FLine>;
        using ListView = SListView<TextItem>;

        TArray<TextItem> Source;
        TSharedPtr<SListView<TextItem>> Widget;
        TArray<uint8> Snapshot;
        TArray<uint8> Commands;
        int FrameId;

        void Refresh(const TArray<uint8>& Data);
    }TextViews[2];

};

UCLASS()
class UArxReplayEditor : public UEngineSubsystem
{
    GENERATED_BODY()
public:
    static UArxReplayEditor& Get();

    void Initialize(FSubsystemCollectionBase& Collection);
    void Deinitialize();
    bool ShouldCreateSubsystem(UObject* Outer) const override;

private:
    TSharedPtr<ArxReplayWindow> ReplayWindow;
    TSharedPtr<ArxWorld> World;
};

