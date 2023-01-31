#pragma once

class FListenServer : public FRunnable
{
public:
	bool bRunThread;
	
	FListenServer();
	~FListenServer();

	/* FRunnable Interface */
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

private:
	FRunnableThread* Thread;
	FSocket* Socket;

	TArray<uint8> StringToBytes(const FString& InStr) const;
	static FString BytesToString(TArray<uint8>& Message, int32 BytesLength);
	void PingClient(const FInternetAddr& Destination) const;
};