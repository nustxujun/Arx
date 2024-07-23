// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ArxReplay : ModuleRules
{
	public ArxReplay(ReadOnlyTargetRules Target) : base(Target)
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
            }
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
                "Slate",
				"SlateCore",
				"InputCore",

				"ApplicationCore",
				"EditorStyle",
			}
            );


        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(
				new string[] 
				{ 
					"UnrealEd",
					"ToolMenus",
					"Projects",
                });
        }


    }
}
