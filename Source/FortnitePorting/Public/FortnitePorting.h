#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class UTextureFactory;
DECLARE_LOG_CATEGORY_EXTERN(LogFortnitePorting, Log, All);

class FFortnitePortingModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};