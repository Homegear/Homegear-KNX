/* Copyright 2013-2017 Homegear UG (haftungsbeschränkt) */

#ifndef MYPEER_H_
#define MYPEER_H_

#include "PhysicalInterfaces/MainInterface.h"
#include "DptConverter.h"

#include <homegear-base/BaseLib.h>

using namespace BaseLib;
using namespace BaseLib::DeviceDescription;

namespace MyFamily
{
class MyCentral;

class MyPeer : public BaseLib::Systems::Peer, public BaseLib::Rpc::IWebserverEventSink
{
public:
	MyPeer(uint32_t parentID, IPeerEventSink* eventHandler);
	MyPeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, IPeerEventSink* eventHandler);
	virtual ~MyPeer();
	void init();
	void dispose();

	//Features
	virtual bool wireless() { return false; }
	//End features

	virtual std::string handleCliCommand(std::string command);
	void packetReceived(PMyPacket& packet);

	virtual bool load(BaseLib::Systems::ICentral* central);
    virtual void savePeers() {}

	virtual int32_t getChannelGroupedWith(int32_t channel) { return -1; }
	virtual int32_t getNewFirmwareVersion() { return 0; }
	virtual std::string getFirmwareVersionString(int32_t firmwareVersion) { return "1.0"; }
    virtual bool firmwareUpdateAvailable() { return false; }

    std::string printConfig();

    void initParametersByGroupAddress();
    std::vector<uint16_t> getGroupAddresses();

    /**
	 * {@inheritDoc}
	 */
    virtual void homegearStarted();

    /**
	 * {@inheritDoc}
	 */
    virtual void homegearShuttingDown();

	//RPC methods
	virtual PVariable putParamset(BaseLib::PRpcClientInfo clientInfo, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, PVariable variables, bool onlyPushing = false);
	virtual PVariable setValue(BaseLib::PRpcClientInfo clientInfo, uint32_t channel, std::string valueKey, PVariable value, bool wait);
	//End RPC methods
protected:
	struct ParametersByGroupAddressInfo
	{
		int32_t channel;
		ParameterCast::PGeneric cast;
		PParameter parameter;
	};

	struct GroupedParametersInfo
	{
		PParameter rawParameter;
		PParameter submitParameter;
		std::vector<PParameter> parameters;
	};

	bool _shuttingDown = false;
	std::shared_ptr<DptConverter> _dptConverter;
	std::map<uint16_t, ParametersByGroupAddressInfo> _parametersByGroupAddress;
	std::map<int32_t, std::map<std::string, GroupedParametersInfo>> _groupedParameters;

	//{{{ getValueFromDevice
		struct GetValueFromDeviceInfo
		{
			bool requested;
			std::mutex mutex;
			std::condition_variable conditionVariable;
			bool mutexReady = false;

			int32_t channel;
			std::string variableName;
			PVariable value;
		};

		std::mutex _getValueFromDeviceMutex;
		GetValueFromDeviceInfo _getValueFromDeviceInfo;
	//}}}

	virtual void loadVariables(BaseLib::Systems::ICentral* central, std::shared_ptr<BaseLib::Database::DataTable>& rows);

	virtual std::shared_ptr<BaseLib::Systems::ICentral> getCentral();

	/**
	 * {@inheritDoc}
	 */
	virtual PVariable getValueFromDevice(PParameter& parameter, int32_t channel, bool asynchronous);

	virtual PParameterGroup getParameterSet(int32_t channel, ParameterGroup::Type::Enum type);

	// {{{ Hooks
		/**
		 * {@inheritDoc}
		 */
		virtual bool getAllValuesHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters);

		/**
		 * {@inheritDoc}
		 */
		virtual bool getParamsetHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters);

		/**
		 * {@inheritDoc}
		 */
		virtual bool convertFromPacketHook(PParameter parameter, std::vector<uint8_t>& data, PVariable& result);

		/**
		 * {@inheritDoc}
		 */
		virtual bool convertToPacketHook(PParameter parameter, PVariable data, std::vector<uint8_t>& result);
	// }}}
};

typedef std::shared_ptr<MyPeer> PMyPeer;

}

#endif
