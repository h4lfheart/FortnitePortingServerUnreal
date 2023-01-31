#include "ListenServer.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketBuilder.h"
#include "FortnitePorting/Public/Utils.h"

FListenServer::FListenServer()
{
	Thread = FRunnableThread::Create(this, TEXT("FortnitePortingListenServer"));
}

FListenServer::~FListenServer()
{
	if (Thread)
	{
		Thread->Kill();
		delete Thread;
	}
}

bool FListenServer::Init()
{
	bRunThread = true;
	return true;
}

uint32 FListenServer::Run()
{
	FIPv4Endpoint Endpoint;
	FIPv4Endpoint::Parse(TEXT("127.0.0.1:24281"), Endpoint);

	auto BufferSize = 1024;
	Socket = FUdpSocketBuilder(TEXT("FortnitePortingServerSocket"))
	         .AsBlocking()
	         .AsReusable()
	         .WithReceiveBufferSize(BufferSize)
	         .BoundToEndpoint(Endpoint)
	         .Build();

	const auto Address = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	while (bRunThread)
	{
		FString Data;

		// Chunk Receive Loop
		while (true)
		{
			auto BytesRead = 0;
			TArray<uint8> RawData;
			RawData.SetNumUninitialized(BufferSize);

			if (Socket->RecvFrom(RawData.GetData(), BufferSize, BytesRead, *Address))
			{
				// Uncompressed Messages
				auto ReceivedString = FUtils::BytesToString(RawData, BytesRead);
				if (ReceivedString.Equals("MessageFinished", ESearchCase::IgnoreCase))
				{
					break;
				}

				if (ReceivedString.Equals("Ping", ESearchCase::IgnoreCase))
				{
					PingClient(*Address);
					continue;
				}

				// TODO GZIP COMPRESSION
				Data.Append(ReceivedString);
				PingClient(*Address);
			}
		}

		// Decompression for Gzip.
		// auto Uncompressed = FCompression::UncompressMemory(NAME_Gzip, UncompressedContent.GetData(), UncompressedContent.Num(),
		//                                                    RawData.GetData(), RawData.Num());

		FUtils::ImportResponse(Data);
	}

	return 0;
}

void FListenServer::Stop()
{
	Socket->Close();
	bRunThread = false;
}

void FListenServer::PingClient(const FInternetAddr& Destination) const
{
	TArray<uint8> Ping = FUtils::StringToBytes("Ping");
	auto BytesSent = 0;
	Socket->SendTo(Ping.GetData(), Ping.Num(), BytesSent, Destination);
}