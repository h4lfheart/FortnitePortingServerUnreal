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

	void PingClient(const FInternetAddr& Destination) const;
};