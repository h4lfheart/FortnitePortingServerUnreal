#pragma once
#include "ExportModel.h"
#include "Materials/MaterialInstanceConstant.h"

class FUtils
{
public:
	static void ImportResponse(const FString& Response);
	static FString BytesToString(TArray<uint8>& Message, int BytesLength);
	static TArray<uint8> StringToBytes(const FString& InStr);
	static auto SplitExportPath(const FString& InStr);
	static UObject* ImportMesh(const FExportMesh& Mesh);
	static void ImportMaterial(const FExportMaterial& Material);
	static UTexture* ImportTexture(const FTextureParameter& Texture);

	inline static FExport CurrentExport = FExport();
};
