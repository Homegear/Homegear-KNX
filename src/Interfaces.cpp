/* Copyright 2013-2019 Homegear GmbH */

#include "Interfaces.h"
#include "GD.h"
#include "KnxIpForwarder.h"
#include "Cemi.h"

namespace Knx {

Interfaces::Interfaces(BaseLib::SharedObjects *bl, std::map<std::string, Systems::PPhysicalInterfaceSettings> physicalInterfaceSettings) : Systems::PhysicalInterfaces(bl, GD::family->getFamily(), physicalInterfaceSettings) {
  create();
}

Interfaces::~Interfaces() {
}

void Interfaces::create() {
  try {
    for (auto deviceEntry : _physicalInterfaceSettings) {
      std::shared_ptr<MainInterface> device;
      if (!deviceEntry.second) continue;
      GD::out.printDebug("Debug: Creating physical device. Type defined in knx.conf is: " + deviceEntry.second->type);
      if (deviceEntry.second->type == "knxnetip") device.reset(new MainInterface(deviceEntry.second));
      else GD::out.printError("Error: Unsupported physical device type: " + deviceEntry.second->type);
      if (device) {
        if (_physicalInterfaces.find(deviceEntry.second->id) != _physicalInterfaces.end()) GD::out.printError("Error: id used for two devices: " + deviceEntry.second->id);
        _physicalInterfaces[deviceEntry.second->id] = device;
        GD::physicalInterfaces[deviceEntry.second->id] = device;
        if (deviceEntry.second->isDefault || !GD::defaultPhysicalInterface) GD::defaultPhysicalInterface = device;

        auto iterator = deviceEntry.second->all.find("enableforwarder");
        if (iterator != deviceEntry.second->all.end()) {
          auto listenIpIterator = deviceEntry.second->all.find("forwarderlistenip");
          auto portIterator = deviceEntry.second->all.find("forwarderlistenport");

          if (portIterator == deviceEntry.second->all.end()) {
            GD::out.printError(R"(Error: Missing setting "forwarderListenPort".)");
            continue;
          }

          std::string listenIp;
          if (listenIpIterator != deviceEntry.second->all.end()) listenIp = listenIpIterator->second->stringValue;

          uint16_t port = portIterator->second->integerValue64;
          if (port == 0) {
            GD::out.printError(R"(Error: Invalid value for setting "forwarderListenPort".)");
            continue;
          }

          GD::out.printInfo("Info: Starting KNXnet/IP forwarder for interface " + deviceEntry.second->id + "...");

          //Add forwarder for device
          auto forwarder = std::make_shared<KnxIpForwarder>(listenIp, port, device);
          _forwarders.emplace(deviceEntry.second->id, forwarder);
          forwarder->startListening();
        }
      }
    }
    if (!GD::defaultPhysicalInterface) GD::defaultPhysicalInterface = std::make_shared<MainInterface>(std::make_shared<BaseLib::Systems::PhysicalInterfaceSettings>());
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

}
