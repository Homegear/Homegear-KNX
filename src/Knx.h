/* Copyright 2013-2019 Homegear GmbH */

#ifndef MYFAMILY_H_
#define MYFAMILY_H_

#include <homegear-base/BaseLib.h>

using namespace BaseLib;

namespace Knx {
class KnxCentral;

class Knx : public BaseLib::Systems::DeviceFamily {
 public:
  Knx(BaseLib::SharedObjects *bl, BaseLib::Systems::IFamilyEventSink *eventHandler);
  ~Knx() override;
  bool init() override;
  void dispose() override;

  virtual bool hasPhysicalInterface() { return true; }
  virtual PVariable getPairingInfo();
  void reloadRpcDevices();
 protected:
  virtual std::shared_ptr<BaseLib::Systems::ICentral> initializeCentral(uint32_t deviceId, int32_t address, std::string serialNumber);
  virtual void createCentral();
};

}

#endif
