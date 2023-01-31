#include "FFortnitePortingUtils.h"
#include "FortnitePorting.h"
#include "JsonObjectConverter.h"

void FFortnitePortingUtils::ImportResponse(const FString Response)
{
	FExport Export;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(Response, &Export))
	{
		UE_LOG(LogFortnitePorting, Error, TEXT("Unable to deserialize response from FortnitePorting."));
		return;
	}

	FString AssetsRoot = Export.AssetsRoot;
	FExportSettings Settings = Export.Settings;
	TArray<FExportDataBase> Data = Export.Data;

	for (auto [Name, Type] : Data)
	{
		UE_LOG(LogFortnitePorting, Log, TEXT("Importing %s (%s)"), *Name, *Type)
	}
}

FString FFortnitePortingUtils::BytesToString(TArray<uint8>& Message, const int BytesLength)
{
	if (BytesLength <= 0)
	{
		return FString("");
	}
	if (Message.Num() < BytesLength)
	{
		return FString("");
	}

	TArray<uint8> StringAsArray;
	StringAsArray.Reserve(BytesLength);

	for (int i = 0; i < BytesLength; i++)
	{
		StringAsArray.Add(Message[0]);
		Message.RemoveAt(0);
	}

	return FString(reinterpret_cast<const char*>(StringAsArray.GetData()), StringAsArray.Num());
}

TArray<uint8> FFortnitePortingUtils::StringToBytes(const FString& InStr)
{
	const FTCHARToUTF8 Convert(*InStr);
	const auto BytesLength = Convert.Length();
	const auto MessageBytes = static_cast<uint8*>(FMemory::Malloc(BytesLength));
	FMemory::Memcpy(MessageBytes, TCHAR_TO_UTF8(InStr.GetCharArray().GetData()), BytesLength);

	TArray<uint8> Result;
	for (auto i = 0; i < BytesLength; i++)
	{
		Result.Add(MessageBytes[i]);
	}

	FMemory::Free(MessageBytes);
	return Result;
}
