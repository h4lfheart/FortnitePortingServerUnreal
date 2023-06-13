#pragma once
#include "ExportModel.h"
#include "Factories/TextureFactory.h"

class FImportUtils
{
public:
	static void CheckForDependencies();
	static UMaterial* GetMaterial();
	static void ImportResponse(const FString& Response);
	static auto SplitExportPath(const FString& InStr);
	static UObject* ImportMesh(const FExportMesh& Mesh);
	static void ImportMaterial(const FExportMaterial& Material);
	static UTexture* ImportTexture(const FTextureParameter& Texture);

	inline static FExport CurrentExport = FExport();
	
	inline static UMaterial* DefaultMaterial;
	inline static UMaterial* UEFNMaterial;
	inline static UTextureFactory* TextureFactory;
	
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
