#include "FFortnitePortingUtils.h"

#include "FortnitePorting.h"
#include "json/single_include/nlohmann/json.hpp"
using json = nlohmann::json;

void FFortnitePortingUtils::ImportResponse(FString Response)
{
	auto ResponseData = json::parse(std::string(TCHAR_TO_ANSI(*Response)));

	auto AssetsRoot = ResponseData["AssetsRoot"];
	auto Settings = ResponseData["Settings"];
	auto Data = ResponseData["Data"];

	for (auto [Index, ImportData] : Data.items())
	{
		UE_LOG(LogFortnitePorting, Log, TEXT("Importing %s (%s)"), *FFortnitePortingUtils::GetString(ImportData, "Name"), *FFortnitePortingUtils::GetString(ImportData, "Type"))
	}
}

FString FFortnitePortingUtils::GetString(nlohmann::basic_json<>& Data, std::string Name)
{
	return FString(Data[Name].get<std::string>().c_str());
}
