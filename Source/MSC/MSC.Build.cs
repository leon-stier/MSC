// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MSC : ModuleRules
{
	public MSC(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", "EnhancedInput", 
			"GameplayAbilities", "GameplayTags", "GameplayTasks", 
			"AIModule", "StateTreeModule", "GameplayStateTreeModule" 
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

	}
}
