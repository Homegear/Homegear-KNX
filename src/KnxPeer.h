/* Copyright 2013-2019 Homegear GmbH */

#ifndef MYPEER_H_
#define MYPEER_H_

#include "PhysicalInterfaces/MainInterface.h"
#include "DptConverter.h"

#include <homegear-base/BaseLib.h>
#include <unordered_set>

using namespace BaseLib;
using namespace BaseLib::DeviceDescription;

namespace Knx {
class KnxCentral;
class Cemi;
typedef std::shared_ptr<Cemi> PCemi;

class KnxPeer : public BaseLib::Systems::Peer, public BaseLib::Rpc::IWebserverEventSink {
 public:
  KnxPeer(uint32_t parentID, IPeerEventSink *eventHandler);
  KnxPeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, IPeerEventSink *eventHandler);
  ~KnxPeer() override;
  void init();
  void dispose() override;
  void stopWorkerThread() { _stopWorkerThread = true; }

  //Features
  bool wireless() override { return false; }
  //End features

  void worker();
  void interfaceReconnected() { _readVariables = true; }
  std::string handleCliCommand(std::string command) override;
  void packetReceived(PCemi &packet);

  bool load(BaseLib::Systems::ICentral *central) override;
  void savePeers() override {}

  int32_t getChannelGroupedWith(int32_t channel) override { return -1; }
  int32_t getNewFirmwareVersion() override { return 0; }
  std::string getFirmwareVersionString(int32_t firmwareVersion) override { return "1.0"; }
  bool firmwareUpdateAvailable() override { return false; }

  std::string getFormattedAddress();

  std::string printConfig();

  void initParametersByGroupAddress();
  std::vector<uint16_t> getGroupAddresses();

  /**
   * {@inheritDoc}
   */
  void homegearStarted() override;

  /**
   * {@inheritDoc}
   */
  void homegearShuttingDown() override;

  //RPC methods
  PVariable getDeviceInfo(BaseLib::PRpcClientInfo clientInfo, std::map<std::string, bool> fields) override;
  PVariable putParamset(BaseLib::PRpcClientInfo clientInfo, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, PVariable variables, bool checkAcls, bool onlyPushing) override;
  PVariable setValue(BaseLib::PRpcClientInfo clientInfo, uint32_t channel, std::string valueKey, PVariable value, bool wait) override;
  //End RPC methods
 protected:
  struct ParametersByGroupAddressInfo {
    int32_t channel = -1;
    ParameterCast::PGeneric cast;
    PParameter parameter;
  };

  struct GroupedParametersInfo {
    PParameter rawParameter;
    PParameter submitParameter;
    std::vector<PParameter> parameters;
  };

  std::atomic_bool _stopWorkerThread;
  std::atomic_bool _readVariables;
  std::shared_ptr<DptConverter> _dptConverter;
  std::map<uint16_t, std::vector<ParametersByGroupAddressInfo>> _parametersByGroupAddress;
  std::map<int32_t, std::map<std::string, GroupedParametersInfo>> _groupedParameters;

  //{{{ getValueFromDevice
  struct GetValueFromDeviceInfo {
    bool requested = false;
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool mutexReady = false;

    int32_t channel = -1;
    std::string variableName;
    PVariable value;
  };

  std::mutex _getValueFromDeviceMutex;
  GetValueFromDeviceInfo _getValueFromDeviceInfo;
  //}}}

  void loadVariables(BaseLib::Systems::ICentral *central, std::shared_ptr<BaseLib::Database::DataTable> &rows) override;

  std::shared_ptr<BaseLib::Systems::ICentral> getCentral() override;

  /**
   * {@inheritDoc}
   */
  PVariable getValueFromDevice(PParameter &parameter, int32_t channel, bool asynchronous) override;

  PParameterGroup getParameterSet(int32_t channel, ParameterGroup::Type::Enum type) override;

  void sendPacket(const PCemi &packet);

  // {{{ Hooks
  /**
   * {@inheritDoc}
   */
  bool getAllValuesHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters) override;

  /**
   * {@inheritDoc}
   */
  bool getParamsetHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters) override;

  /**
   * {@inheritDoc}
   */
  bool convertFromPacketHook(BaseLib::Systems::RpcConfigurationParameter &parameter, std::vector<uint8_t> &data, PVariable &result) override;

  /**
   * {@inheritDoc}
   */
  bool convertToPacketHook(BaseLib::Systems::RpcConfigurationParameter &parameter, PVariable &data, std::vector<uint8_t> &result) override;
  // }}}
};

typedef std::shared_ptr<KnxPeer> PKnxPeer;

}

#endif
