// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SpyVsSpy : ModuleRules
{
	public SpyVsSpy(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"GeometryCore", 
			"GeometryFramework", 
			"GeometryScriptingCore", 
			"DynamicMeshRoomGen", 
			"HeadMountedDisplay", 
			"EnhancedInput", 
			"NetCore", 
			"Niagara"
		});
	}
}
