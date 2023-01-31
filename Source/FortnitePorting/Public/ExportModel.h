#pragma once

#include "ExportModel.generated.h"

USTRUCT()
struct FExportMesh
{
	GENERATED_BODY()

	UPROPERTY()
	FString MeshPath;
	
	UPROPERTY()
	int NumLods;
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