#include "FortnitePorting/Import/Public/RemoteImportUtils.h"
#include "FortnitePorting/Import/Public/ImportUtils.h"


#include "Editor/UnrealEd/Public/AssetImportTask.h" // UnrealEd (Editor Only)
#include "AssetToolsModule.h" // AssetTools (Editor Only)

void URemoteImportUtils::ImportResponseRemoteAPI(const FString& Response)
{
	FImportUtils::ImportResponse(Response);
}