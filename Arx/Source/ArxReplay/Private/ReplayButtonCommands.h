// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ReplayButtonStyle.h"

class FReplayButtonCommands : public TCommands<FReplayButtonCommands>
{
public:

	FReplayButtonCommands()
		: TCommands<FReplayButtonCommands>(TEXT("ArxReplay"), NSLOCTEXT("Contexts", "ArxReplay", "Arx Plugin"), NAME_None, FReplayButtonStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};