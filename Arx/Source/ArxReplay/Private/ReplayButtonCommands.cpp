// Copyright Epic Games, Inc. All Rights Reserved.

#include "ReplayButtonCommands.h"

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FReplayModule"

void FReplayButtonCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "Arx", "Open arx replay tool", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE

#endif