#include "FortnitePorting.h"
#include "Common/UdpSocketBuilder.h"

#define LOCTEXT_NAMESPACE "FFortnitePortingModule"

DEFINE_LOG_CATEGORY(LogFortnitePorting);

void FFortnitePortingModule::StartupModule()
{
	UE_LOG(LogFortnitePorting, Log, TEXT("FortnitePorting Server Listening at localhost:%d"), 24281)

	FIPv4Endpoint Endpoint;
    FIPv4Endpoint::Parse("127.0.0.1:24281", Endpoint);

    auto BufferSize = 1024;
    const auto Socket = FUdpSocketBuilder(TEXT("FortnitePortingServerSocket"))
	    .AsNonBlocking()
	    .WithReceiveBufferSize(BufferSize)
	    .BoundToEndpoint(Endpoint)
	    .Build();
}

void FFortnitePortingModule::ShutdownModule()
{
	UE_LOG(LogFortnitePorting, Log, TEXT("FortnitePorting Server Closed"))
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFortnitePortingModule, FortnitePorting)