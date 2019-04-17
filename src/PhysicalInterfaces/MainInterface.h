/* Copyright 2013-2019 Homegear GmbH */

#ifndef MAININTERFACE_H_
#define MAININTERFACE_H_

#include "../MyPacket.h"
#include <homegear-base/BaseLib.h>

namespace MyFamily {

class MainInterface : public BaseLib::Systems::IPhysicalInterface
{
public:
	MainInterface(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings);
	virtual ~MainInterface();

    void setReconnected(std::function<void()> value) { _reconnected.swap(value); }

	void startListening();
	void stopListening();

	bool isOpen() { return _socket->isOpen() && _initComplete; }

	void sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet);

protected:
	std::map<char, std::string> _connectResponseStatusCodes;
	std::map<char, std::string> _connectionStateResponseStatusCodes;
	std::map<char, std::string> _tunnelingAckStatusCodes;

	class Request
	{
	public:
		std::mutex mutex;
		std::condition_variable conditionVariable;
		bool mutexReady = false;
		std::vector<char> response;

		Request() {}
		virtual ~Request() {}
	private:
	};

	BaseLib::Output _out;
    std::function<void()> _reconnected;
	std::atomic_bool _initComplete;
	std::string _port;
	std::string _listenIp;
	char _listenIpBytes[4];
	char _listenPortBytes[2];
	std::atomic_int32_t _knxAddress;
	std::atomic_char _channelId;
	std::unique_ptr<BaseLib::UdpSocket> _socket;

	std::mutex _sendPacketMutex;
	std::mutex _getResponseMutex;

	std::mutex _requestsMutex;
	std::map<uint32_t, std::shared_ptr<Request>> _requests;

	std::atomic_uchar _sequenceCounter;
	int64_t _lastConnectionState = 0;
	std::thread _keepAliveThread;
	std::thread _initThread;

	void setListenAddress();
	void reconnect();
	void init();
	void listen();
	void processPacket(std::vector<char>& data);
	void getSystemResponse(uint16_t serviceType, const std::vector<char>& requestPacket, std::vector<char>& responsePacket);
	void sendAck(char sequenceCounter, char error);
	void sendDisconnectResponse(char sequenceCounter, char error);
	bool getConnectionState();
};

}

#endif
