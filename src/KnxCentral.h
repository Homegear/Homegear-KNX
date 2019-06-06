/* Copyright 2013-2019 Homegear GmbH */

#ifndef MYCENTRAL_H_
#define MYCENTRAL_H_

#include <homegear-base/BaseLib.h>
#include "KnxPeer.h"
#include "Search.h"

#include <stdio.h>
#include <memory>
#include <mutex>
#include <string>

namespace Knx
{

typedef std::shared_ptr<std::map<uint64_t, PMyPeer>> PGroupAddressPeers;

class KnxCentral : public BaseLib::Systems::ICentral
{
public:
	KnxCentral(ICentralEventSink* eventHandler);
	KnxCentral(uint32_t deviceType, std::string serialNumber, ICentralEventSink* eventHandler);
	virtual ~KnxCentral();
	virtual void dispose(bool wait = true);

	std::string handleCliCommand(std::string command);
	virtual bool onPacketReceived(std::string& senderId, std::shared_ptr<BaseLib::Systems::Packet> packet);

	uint64_t getPeerIdFromSerial(std::string& serialNumber) { std::shared_ptr<KnxPeer> peer = getPeer(serialNumber); if(peer) return peer->getID(); else return 0; }
	PMyPeer getPeer(uint64_t id);
	PMyPeer getPeer(int32_t address);
	PMyPeer getPeer(std::string serialNumber);
	PGroupAddressPeers getPeer(uint16_t groupAddress);

	virtual PVariable deleteDevice(BaseLib::PRpcClientInfo clientInfo, std::string serialNumber, int32_t flags);
	virtual PVariable deleteDevice(BaseLib::PRpcClientInfo clientInfo, uint64_t peerId, int32_t flags);
    virtual PVariable invokeFamilyMethod(BaseLib::PRpcClientInfo clientInfo, std::string& method, PArray parameters);
	virtual PVariable searchDevices(BaseLib::PRpcClientInfo clientInfo);
	virtual PVariable setInterface(BaseLib::PRpcClientInfo clientInfo, uint64_t peerId, std::string interfaceId);
protected:
	std::map<std::string, std::function<BaseLib::PVariable(BaseLib::PRpcClientInfo& clientInfo, BaseLib::PArray& parameters)>> _localRpcMethods;

	std::unique_ptr<Search> _search;
	std::mutex _searchMutex;
	std::map<uint16_t, PGroupAddressPeers> _peersByGroupAddress;

	std::atomic_bool _stopWorkerThread;
	std::thread _workerThread;

	virtual void init();
    virtual void worker();
	void loadPeers() override;
	void savePeers(bool full) override;
	void loadVariables() override {}
	void saveVariables() override {}
	void setPeerId(uint64_t oldPeerId, uint64_t newPeerId) override;
	PMyPeer createPeer(uint32_t type, int32_t address, std::string serialNumber, bool save = true);
	void deletePeer(uint64_t id);
	void removePeerFromGroupAddresses(uint16_t groupAddress, uint64_t peerId);
    void interfaceReconnected();

	//{{{ Family RPC methods
		BaseLib::PVariable updateDevice(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray& parameters);
	//}}}
};

}

#endif