#include "FortnitePorting.h"

#include "FListenServer.h"

#define LOCTEXT_NAMESPACE "FFortnitePortingModule"

DEFINE_LOG_CATEGORY(LogFortnitePorting);

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