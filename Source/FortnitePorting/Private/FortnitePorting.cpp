#include "FortnitePorting.h"

#include "EditorAssetLibrary.h"
#include "ListenServer.h"
#include "Factories/TextureFactory.h"

#define LOCTEXT_NAMESPACE "FFortnitePortingModule"

DEFINE_LOG_CATEGORY(LogFortnitePorting);

UMaterial* FFortnitePortingModule::DefaultMaterial = nullptr;

void FFortnitePortingModule::StartupModule()
{
	UE_LOG(LogFortnitePorting, Log, TEXT("FortnitePorting Server Listening at localhost:%d"), 24281)
	ListenServer = new FListenServer();
}

void FFortnitePortingModule::ShutdownModule()
{
	UE_LOG(LogFortnitePorting, Log, TEXT("FortnitePorting Server Closed"))
	delete ListenServer;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFortnitePortingModule, FortnitePorting)
