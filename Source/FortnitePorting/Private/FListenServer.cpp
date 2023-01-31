#include "FListenServer.h"

#include "FFortnitePortingUtils.h"
#include "FortnitePorting.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketBuilder.h"

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
				auto ReceivedString = BytesToString(RawData, BytesRead);
					
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

		FFortnitePortingUtils::ImportResponse(Data);

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
	auto PingString = StringToBytes("Ping");
	auto BytesSent = 0;
	Socket->SendTo(PingString.GetData(), PingString.Num(), BytesSent, Destination);
}

FString FListenServer::BytesToString(TArray<uint8>& Message, int32 BytesLength)
{
	if (BytesLength <= 0)
	{
		return FString("");
	}
	if (Message.Num() < BytesLength)
	{
		return FString("");
	}

	TArray<uint8> StringAsArray;
	StringAsArray.Reserve(BytesLength);

	for (int i = 0; i < BytesLength; i++)
	{
		StringAsArray.Add(Message[0]);
		Message.RemoveAt(0);
	}

	std::string cstr(reinterpret_cast<const char*>(StringAsArray.GetData()), StringAsArray.Num());
	return FString(UTF8_TO_TCHAR(cstr.c_str()));
}

TArray<uint8> FListenServer::StringToBytes(const FString& InStr) const
{
	const FTCHARToUTF8 Convert(*InStr);
	const auto BytesLength = Convert.Length();
	const auto MessageBytes = static_cast<uint8*>(FMemory::Malloc(BytesLength));
	FMemory::Memcpy(MessageBytes, TCHAR_TO_UTF8(InStr.GetCharArray().GetData()), BytesLength);

	TArray<uint8> Result;
	for (auto i = 0; i < BytesLength; i++)
	{
		Result.Add(MessageBytes[i]);
	}

	FMemory::Free(MessageBytes);

	return Result;
}







