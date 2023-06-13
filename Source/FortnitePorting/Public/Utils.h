#pragma once

class FUtils
{
public:
	static FString BytesToString(TArray<uint8>& Message, int BytesLength);
	static TArray<uint8> StringToBytes(const FString& InStr);
};
