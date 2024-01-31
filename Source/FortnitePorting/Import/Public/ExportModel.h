#pragma once

#include "ExportModel.generated.h"
USTRUCT()
struct FTextureParameter
{
	GENERATED_BODY()

	// BASE
	UPROPERTY()
	FString Name;
	
	UPROPERTY()
	FString Value;
	
	UPROPERTY()
	bool sRGB;
	
	UPROPERTY()
    TEnumAsByte<TextureCompressionSettings> CompressionSettings;

	FTextureParameter(){
		Name = "";
		Value = "";
		sRGB = true;
		CompressionSettings = TextureCompressionSettings::TC_Default;
	}

};

USTRUCT()
struct FScalarParameter
{
	GENERATED_BODY()

	// BASE
	UPROPERTY()
	FString Name;
	
	UPROPERTY()
	float Value;

	FScalarParameter(){
		Name = "";
		Value = 0.f;
	}
};

USTRUCT()
struct FVectorParameter
{
	GENERATED_BODY()

	// BASE
	UPROPERTY()
	FString Name;
	
	UPROPERTY()
	FLinearColor Value;

	FVectorParameter(){
		Name = "";
		Value = FLinearColor();
	}
};

USTRUCT()
struct FSwitchParameter
{
	GENERATED_BODY()

	// BASE
	UPROPERTY()
	FString Name;
	
	UPROPERTY()
	bool Value;

	FSwitchParameter(){
		Name = "";
		Value = false;
	}
};


USTRUCT()
struct FExportMaterial
{
	GENERATED_BODY()

	// BASE
	UPROPERTY()
	FString Path;
	
	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString AbsoluteParent;

	UPROPERTY()
	bool UseGlassMaterial;

	UPROPERTY()
	bool UseFoliageMaterial;
	
	UPROPERTY()
	int Slot; 
	
	UPROPERTY()
	int Hash;

	UPROPERTY()
	TArray<FTextureParameter> Textures;

	UPROPERTY()
	TArray<FScalarParameter> Scalars;
	
	UPROPERTY()
	TArray<FVectorParameter> Vectors;

	UPROPERTY()
	TArray<FSwitchParameter> Switches;

	FExportMaterial(){
		Path = "";
		Name = "";
		AbsoluteParent = "";

		Slot = 0;
		Hash = 0;
		UseGlassMaterial = false;
		UseFoliageMaterial = false;
	}
};

USTRUCT()
struct FExportMesh
{
	GENERATED_BODY()

	// BASE
	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Type;

	UPROPERTY()
	FString Path;
	
	UPROPERTY()
	int NumLods;

	// PART
	UPROPERTY()
	FString MorphName;

	UPROPERTY()
	FString SocketName;

	UPROPERTY()
	FString PoseAnimation;
	
	UPROPERTY()
	TArray<FString> PoseNames;
	
	UPROPERTY()
	TArray<FExportMaterial> Materials;
	
	UPROPERTY()
	TArray<FExportMaterial> OverrideMaterials;
	
	FExportMesh(){
		Name = "";
		Type = "";
		Path = "";

		NumLods = 0;

	}
};


USTRUCT()
struct FExportData
{
	GENERATED_BODY()

	// BASE
	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Type;

	// MESH
	UPROPERTY()
	TArray<FExportMesh> Meshes;

	FExportData(){
		Name = "";
		Type = "";
	}
};

USTRUCT()
struct FExportSettings
{
	GENERATED_BODY()

	UPROPERTY()
	bool ForUEFN;

	UPROPERTY()
	bool ImportMaterials;
	
	UPROPERTY()
	float AmbientOcclusion;
	
	UPROPERTY()
	float Cavity;
	
	UPROPERTY()
	float Subsurface;

	FExportSettings(){
		ForUEFN = false;
		ImportMaterials = true;
		AmbientOcclusion = 0.f;
		Cavity = 0.f;
		Subsurface = 0.0f;
	}
};

USTRUCT()
struct FExport
{
	GENERATED_BODY()
	
	UPROPERTY()
	FString AssetsFolder;

	UPROPERTY()
	TArray<FExportData> Data;

	UPROPERTY()
	FExportSettings Settings;

	FExport(){
		AssetsFolder = "";
	}
};

USTRUCT()
struct FPartData
{
	GENERATED_BODY()
	
	UPROPERTY()
	UObject* ImportedMesh;
	
	UPROPERTY()
	FExportMesh MeshData;

	FPartData() {}
	FPartData(UObject* ImportedMesh, const FExportMesh& ExportMesh) : ImportedMesh(ImportedMesh), MeshData(ExportMesh) {}
};