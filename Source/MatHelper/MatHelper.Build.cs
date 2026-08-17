// Copyright AKaKLya 2024
// UE4.26 port: removed AssetDefinition/EngineAssetDefinitions (UE5-only).

using UnrealBuildTool;
using System.IO;

public class MatHelper : ModuleRules
{
	public MatHelper(ReadOnlyTargetRules Target) : base(Target)
	{
		// Get the engine path. Ends with "Engine/"
		string engine_path = Path.GetFullPath(Target.RelativeEnginePath);

		// Private include paths for engine module internals we access via template hack.
		string Material_path = engine_path + "Source/Editor/MaterialEditor/Private/";
		string Niagara_path = engine_path + "Plugins/FX/Niagara/Source/NiagaraEditor/Private/Sequencer/LevelSequence/";

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(Material_path);
		PublicIncludePaths.Add(Niagara_path);

		PrivateIncludePaths.Add(Material_path);
		PrivateIncludePaths.Add(Niagara_path);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"MaterialEditor",
				"GraphEditor",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"MaterialEditor",
				"Projects",
				"ApplicationCore",
				"InputCore",
				"ContentBrowser",
				"AssetTools",
				"GraphEditor",
				"EditorWidgets",
				"ToolMenus",
				"EditorStyle",
				"DeveloperSettings",
				"StaticMeshEditor",
				"LevelSequence",
				"NiagaraEditor",
				"Niagara",
				"Sequencer",
				"MovieScene",
				"SceneOutliner",
				"PropertyEditor",
				"RenderCore",
				"LevelEditor",
				"AssetRegistry",
				"Kismet",
				"WorkspaceMenuStructure",
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
