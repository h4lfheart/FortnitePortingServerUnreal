#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFortnitePorting, Log, All);

class FFortnitePortingModule : public IModuleInterface
{
public:
	
	FString HOST = "localhost";
	int PORT = 24281;

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
