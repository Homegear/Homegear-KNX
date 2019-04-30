/* Copyright 2013-2019 Homegear GmbH */

#ifndef MAININTERFACE_H_
#define MAININTERFACE_H_

#include <homegear-base/BaseLib.h>
#include "../KnxIpPacket.h"

namespace Knx {

class MainInterface : public BaseLib::Systems::IPhysicalInterface
{
public:
	explicit MainInterface(const std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings>& settings);
	~MainInterface() override;

	uint8_t getChannelId();
    uint8_t getManagementChannelId();
    uint8_t getSequenceCounter();
    void incrementSequenceCounter();
    uint8_t getManagementSequenceCounter();
    void incrementManagementSequenceCounter();
	uint16_t getKnxAddress();
	bool managementConnected();
    std::array<uint8_t, 4> getListenIpBytes();
    std::array<uint8_t, 2> getListenPortBytes();

    void setReconnected(std::function<void()> value) { _reconnected.swap(value); }

	void startListening() override;
	void stopListening() override;

	bool isOpen() override { return _socket->isOpen() && _initComplete; }

	void sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet) override;

	std::unique_lock<std::mutex> getSendPacketLock();

    void getResponse(ServiceType serviceType, const std::vector<uint8_t>& requestPacket, std::vector<uint8_t>& responsePacket, int32_t timeout = 10000);

    void registerPacketReceivedCallback(std::function<void(const PKnxIpPacket&)> packet) { _packetReceivedCallback.swap(packet); }

    void sendRaw(const std::vector<uint8_t>& packet);

    void connectManagement();
    void disconnectManagement();
protected:
	struct Request
	{
		std::mutex mutex;
		std::condition_variable conditionVariable;
		bool mutexReady = false;
		std::vector<uint8_t> response;
	};

	BaseLib::Output _out;
    std::function<void()> _reconnected;
	std::atomic_bool _initComplete;
	std::string _port;
	std::string _listenIp;
    std::array<uint8_t, 4> _listenIpBytes;
    std::array<uint8_t, 2> _listenPortBytes;
    std::atomic_uchar _managementChannelId;
	std::atomic_int _knxAddress;
	std::atomic_uchar _channelId;
	std::unique_ptr<BaseLib::UdpSocket> _socket;

	std::mutex _sendPacketMutex;
	std::mutex _getResponseMutex;

	std::mutex _requestsMutex;
	std::map<uint32_t, std::shared_ptr<Request>> _requests;

	std::atomic_uchar _sequenceCounter;
	std::atomic_uchar _managementSequenceCounter;
	std::atomic_bool _managementConnected{false};
	int64_t _lastConnectionState = 0;
	std::thread _keepAliveThread;
	std::thread _initThread;

    std::function<void(const PKnxIpPacket&)> _packetReceivedCallback;

	void setListenAddress();
	void reconnect();
	void init();
	void listen();
	void processPacket(const std::vector<uint8_t>& data);
	void sendAck(uint8_t sequenceCounter, uint8_t error);
	void sendDisconnectResponse(KnxIpErrorCodes status, uint8_t channelId);
	bool getConnectionState();
};

}

#endif
