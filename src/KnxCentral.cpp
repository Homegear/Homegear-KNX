/* Copyright 2013-2019 Homegear GmbH */

#include "KnxCentral.h"
#include "Gd.h"
#include "Cemi.h"
#include "KnxIpPacket.h"

#include <iomanip>

namespace Knx {

KnxCentral::KnxCentral(ICentralEventSink *eventHandler) : BaseLib::Systems::ICentral(MY_FAMILY_ID, Gd::bl, eventHandler) {
  init();
}

KnxCentral::KnxCentral(uint32_t deviceID, std::string serialNumber, ICentralEventSink *eventHandler) : BaseLib::Systems::ICentral(MY_FAMILY_ID, Gd::bl, deviceID, serialNumber, -1, eventHandler) {
  init();
}

KnxCentral::~KnxCentral() {
  dispose();
}

void KnxCentral::dispose(bool wait) {
  try {
    if (_disposing) return;
    _disposing = true;

    _stopWorkerThread = true;

    auto peers = getPeers();
    for (auto &peer: peers) {
      auto myPeer = std::dynamic_pointer_cast<KnxPeer>(peer);
      myPeer->stopWorkerThread();
    }

    Gd::out.printDebug("Debug: Waiting for worker thread of device " + std::to_string(_deviceId) + "...");
    Gd::bl->threadManager.join(_workerThread);

    Gd::out.printDebug("Removing device " + std::to_string(_deviceId) + " from physical device's event queue...");
    for (std::map<std::string, std::shared_ptr<MainInterface>>::iterator i = Gd::physicalInterfaces.begin(); i != Gd::physicalInterfaces.end(); ++i) {
      //Just to make sure cycle through all physical devices. If event handler is not removed => segfault
      i->second->removeEventHandler(_physicalInterfaceEventhandlers[i->first]);
    }
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void KnxCentral::init() {
  try {
    if (_initialized) return; //Prevent running init two times
    _initialized = true;

    _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(const BaseLib::PRpcClientInfo &clientInfo, const BaseLib::PArray &parameters)>>("updateDevices",
                                                                                                                                                                    std::bind(&KnxCentral::updateDevices,
                                                                                                                                                                              this,
                                                                                                                                                                              std::placeholders::_1,
                                                                                                                                                                              std::placeholders::_2)));
    _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(const BaseLib::PRpcClientInfo &clientInfo, const BaseLib::PArray &parameters)>>("groupValueRead",
                                                                                                                                                                    std::bind(&KnxCentral::groupValueRead,
                                                                                                                                                                              this,
                                                                                                                                                                              std::placeholders::_1,
                                                                                                                                                                              std::placeholders::_2)));
    _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(const BaseLib::PRpcClientInfo &clientInfo, const BaseLib::PArray &parameters)>>("groupValueWrite",
                                                                                                                                                                    std::bind(&KnxCentral::groupValueWrite,
                                                                                                                                                                              this,
                                                                                                                                                                              std::placeholders::_1,
                                                                                                                                                                              std::placeholders::_2)));
    _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(const BaseLib::PRpcClientInfo &clientInfo, const BaseLib::PArray &parameters)>>("sendRawPacket",
                                                                                                                                                                    std::bind(&KnxCentral::groupValueWrite,
                                                                                                                                                                              this,
                                                                                                                                                                              std::placeholders::_1,
                                                                                                                                                                              std::placeholders::_2)));

    _search.reset(new Search());

    for (std::map<std::string, std::shared_ptr<MainInterface>>::iterator i = Gd::physicalInterfaces.begin(); i != Gd::physicalInterfaces.end(); ++i) {
      _physicalInterfaceEventhandlers[i->first] = i->second->addEventHandler((BaseLib::Systems::IPhysicalInterface::IPhysicalInterfaceEventSink *)this);
      i->second->setReconnected(std::function<void()>(std::bind(&KnxCentral::interfaceReconnected, this)));
    }

    _stopWorkerThread = false;
    Gd::bl->threadManager.start(_workerThread, true, _bl->settings.workerThreadPriority(), _bl->settings.workerThreadPolicy(), &KnxCentral::worker, this);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void KnxCentral::interfaceReconnected() {
  try {
    auto peers = getPeers();
    for (auto &peer: peers) {
      auto myPeer = std::dynamic_pointer_cast<KnxPeer>(peer);
      myPeer->interfaceReconnected();
    }
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void KnxCentral::worker() {
  try {
    std::chrono::milliseconds sleepingTime(100);
    uint32_t counter = 0;
    uint64_t lastPeer;
    lastPeer = 0;

    while (!_stopWorkerThread && !Gd::bl->shuttingDown) {
      try {
        std::this_thread::sleep_for(sleepingTime);
        if (_stopWorkerThread || Gd::bl->shuttingDown) return;
        if (counter > 1000) {
          counter = 0;

          {
            std::lock_guard<std::mutex> peersGuard(_peersMutex);
            if (_peersById.size() > 0) {
              int32_t windowTimePerPeer = _bl->settings.workerThreadWindow() / _peersById.size();
              sleepingTime = std::chrono::milliseconds(windowTimePerPeer);
            }
          }
        }

        std::shared_ptr<KnxPeer> peer;

        {
          std::lock_guard<std::mutex> peersGuard(_peersMutex);
          if (!_peersById.empty()) {
            if (!_peersById.empty()) {
              std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator nextPeer = _peersById.find(lastPeer);
              if (nextPeer != _peersById.end()) {
                nextPeer++;
                if (nextPeer == _peersById.end()) nextPeer = _peersById.begin();
              } else nextPeer = _peersById.begin();
              lastPeer = nextPeer->first;
              peer = std::dynamic_pointer_cast<KnxPeer>(nextPeer->second);
            }
          }
        }

        if (peer && !peer->deleting) peer->worker();
        counter++;
      }
      catch (const std::exception &ex) {
        Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
    }
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void KnxCentral::loadPeers() {
  try {
    std::shared_ptr<BaseLib::Database::DataTable> rows = _bl->db->getPeers(_deviceId);
    for (BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row) {
      uint64_t peerID = row->second.at(0)->intValue;
      Gd::out.printMessage("Loading KNX peer " + std::to_string(peerID));
      std::shared_ptr<KnxPeer> peer(new KnxPeer(peerID, row->second.at(2)->intValue, row->second.at(3)->textValue, _deviceId, this));
      if (!peer->load(this)) {
        if (peer->getDeviceType() == 0) {
          Gd::out.printError("Deleting peer " + std::to_string(peerID) + " with invalid device type.");
          peer->deleteFromDatabase();
        }
        continue;
      }
      if (!peer->getRpcDevice()) continue;
      std::lock_guard<std::mutex> peersGuard(_peersMutex);
      if (!peer->getSerialNumber().empty()) _peersBySerial[peer->getSerialNumber()] = peer;
      _peersById[peerID] = peer;
      if (peer->getAddress() != -1) _peers[peer->getAddress()] = peer;
      std::vector<uint16_t> groupAddresses = peer->getGroupAddresses();
      for (std::vector<uint16_t>::iterator i = groupAddresses.begin(); i != groupAddresses.end(); ++i) {
        auto peersIterator = _peersByGroupAddress.find(*i);
        if (peersIterator == _peersByGroupAddress.end()) _peersByGroupAddress.emplace(*i, std::make_shared<std::map<uint64_t, PKnxPeer>>());
        _peersByGroupAddress[*i]->emplace(peerID, peer);
      }
    }
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

std::shared_ptr<KnxPeer> KnxCentral::getPeer(uint64_t id) {
  try {
    std::lock_guard<std::mutex> peersGuard(_peersMutex);
    auto peersIterator = _peersById.find(id);
    if (peersIterator != _peersById.end()) return std::dynamic_pointer_cast<KnxPeer>(peersIterator->second);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::shared_ptr<KnxPeer>();
}

std::shared_ptr<KnxPeer> KnxCentral::getPeer(int32_t address) {
  try {
    std::lock_guard<std::mutex> peersGuard(_peersMutex);
    auto peersIterator = _peers.find(address);
    if (peersIterator != _peers.end()) return std::dynamic_pointer_cast<KnxPeer>(peersIterator->second);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::shared_ptr<KnxPeer>();
}

std::shared_ptr<KnxPeer> KnxCentral::getPeer(std::string serialNumber) {
  try {
    std::lock_guard<std::mutex> peersGuard(_peersMutex);
    auto peersIterator = _peersBySerial.find(serialNumber);
    if (peersIterator != _peersBySerial.end()) return std::dynamic_pointer_cast<KnxPeer>(peersIterator->second);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::shared_ptr<KnxPeer>();
}

PGroupAddressPeers KnxCentral::getPeer(uint16_t groupAddress) {
  try {
    std::lock_guard<std::mutex> peersGuard(_peersMutex);
    auto peersIterator = _peersByGroupAddress.find(groupAddress);
    if (peersIterator != _peersByGroupAddress.end()) return peersIterator->second;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return PGroupAddressPeers();
}

uint64_t KnxCentral::getRoomIdByName(std::string &name) {
  return raiseGetRoomIdByName(name);
}

bool KnxCentral::onPacketReceived(std::string &senderId, std::shared_ptr<BaseLib::Systems::Packet> packet) {
  try {
    if (_disposing) return false;
    if (!packet) return false;
    std::shared_ptr<Cemi> myPacket(std::dynamic_pointer_cast<Cemi>(packet));
    if (!myPacket) return false;

    if (_bl->debugLevel >= 4)
      Gd::out.printInfo("Packet received from " + myPacket->getFormattedSourceAddress() + " to " + myPacket->getFormattedDestinationAddress() + ". Operation: " + myPacket->getOperationString() + ". Payload: "
                            + BaseLib::HelperFunctions::getHexString(myPacket->getPayload()));

    auto peers = getPeer(myPacket->getDestinationAddress());
    if (!peers) return false;
    for (auto &peer: *peers) {
      peer.second->packetReceived(myPacket);
    }

    return true;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

void KnxCentral::savePeers(bool full) {
  try {
    std::lock_guard<std::mutex> peersGuard(_peersMutex);
    for (std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peersById.begin(); i != _peersById.end(); ++i) {
      Gd::out.printInfo("Info: Saving KNX peer " + std::to_string(i->second->getID()));
      i->second->save(full, full, full);
    }
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void KnxCentral::setPeerId(uint64_t oldPeerId, uint64_t newPeerId) {
  try {
    ICentral::setPeerId(oldPeerId, newPeerId);

    auto peer = getPeer(newPeerId);
    auto groupAddresses = peer->getGroupAddresses();

    for (const uint16_t &address: groupAddresses) {
      removePeerFromGroupAddresses(address, oldPeerId);
    }

    std::lock_guard<std::mutex> peersGuard(_peersMutex);
    for (const uint16_t &address: groupAddresses) {
      auto peersIterator = _peersByGroupAddress.find(address);
      if (peersIterator == _peersByGroupAddress.end()) _peersByGroupAddress.emplace(address, std::make_shared<std::map<uint64_t, PKnxPeer>>());
      _peersByGroupAddress[address]->emplace(newPeerId, peer);
    }
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void KnxCentral::removePeerFromGroupAddresses(uint16_t groupAddress, uint64_t peerId) {
  try {
    std::lock_guard<std::mutex> peersGuard(_peersMutex);
    auto peersIterator = _peersByGroupAddress.find(groupAddress);
    if (peersIterator == _peersByGroupAddress.end()) return;
    peersIterator->second->erase(peerId);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void KnxCentral::deletePeer(uint64_t id) {
  try {
    std::shared_ptr<KnxPeer> peer(getPeer(id));
    if (!peer) return;
    peer->deleting = true;
    PVariable deviceAddresses(new Variable(VariableType::tArray));
    deviceAddresses->arrayValue->push_back(PVariable(new Variable(peer->getSerialNumber())));

    PVariable deviceInfo(new Variable(VariableType::tStruct));
    deviceInfo->structValue->insert(StructElement("ID", PVariable(new Variable((int32_t)peer->getID()))));
    PVariable channels(new Variable(VariableType::tArray));
    deviceInfo->structValue->insert(StructElement("CHANNELS", channels));

    for (Functions::iterator i = peer->getRpcDevice()->functions.begin(); i != peer->getRpcDevice()->functions.end(); ++i) {
      deviceAddresses->arrayValue->push_back(PVariable(new Variable(peer->getSerialNumber() + ":" + std::to_string(i->first))));
      channels->arrayValue->push_back(PVariable(new Variable(i->first)));
    }

    std::vector<uint64_t> deletedIds{id};
    raiseRPCDeleteDevices(deletedIds, deviceAddresses, deviceInfo);

    std::vector<uint16_t> groupAddresses;
    {
      std::lock_guard<std::mutex> peersGuard(_peersMutex);
      if (_peersBySerial.find(peer->getSerialNumber()) != _peersBySerial.end()) _peersBySerial.erase(peer->getSerialNumber());
      if (peer->getAddress() != -1 && _peers.find(peer->getAddress()) != _peers.end()) _peers.erase(peer->getAddress());
      if (_peersById.find(id) != _peersById.end()) _peersById.erase(id);

      groupAddresses = peer->getGroupAddresses();
    }

    for (const uint16_t &address: groupAddresses) {
      removePeerFromGroupAddresses(address, id);
    }

    int32_t i = 0;
    while (peer.use_count() > 1 && i < 600) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      i++;
    }
    if (i == 600) Gd::out.printError("Error: Peer deletion took too long.");

    peer->deleteFromDatabase();

    Gd::out.printInfo("Info: Deleting XML file \"" + peer->getRpcDevice()->getPath() + "\"");
    Gd::bl->io.deleteFile(peer->getRpcDevice()->getPath());
    Gd::out.printMessage("Removed KNX peer " + std::to_string(peer->getID()));
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

std::string KnxCentral::handleCliCommand(std::string command) {
  try {
    std::vector<std::string> arguments;
    bool showHelp = false;
    std::ostringstream stringStream;
    if (BaseLib::HelperFunctions::checkCliCommand(command, "help", "h", "", 0, arguments, showHelp)) {
      stringStream << "List of commands:" << std::endl << std::endl;
      stringStream << "For more information about the individual command type: COMMAND help" << std::endl << std::endl;
      stringStream << "peers list (ls)    List all peers" << std::endl;
      stringStream << "peers remove (pr)  Remove a peer" << std::endl;
      stringStream << "peers select (ps)  Select a peer" << std::endl;
      stringStream << "peers setname (pn) Name a peer" << std::endl;
      stringStream << "search (sp)        Searches for new devices" << std::endl;
      stringStream << "unselect (u)       Unselect this device" << std::endl;
      return stringStream.str();
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "peers remove", "pr", "", 0, arguments, showHelp)) {
      if (showHelp) {
        stringStream << "Description: This command removes a peer." << std::endl;
        stringStream << "Usage: peers unpair PEERID" << std::endl << std::endl;
        stringStream << "Parameters:" << std::endl;
        stringStream << "  PEERID: The id of the peer to remove. Example: 513" << std::endl;
        return stringStream.str();
      }

      uint64_t peerID = (arguments.size() > 0 ? BaseLib::Math::getNumber64(arguments.at(0)) : 0);

      if (!peerExists(peerID)) stringStream << "This peer is not paired to this central." << std::endl;
      else {
        stringStream << "Removing peer " << std::to_string(peerID) << std::endl;
        deletePeer(peerID);
      }
      return stringStream.str();
    } else if (command.compare(0, 10, "peers list") == 0 || command.compare(0, 2, "pl") == 0 || command.compare(0, 2, "ls") == 0) {
      try {
        std::string filterType;
        std::string filterValue;

        std::stringstream stream(command);
        std::string element;
        int32_t offset = (command.at(1) == 'l' || command.at(1) == 's') ? 0 : 1;
        int32_t index = 0;
        while (std::getline(stream, element, ' ')) {
          if (index < 1 + offset) {
            index++;
            continue;
          } else if (index == 1 + offset) {
            if (element == "help") {
              index = -1;
              break;
            }
            filterType = BaseLib::HelperFunctions::toLower(element);
          } else if (index == 2 + offset) {
            filterValue = element;
            if (filterType == "name") BaseLib::HelperFunctions::toLower(filterValue);
          }
          index++;
        }
        if (index == -1) {
          stringStream << "Description: This command lists information about all peers." << std::endl;
          stringStream << "Usage: peers list [FILTERTYPE] [FILTERVALUE]" << std::endl << std::endl;
          stringStream << "Parameters:" << std::endl;
          stringStream << "  FILTERTYPE:  See filter types below." << std::endl;
          stringStream << "  FILTERVALUE: Depends on the filter type. If a number is required, it has to be in hexadecimal format." << std::endl << std::endl;
          stringStream << "Filter types:" << std::endl;
          stringStream << "  ID: Filter by id." << std::endl;
          stringStream << "      FILTERVALUE: The id of the peer to filter (e. g. 513)." << std::endl;
          stringStream << "  SERIAL: Filter by serial number." << std::endl;
          stringStream << "      FILTERVALUE: The serial number of the peer to filter (e. g. JEQ0554309)." << std::endl;
          stringStream << "  NAME: Filter by name." << std::endl;
          stringStream << "      FILTERVALUE: The part of the name to search for (e. g. \"1st floor\")." << std::endl;
          stringStream << "  TYPE: Filter by device type." << std::endl;
          stringStream << "      FILTERVALUE: The 2 byte device type in hexadecimal format." << std::endl;
          return stringStream.str();
        }

        if (_peersById.empty()) {
          stringStream << "No peers are paired to this central." << std::endl;
          return stringStream.str();
        }
        std::string bar(" │ ");
        const int32_t idWidth = 8;
        const int32_t nameWidth = 25;
        const int32_t addressWidth = 9;
        const int32_t serialWidth = 13;
        const int32_t typeWidth1 = 7;
        const int32_t typeWidth2 = 25;
        std::string nameHeader("Name");
        nameHeader.resize(nameWidth, ' ');
        std::string typeStringHeader("Type String");
        typeStringHeader.resize(typeWidth2, ' ');
        stringStream << std::setfill(' ')
                     << std::setw(idWidth) << "ID" << bar
                     << nameHeader << bar
                     << std::setw(addressWidth) << "Address" << bar
                     << std::setw(serialWidth) << "Serial Number" << bar
                     << std::setw(typeWidth1) << "Type" << bar
                     << typeStringHeader
                     << std::endl;
        stringStream << "─────────┼───────────────────────────┼───────────┼───────────────┼─────────┼───────────────────────────" << std::endl;
        stringStream << std::setfill(' ')
                     << std::setw(idWidth) << " " << bar
                     << std::setw(nameWidth) << " " << bar
                     << std::setw(addressWidth) << " " << bar
                     << std::setw(serialWidth) << " " << bar
                     << std::setw(typeWidth1) << " " << bar
                     << std::setw(typeWidth2)
                     << std::endl;
        _peersMutex.lock();
        for (std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peersById.begin(); i != _peersById.end(); ++i) {
          auto myPeer = std::dynamic_pointer_cast<KnxPeer>(i->second);

          if (filterType == "id") {
            uint64_t id = BaseLib::Math::getNumber(filterValue, false);
            if (i->second->getID() != id) continue;
          } else if (filterType == "name") {
            std::string name = i->second->getName();
            if ((signed)BaseLib::HelperFunctions::toLower(name).find(filterValue) == (signed)std::string::npos) continue;
          } else if (filterType == "serial") {
            if (i->second->getSerialNumber() != filterValue) continue;
          } else if (filterType == "type") {
            int32_t deviceType = BaseLib::Math::getNumber(filterValue, true);
            if ((int32_t)i->second->getDeviceType() != deviceType) continue;
          }

          stringStream << std::setw(idWidth) << std::setfill(' ') << std::to_string(i->second->getID()) << bar;
          std::string name = i->second->getName();
          size_t nameSize = BaseLib::HelperFunctions::utf8StringSize(name);
          if (nameSize > (unsigned)nameWidth) {
            name = BaseLib::HelperFunctions::utf8Substring(name, 0, nameWidth - 3);
            name += "...";
          } else name.resize(nameWidth + (name.size() - nameSize), ' ');
          stringStream << name << bar
                       << std::setw(addressWidth) << myPeer->getFormattedAddress() << bar
                       << std::setw(serialWidth) << i->second->getSerialNumber() << bar
                       << std::setw(typeWidth1) << BaseLib::HelperFunctions::getHexString(i->second->getDeviceType()) << bar;
          if (i->second->getRpcDevice()) {
            PSupportedDevice type = i->second->getRpcDevice()->getType(i->second->getDeviceType(), i->second->getFirmwareVersion());
            std::string typeID;
            if (type) typeID = type->id;
            if (typeID.size() > (unsigned)typeWidth2) {
              typeID.resize(typeWidth2 - 3);
              typeID += "...";
            } else typeID.resize(typeWidth2, ' ');
            stringStream << typeID << bar;
          } else stringStream << std::setw(typeWidth2);
          stringStream << std::endl << std::dec;
        }
        _peersMutex.unlock();
        stringStream << "─────────┴───────────────────────────┴───────────┴───────────────┴─────────┴───────────────────────────" << std::endl;

        return stringStream.str();
      }
      catch (const std::exception &ex) {
        _peersMutex.unlock();
        Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
    } else if (command.compare(0, 13, "peers setname") == 0 || command.compare(0, 2, "pn") == 0) {
      uint64_t peerID = 0;
      std::string name;

      std::stringstream stream(command);
      std::string element;
      int32_t offset = (command.at(1) == 'n') ? 0 : 1;
      int32_t index = 0;
      while (std::getline(stream, element, ' ')) {
        if (index < 1 + offset) {
          index++;
          continue;
        } else if (index == 1 + offset) {
          if (element == "help") break;
          else {
            peerID = BaseLib::Math::getNumber(element, false);
            if (peerID == 0) return "Invalid id.\n";
          }
        } else if (index == 2 + offset) name = element;
        else name += ' ' + element;
        index++;
      }
      if (index == 1 + offset) {
        stringStream << "Description: This command sets or changes the name of a peer to identify it more easily." << std::endl;
        stringStream << "Usage: peers setname PEERID NAME" << std::endl << std::endl;
        stringStream << "Parameters:" << std::endl;
        stringStream << "  PEERID:\tThe id of the peer to set the name for. Example: 513" << std::endl;
        stringStream << "  NAME:\tThe name to set. Example: \"1st floor light switch\"." << std::endl;
        return stringStream.str();
      }

      if (!peerExists(peerID)) stringStream << "This peer is not paired to this central." << std::endl;
      else {
        std::shared_ptr<KnxPeer> peer = getPeer(peerID);
        peer->setName(name);
        stringStream << "Name set to \"" << name << "\"." << std::endl;
      }
      return stringStream.str();
    } else if (command.compare(0, 6, "search") == 0 || command.compare(0, 2, "sp") == 0) {
      std::stringstream stream(command);
      std::string element;
      int32_t offset = (command.at(1) == 'p') ? 0 : 1;
      int32_t index = 0;
      while (std::getline(stream, element, ' ')) {
        if (index < 1 + offset) {
          index++;
          continue;
        } else if (index == 1 + offset) {
          if (element == "help") {
            stringStream << "Description: This command imports new or changed devices from ETS project files in \"" + _bl->settings.deviceDescriptionPath() + std::to_string(MY_FAMILY_ID) + "/\"." << std::endl;
            stringStream << "Usage: search" << std::endl << std::endl;
            stringStream << "Parameters:" << std::endl;
            stringStream << "  There are no parameters." << std::endl;
            return stringStream.str();
          }
        }
        index++;
      }

      PVariable result = searchDevices(nullptr, "");
      if (result->errorStruct) stringStream << "Error: " << result->structValue->at("faultString")->stringValue << std::endl;
      else stringStream << "Search completed successfully." << std::endl;
      return stringStream.str();
    } else if (command == "test") {
      //auto rawPacket = BaseLib::HelperFunctions::getUBinary("061004200018044D02001100BCE00000210A0400800B3500");
      auto rawPacket = BaseLib::HelperFunctions::getUBinary("06100420001504095E002900BCE011540047010081");
      PKnxIpPacket packet = std::make_shared<KnxIpPacket>(KnxIpPacket(rawPacket));
      std::string interface = "MyInterface";
      auto packetData = packet->getTunnelingRequest();
      if (packetData) {
        onPacketReceived(interface, packetData->cemi);
        stringStream << "Hallo" << std::endl;
      }
      return stringStream.str();
    } else return "Unknown command.\n";
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return "Error executing command. See log file for more details.\n";
}

std::shared_ptr<KnxPeer> KnxCentral::createPeer(uint64_t deviceType, int32_t address, std::string serialNumber, bool save) {
  try {
    std::shared_ptr<KnxPeer> peer(new KnxPeer(_deviceId, this));
    peer->setDeviceType(deviceType);
    peer->setAddress(address);
    peer->setSerialNumber(std::move(serialNumber));
    peer->setRpcDevice(Gd::family->getRpcDevices()->find(deviceType, 0x10, -1));
    if (!peer->getRpcDevice()) return std::shared_ptr<KnxPeer>();
    if (save) peer->save(true, true, false); //Save and create peerID
    return peer;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::shared_ptr<KnxPeer>();
}

PVariable KnxCentral::deleteDevice(BaseLib::PRpcClientInfo clientInfo, std::string serialNumber, int32_t flags) {
  try {
    if (serialNumber.empty()) return Variable::createError(-2, "Unknown device.");

    uint64_t peerId = 0;

    {
      std::shared_ptr<KnxPeer> peer = getPeer(serialNumber);
      if (!peer) return std::make_shared<Variable>(VariableType::tVoid);
      peerId = peer->getID();
    }

    return deleteDevice(clientInfo, peerId, flags);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Variable::createError(-32500, "Unknown application error.");
}

PVariable KnxCentral::deleteDevice(BaseLib::PRpcClientInfo clientInfo, uint64_t peerId, int32_t flags) {
  try {
    if (peerId == 0) return Variable::createError(-2, "Unknown device.");

    {
      std::shared_ptr<KnxPeer> peer = getPeer(peerId);
      if (!peer) return std::make_shared<Variable>(VariableType::tVoid);
    }

    deletePeer(peerId);

    if (peerExists(peerId)) return Variable::createError(-1, "Error deleting peer. See log for more details.");

    return std::make_shared<Variable>(VariableType::tVoid);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Variable::createError(-32500, "Unknown application error.");
}

PVariable KnxCentral::invokeFamilyMethod(BaseLib::PRpcClientInfo clientInfo, std::string &method, PArray parameters) {
  try {
    auto localMethodIterator = _localRpcMethods.find(method);
    if (localMethodIterator != _localRpcMethods.end()) {
      return localMethodIterator->second(clientInfo, parameters);
    } else return BaseLib::Variable::createError(-32601, ": Requested method not found.");
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Variable::createError(-32502, "Unknown application error.");
}

PVariable KnxCentral::searchDevices(BaseLib::PRpcClientInfo clientInfo, const std::string &interfaceId) {
  try {
    std::lock_guard<std::mutex> searchGuard(_searchMutex);

    std::unordered_set<std::string> peersWithoutAutochannels;

    {
      std::lock_guard<std::mutex> peersGuard(_peersMutex);

      for (auto &peer: _peersById) {
        auto rpcDevice = peer.second->getRpcDevice();
        if (rpcDevice->supportedDevices.empty() || !rpcDevice->metadata) continue;
        auto metadataIterator = rpcDevice->metadata->structValue->find("useAutoChannel");
        if (metadataIterator == rpcDevice->metadata->structValue->end() || !metadataIterator->second->booleanValue) peersWithoutAutochannels.emplace(rpcDevice->supportedDevices.front()->id);
      }

      _peers.clear();
      _peersBySerial.clear();
      _peersById.clear();
      _peersByGroupAddress.clear();
    }

    auto usedTypeNumbers = Gd::family->getRpcDevices()->getKnownTypeNumbers();
    auto idTypeNumberMap = Gd::family->getRpcDevices()->getIdTypeNumberMap();

    std::vector<Search::PeerInfo> peerInfo = _search->search(usedTypeNumbers, idTypeNumberMap, peersWithoutAutochannels);
    Gd::out.printInfo("Info: Search completed. Found " + std::to_string(peerInfo.size()) + " devices.");

    return std::make_shared<Variable>(reloadAndUpdatePeers(clientInfo, peerInfo));
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Variable::createError(-32500, "Unknown application error.");
}

PVariable KnxCentral::setInterface(BaseLib::PRpcClientInfo clientInfo, uint64_t peerId, std::string interfaceId) {
  try {
    std::shared_ptr<KnxPeer> peer(getPeer(peerId));
    if (!peer) return Variable::createError(-2, "Unknown device.");
    return peer->setInterface(clientInfo, interfaceId);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Variable::createError(-32500, "Unknown application error.");
}

size_t KnxCentral::reloadAndUpdatePeers(BaseLib::PRpcClientInfo clientInfo, const std::vector<Search::PeerInfo> &peerInfo) {
  try {
    Gd::family->reloadRpcDevices();

    loadPeers();

    std::vector<std::shared_ptr<KnxPeer>> newPeers;
    for (auto &peerInfoElement: peerInfo) {
      {
        std::lock_guard<std::mutex> peersGuard(_peersMutex);
        auto peersIterator = _peersBySerial.find(peerInfoElement.serialNumber);
        if (peersIterator != _peersBySerial.end()) {
          auto myPeer = std::dynamic_pointer_cast<KnxPeer>(peersIterator->second);
          if (peerInfoElement.roomId != 0 && peersIterator->second->getRoom(-1) != peerInfoElement.roomId) {
            peersIterator->second->setRoom(-1, peerInfoElement.roomId);
          }
          if (!peerInfoElement.name.empty() && peersIterator->second->getName() != peerInfoElement.name) {
            auto useKnxProjectDeviceNames = Gd::family->getFamilySetting("useKnxProjectDeviceNames");
            if (useKnxProjectDeviceNames && (bool)useKnxProjectDeviceNames->integerValue) {
              peersIterator->second->setName(peerInfoElement.name);
            }
          } else if (myPeer->getName().empty()) {
            peersIterator->second->setName(myPeer->getFormattedAddress());
          }

          for (auto &roomChannel: peerInfoElement.variableRoomIds) {
            for (auto &variableRoom: roomChannel.second) {
              auto variableName = variableRoom.first;
              auto roomId = myPeer->getVariableRoom(roomChannel.first, variableName);
              if (roomId != variableRoom.second) {
                myPeer->setVariableRoom(roomChannel.first, variableName, variableRoom.second);
              }
            }
          }

          //Always assume that device was updated - we don't know if variables were added or deleted
          raiseRPCUpdateDevice(myPeer->getID(), 0, myPeer->getSerialNumber() + ":" + std::to_string(0), 0);

          continue;
        }
      }
      std::shared_ptr<KnxPeer> peer = createPeer(peerInfoElement.type, peerInfoElement.address, peerInfoElement.serialNumber, true);
      if (!peer) {
        Gd::out.printError("Error: Could not add device with type " + BaseLib::HelperFunctions::getHexString(peerInfoElement.type) + ". No matching XML file was found.");
        continue;
      }

      peer->initializeCentralConfig();
      peer->initParametersByGroupAddress();

      if (!peerInfoElement.name.empty()) peer->setName(peerInfoElement.name);
      else peer->setName(peer->getFormattedAddress());
      if (peerInfoElement.roomId != 0) peer->setRoom(-1, peerInfoElement.roomId);
      for (auto &roomChannel: peerInfoElement.variableRoomIds) {
        for (auto &variableRoom: roomChannel.second) {
          auto variableName = variableRoom.first;
          peer->setVariableRoom(roomChannel.first, variableName, variableRoom.second);
        }
      }

      std::lock_guard<std::mutex> peersGuard(_peersMutex);
      if (peer->getAddress() != -1) _peers[peer->getAddress()] = peer;
      _peersBySerial[peer->getSerialNumber()] = peer;
      _peersById[peer->getID()] = peer;
      std::vector<uint16_t> groupAddresses = peer->getGroupAddresses();
      for (std::vector<uint16_t>::iterator j = groupAddresses.begin(); j != groupAddresses.end(); ++j) {
        auto peersIterator = _peersByGroupAddress.find(*j);
        if (peersIterator == _peersByGroupAddress.end()) _peersByGroupAddress.emplace(*j, std::make_shared<std::map<uint64_t, PKnxPeer>>());
        _peersByGroupAddress[*j]->emplace(peer->getID(), peer);
      }
      newPeers.push_back(peer);
    }

    Gd::out.printInfo("Info: Found " + std::to_string(newPeers.size()) + " new devices.");

    if (!newPeers.empty()) {
      std::vector<uint64_t> newIds;
      newIds.reserve(newPeers.size());
      PVariable deviceDescriptions(new Variable(VariableType::tArray));
      for (std::vector<std::shared_ptr<KnxPeer>>::iterator i = newPeers.begin(); i != newPeers.end(); ++i) {
        std::shared_ptr<std::vector<PVariable>> descriptions = (*i)->getDeviceDescriptions(clientInfo, true, std::map<std::string, bool>());
        if (!descriptions) continue;
        newIds.push_back((*i)->getID());
        for (std::vector<PVariable>::iterator j = descriptions->begin(); j != descriptions->end(); ++j) {
          deviceDescriptions->arrayValue->push_back(*j);
        }
      }
      raiseRPCNewDevices(newIds, deviceDescriptions);
    }

    return newPeers.size();
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return 0;
}

//{{{ Family RPC methods
BaseLib::PVariable KnxCentral::updateDevices(const BaseLib::PRpcClientInfo &clientInfo, const BaseLib::PArray &parameters) {
  try {
    if (parameters->empty()) return BaseLib::Variable::createError(-1, "Wrong parameter count.");
    if (parameters->at(0)->type != BaseLib::VariableType::tArray) return BaseLib::Variable::createError(-1, "Parameter is not of type Array.");

    std::lock_guard<std::mutex> searchGuard(_searchMutex);

    std::vector<Search::PeerInfo> updatedPeersInfo;
    updatedPeersInfo.reserve(parameters->at(0)->arrayValue->size());

    auto usedTypeNumbers = Gd::family->getRpcDevices()->getKnownTypeNumbers();
    auto idTypeNumberMap = Gd::family->getRpcDevices()->getIdTypeNumberMap();

    for (auto &infoStruct: *parameters->at(0)->arrayValue) {

      auto peerInfo = _search->updateDevice(usedTypeNumbers, idTypeNumberMap, infoStruct);
      if (peerInfo.address == -1 || peerInfo.type == -1) {
        Gd::out.printError("Could not create peer. Probably there is an error in the provided data structure. Check previous log messages for more details.");
        continue;
      }

      Gd::out.printInfo("Info: Successfully created peer structure for peer with physical address " + Cemi::getFormattedPhysicalAddress(peerInfo.address) + " and name " + peerInfo.name + ". Type number is 0x"
                            + BaseLib::HelperFunctions::getHexString(peerInfo.type, 4));

      updatedPeersInfo.emplace_back(std::move(peerInfo));
    }
    Gd::out.printInfo("Info: Parsing completed. Found " + std::to_string(updatedPeersInfo.size()) + " devices.");

    //{{{ Remove updated peers
    for (auto &peerInfo: updatedPeersInfo) {
      auto peer = getPeer(peerInfo.serialNumber);
      if (peer) {
        {
          std::lock_guard<std::mutex> lockGuard(_peersMutex);
          if (_peers.find(peer->getAddress()) != _peers.end()) _peers.erase(peer->getAddress());
          if (_peersBySerial.find(peer->getSerialNumber()) != _peersBySerial.end()) _peersBySerial.erase(peer->getSerialNumber());
          if (_peersById.find(peer->getID()) != _peersById.end()) _peersById.erase(peer->getID());
        }

        auto groupAddresses = peer->getGroupAddresses();
        for (const uint16_t &address: groupAddresses) {
          removePeerFromGroupAddresses(address, peer->getID());
        }

        int32_t i = 0;
        while (peer.use_count() > 1 && i < 600) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          i++;
        }
        if (i == 600) Gd::out.printError("Error: Peer deletion took too long.");
      }
    }
    //}}}

    return std::make_shared<Variable>(reloadAndUpdatePeers(std::move(clientInfo), updatedPeersInfo));
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable KnxCentral::groupValueRead(const PRpcClientInfo &clientInfo, const PArray &parameters) {
  try {
    if (parameters->size() != 2) return BaseLib::Variable::createError(-1, "Wrong parameter count.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type String.");

    auto interfaceId = parameters->at(0)->stringValue;
    auto destinationAddress = Cemi::parseGroupAddress(parameters->at(1)->stringValue);

    if (destinationAddress == 0) return Variable::createError(-1, "Invalid group address.");

    auto cemi = std::make_shared<Cemi>(Cemi::Operation::groupValueRead, 0, destinationAddress);

    auto interfaceIterator = Gd::physicalInterfaces.find(interfaceId);
    if (interfaceIterator == Gd::physicalInterfaces.end()) {
      return Variable::createError(-2, "Unknown communication interface.");
    }
    interfaceIterator->second->sendPacket(cemi);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable KnxCentral::groupValueWrite(const PRpcClientInfo &clientInfo, const PArray &parameters) {
  try {
    if (parameters->size() != 4) return BaseLib::Variable::createError(-1, "Wrong parameter count.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type String.");
    if (parameters->at(1)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type String.");
    if (parameters->at(2)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 3 is not of type String.");

    DptConverter dptConverter(_bl);

    auto interfaceId = parameters->at(0)->stringValue;
    auto destinationAddress = Cemi::parseGroupAddress(parameters->at(1)->stringValue);
    auto dpt = BaseLib::HelperFunctions::toUpper(parameters->at(2)->stringValue);
    auto value = dptConverter.getDpt(dpt, parameters->at(3), BaseLib::Role());

    if (destinationAddress == 0) return Variable::createError(-1, "Invalid group address.");

    auto cemi = std::make_shared<Cemi>(Cemi::Operation::groupValueWrite, 0, destinationAddress, dptConverter.fitsInFirstByte(dpt), value);

    auto interfaceIterator = Gd::physicalInterfaces.find(interfaceId);
    if (interfaceIterator == Gd::physicalInterfaces.end()) {
      return Variable::createError(-2, "Unknown communication interface.");
    }
    interfaceIterator->second->sendPacket(cemi);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Variable::createError(-32500, "Unknown application error.");
}
//}}}

}
