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
};

USTRUCT()
struct FExportMaterial
{
	GENERATED_BODY()

	// BASE
	UPROPERTY()
	FString MaterialPath; // UNREAL ONLY
	
	UPROPERTY()
	FString MaterialName;

	UPROPERTY()
	FString MasterMaterialName;

	UPROPERTY()
	int SlotIndex;
	
	UPROPERTY()
	int Hash; // Not Needed for UE

	UPROPERTY()
	TArray<FTextureParameter> Textures;

	UPROPERTY()
	TArray<FScalarParameter> Scalars;
	
	UPROPERTY()
	TArray<FVectorParameter> Vectors;

	// OVERRIDE
	UPROPERTY()
	FString MaterialNameToSwap;
};

USTRUCT()
struct FExportMesh
{
	GENERATED_BODY()

	// BASE
	UPROPERTY()
	FString MeshPath;
	
	UPROPERTY()
	int NumLods;

	// PART
	UPROPERTY()
	FString Part;

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
	
	// OVERRIDE
	UPROPERTY()
	FString MeshToSwap;
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
	TArray<FExportMesh> Parts;

	UPROPERTY()
	TArray<FExportMesh> StyleParts;

	
};

USTRUCT()
struct FExportSettings
{
	GENERATED_BODY()

	// TODO
};

USTRUCT()
struct FExport
{
	GENERATED_BODY()
	
	UPROPERTY()
	FString AssetsRoot;

	UPROPERTY()
	TArray<FExportData> Data;

	UPROPERTY()
	FExportSettings Settings;
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