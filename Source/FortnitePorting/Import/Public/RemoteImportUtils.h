#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "RemoteImportUtils.generated.h"


UCLASS()
class URemoteImportUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, meta=(ScriptName="ImportResponseRemoteAPI"))
	static void ImportResponseRemoteAPI(const FString& Response);

};
