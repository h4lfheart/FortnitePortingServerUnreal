#include "FortnitePorting.h"

#define LOCTEXT_NAMESPACE "FFortnitePortingModule"

DEFINE_LOG_CATEGORY(LogFortnitePorting);

void FFortnitePortingModule::StartupModule()
{
	UE_LOG(LogFortnitePorting, Log, TEXT("FortnitePorting Server Listening at %s:%d"), *HOST, PORT)
}

void FFortnitePortingModule::ShutdownModule()
{
	UE_LOG(LogFortnitePorting, Log, TEXT("FortnitePorting Server Closed"))
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFortnitePortingModule, FortnitePorting)