using UnrealBuildTool;

public class FortnitePorting : ModuleRules
{
	public FortnitePorting(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Engine",
				"Core", 
				"Networking", 
				"Json", 
				"JsonUtilities", 
				"UnrealPSKPSA", 
				"Engine", 
				"PluginUtils",
				"Projects",
				"UEFormat",
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Sockets",
				"Networking",
				"UnrealPSKPSA",
				"UnrealEd", 
				"EditorScriptingUtilities", 
				"PluginUtils",
				"PythonScriptPlugin",
				"AssetTools",
			}
		);
	}
}