/* Copyright 2013-2017 Homegear UG (haftungsbeschränkt) */

#ifndef MYCENTRAL_H_
#define MYCENTRAL_H_

#include <homegear-base/BaseLib.h>
#include "MyPeer.h"
#include "Search.h"

#include <stdio.h>
#include <memory>
#include <mutex>
#include <string>

namespace MyFamily
{

typedef std::shared_ptr<std::map<uint64_t, PMyPeer>> PGroupAddressPeers;

class MyCentral : public BaseLib::Systems::ICentral
{
public:
	MyCentral(ICentralEventSink* eventHandler);
	MyCentral(uint32_t deviceType, std::string serialNumber, ICentralEventSink* eventHandler);
	virtual ~MyCentral();
	virtual void dispose(bool wait = true);

	std::string handleCliCommand(std::string command);
	virtual bool onPacketReceived(std::string& senderId, std::shared_ptr<BaseLib::Systems::Packet> packet);

	uint64_t getPeerIdFromSerial(std::string& serialNumber) { std::shared_ptr<MyPeer> peer = getPeer(serialNumber); if(peer) return peer->getID(); else return 0; }
	PMyPeer getPeer(uint64_t id);
	PMyPeer getPeer(std::string serialNumber);
	PGroupAddressPeers getPeer(uint16_t groupAddress);

	virtual PVariable deleteDevice(BaseLib::PRpcClientInfo clientInfo, std::string serialNumber, int32_t flags);
	virtual PVariable deleteDevice(BaseLib::PRpcClientInfo clientInfo, uint64_t peerId, int32_t flags);
	virtual PVariable getDeviceInfo(BaseLib::PRpcClientInfo clientInfo, uint64_t id, std::map<std::string, bool> fields);
	virtual PVariable putParamset(BaseLib::PRpcClientInfo clientInfo, std::string serialNumber, int32_t channel, ParameterGroup::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, PVariable paramset);
	virtual PVariable putParamset(BaseLib::PRpcClientInfo clientInfo, uint64_t peerId, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteId, int32_t remoteChannel, PVariable paramset);
	virtual PVariable searchDevices(BaseLib::PRpcClientInfo clientInfo);
	virtual PVariable setInterface(BaseLib::PRpcClientInfo clientInfo, uint64_t peerId, std::string interfaceId);
protected:
	std::unique_ptr<Search> _search;
	std::mutex _searchMutex;
	std::map<uint16_t, PGroupAddressPeers> _peersByGroupAddress;

	std::atomic_bool _stopWorkerThread;
	std::thread _workerThread;

	virtual void init();
    virtual void worker();
	virtual void loadPeers();
	virtual void savePeers(bool full);
	virtual void loadVariables() {}
	virtual void saveVariables() {}
	PMyPeer createPeer(uint32_t type, std::string serialNumber, bool save = true);
	void deletePeer(uint64_t id);
	void removePeerFromGroupAddresses(uint16_t groupAddress, uint64_t peerId);
    void interfaceReconnected();
};

}

#endif
