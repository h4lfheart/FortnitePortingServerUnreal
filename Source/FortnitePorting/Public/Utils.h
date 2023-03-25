#pragma once
#include "ExportModel.h"

class FUtils
{
public:
	static void ImportResponse(FString Response);
	static FString BytesToString(TArray<uint8>& Message, int BytesLength);
	static TArray<uint8> StringToBytes(const FString& InStr);
	static void ImportMesh(FString Filepath);
	
	inline static FExport CurrentExport = FExport();
};
