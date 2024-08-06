#pragma once 

#include "CoreMinimal.h"

#include "ArxCommon.h"
#include "Widgets/SWidget.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"


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

    void AddTrack(const FString& Name,const TArray<FReplayFrame>& Frames, bool bCmp);
    float GetRequiredHeight(){return FMath::Max(50.0f, RequiredHeight);}

    using FrameListener = TFunction<void(TFunction<const FReplayFrame*( int, TFunction<bool(const FString&, const FReplayFrame& )>)> FrameGetter, const FString&, const FReplayFrame&, int)>;
    void SetFrameListener(FrameListener  Callback){ OnFrameChanged  = MoveTemp(Callback); };
private:
    TPair<int, int> GetFrameByPos(float X, float Y)const ;
    int GetMaxNumFrames()const;
    void ScrollTrack(const FVector2D& Pos);
private:
    FGeometry ThisGeometry;
    struct Track
    {
        FString Name;
        const TArray<FReplayFrame>& Frames;
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
    const float HintHeight = 10;

    float ViewWidth = 0;
    float ViewHeight = 0;
    float RequiredHeight = 0;
    float HintBeginY = 0;
    //float Scale = 1.0f;
    int Freq = 1;
    int Region = 0;

    bool bIsViewportDirty = true;
    bool bIsStateDirty = true;
    bool bMouseIsMoving = false;


};

class ARXREPLAY_API ArxReplayOpenAssetDialog: public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(ArxReplayOpenAssetDialog){}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, FVector2D InSize, class ArxReplayWindow*);

	// SWidget interface
	virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	// End of SWidget interface

protected:
	void OnAssetSelectedFromPicker(const struct FAssetData& AssetData);
	void OnPressedEnterOnAssetsInPicker(const TArray<struct FAssetData>& SelectedAssets);

public:
    TFunction<void(UWorld*)> OnSelected;
};


class ARXREPLAY_API ArxReplayWindow: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(ArxReplayWindow)
    {}


	SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    void SetContent(const FString& ParentPath);
 
    void InitWorld(UWorld* InWorld = nullptr);
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
    TSharedPtr<ArxReplayFrameTrack> TrackWidget;
    TArray<FReplayFrame> SimulationTrack;

    struct FDummyWorld : public FGCObject
    {
        UWorld* UnrealWorld = nullptr;
        TSharedPtr<ArxWorld> World ;
        bool bNeedDestroy = false;
        FDummyWorld(UWorld* InWorld);
        ~FDummyWorld();

        void AddReferencedObjects(FReferenceCollector& Collector) override;
        virtual FString GetReferencerName() const override
        {
            return "DummyWorld";
        }

        void Reset();
        void Serialize(ArxSerializer& Ser);
    };
    TSharedPtr<FDummyWorld> DummyWorld;
    FString SelectedLevelName;
    FString Workspace;
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
        int FrameId = 0;
        ArxPlayerId PlayerId = NON_PLAYER_CONTROL;
        TArray<uint8> SnapshotData;

        void Refresh(const TArray<uint8>& Data);
    }TextViews[2];

};

