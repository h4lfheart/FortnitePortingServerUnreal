#pragma once

#include "CoreMinimal.h"
#include "ExportModel.h"
#include "ListenServer.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Modules/ModuleManager.h"

class UTextureFactory;
DECLARE_LOG_CATEGORY_EXTERN(LogFortnitePorting, Log, All);

class FFortnitePortingModule : public IModuleInterface
{
public:
	FListenServer* ListenServer;
	static UMaterial* DefaultMaterial;

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};