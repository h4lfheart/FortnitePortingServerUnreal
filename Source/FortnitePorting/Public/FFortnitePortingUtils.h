#pragma once

class FFortnitePortingUtils
{
public:
	static void ImportResponse(FString Response);
	static FString BytesToString(TArray<uint8>& Message, int BytesLength);
	static TArray<uint8> StringToBytes(const FString& InStr);
};
