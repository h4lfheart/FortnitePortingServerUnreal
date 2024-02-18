#pragma once
#include "ExportModel.h"
#include "Engine/SCS_Node.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AssetImportTask.h"


class USceneComponent;
class USCS_Node;
class UAssetImportTask;
class UFactory;

class FImportUtils
{
public:
	inline static const FString IMPORT_ROOT_FOLDER = "FortniteGame";
	static void CheckForDependencies();
	static UMaterial* GetMaterial();
	static void ImportResponse(const FString& Response);
	static auto SplitExportPath(const FString& InStr);
	static FString WrapPathWithImportRootFolder(const FString& Folder);
	static UObject* ImportMesh(const FExportMesh& Mesh);
	static void ImportMaterial(const FExportMaterial& Material);
	static UTexture* ImportTexture(const FTextureParameter& Texture);
	
	static UBlueprint* CreateActorBlueprint(FString ActorBlueprintAssetPath);
	static void GenerateActorComponents(UBlueprint* ActorBlueprint, TMap<FString, FString> StaticMeshAssetPaths, TMap<FString, FString> SkeletalMeshAssetPaths);

	inline static FExport CurrentExport = FExport();
	
	inline static UMaterial* DefaultMaterial;
	inline static UMaterial* UEFNMaterial;

	static UAssetImportTask* CreateImportTask(FString SourcePath, FString DestinationPath, UFactory* ExtraFactory, UObject* ExtraOptions);
	static UObject* ProcessImportTask(UAssetImportTask* ImportTask);
	static UObject* ImportAsset(FString SourcePath, FString DestinationPath);

	inline static TMap<FString, FString> TextureMappings = {
		{"Diffuse", "Diffuse"},
		{"PM_Diffuse", "Diffuse"},
		{"PetalDetailMap", "Diffuse"},
		
		{"SpecularMasks", "SpecularMasks"},
		{"PM_SpecularMasks", "SpecularMasks"},
		{"Specular Mask", "SpecularMasks"},
		{"SpecMap", "SpecularMasks"},
			
		{"Normals", "Normals"},
		{"PM_Normals", "Normals"},
		{"Normal", "Normals"},
		{"NormalMap", "Normals"},
		
		{"M", "M"},

		{"Emissive", "Emissive"},
		{"PM_Emissive", "Emissive"},
		{"EmissiveTexture", "Emissive"},
		{"Tank Emissive", "Emissive"},
	};
	
	inline static TMap<FString, FString> ScalarMappings = {
		{"emissive mult", "Emissive Brightness"},
		{"TH_StaticEmissiveMult", "Emissive Brightness"},
		{"Emissive", "Emissive Brightness"}
	};

	inline static TMap<FString, FString> SwitchMappings = {
		{"SwizzleRoughnessToGreen", "SwizzleRoughnessToGreen"}
	};
};
