#pragma once

#include "json/single_include/nlohmann/json.hpp"

class FFortnitePortingUtils
{
public:
	static void ImportResponse(FString Response);
	static FString GetString(nlohmann::basic_json<>& Data, std::string Name);
};
