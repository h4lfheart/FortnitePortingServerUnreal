#pragma once

#include "CoreMinimal.h"
#include "FListenServer.h"
#include "Modules/ModuleManager.h"
#include "FortnitePorting.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFortnitePorting, Log, All);

class FFortnitePortingModule : public IModuleInterface
{
public:
	FListenServer* ListenServer;

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

USTRUCT()
struct FExportDataBase
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Type;
};

USTRUCT()
struct FExportSettings
{
	GENERATED_BODY()

	// TODO
};

USTRUCT()
struct FExport
{
	GENERATED_BODY()
	
	UPROPERTY()
	FString AssetsRoot;

	UPROPERTY()
	TArray<FExportDataBase> Data;

	UPROPERTY()
	FExportSettings Settings;
};