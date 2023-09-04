// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SpyVsSpy : ModuleRules
{
	public SpyVsSpy(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "GeometryCore", "GeometryFramework", "GeometryScriptingCore", "GeometryScriptingEditor", "DynamicMeshRoomGen", "HeadMountedDisplay", "EnhancedInput", "NetCore" });
	}
}
