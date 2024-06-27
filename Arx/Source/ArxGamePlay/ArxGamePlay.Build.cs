// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ArxGamePlay : ModuleRules
{
	public ArxGamePlay(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
            });


        PublicDependencyModuleNames.AddRange(
			new string[]	
			{
				"Core",
                "ArxRuntime",
                "Rp3dRuntime"
            }
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
			}
            );
		
		

	}
}
