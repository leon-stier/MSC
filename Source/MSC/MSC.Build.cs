// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MSC : ModuleRules
{
	public MSC(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange([
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", "EnhancedInput", 
			"GameplayAbilities", "GameplayTags", "GameplayTasks", 
			"AIModule", "StateTreeModule", "GameplayStateTreeModule", 
			"UMG", "SlateCore", "Slate",
			"Json", "JsonUtilities"
		]);

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

	}
}
