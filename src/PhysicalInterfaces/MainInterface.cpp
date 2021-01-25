/* Copyright 2013-2019 Homegear GmbH */

#include "MainInterface.h"
#include "../Gd.h"
#include "../Cemi.h"
#include "../KnxIpPacket.h"

namespace Knx {

MainInterface::MainInterface(const std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> &settings) : IPhysicalInterface(Gd::bl, Gd::family->getFamily(), settings) {
  _out.init(Gd::bl);
  _out.setPrefix(Gd::out.getPrefix() + "KNXNet/IP \"" + settings->id + "\": ");

  signal(SIGPIPE, SIG_IGN);

  _socket = std::unique_ptr<BaseLib::UdpSocket>(new BaseLib::UdpSocket(_bl));

  if (settings->listenThreadPriority == -1) {
    settings->listenThreadPriority = 45;
    settings->listenThreadPolicy = SCHED_FIFO;
  }

  _stopped = true;
  _sequenceCounter = 0;
  _initComplete = false;
  _knxAddress = 0;
  _channelId = 0;
}

MainInterface::~MainInterface() {
  try {
    _stopCallbackThread = true;
    Gd::bl->threadManager.join(_keepAliveThread);
    Gd::bl->threadManager.join(_listenThread);
    Gd::bl->threadManager.join(_initThread);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

uint8_t MainInterface::getChannelId() {
  return _channelId;
}

uint8_t MainInterface::getManagementChannelId() {
  return _managementChannelId;
}

uint8_t MainInterface::getSequenceCounter() {
  return _sequenceCounter;
}

void MainInterface::incrementSequenceCounter() {
  _sequenceCounter++;
}

uint8_t MainInterface::getManagementSequenceCounter() {
  return _managementSequenceCounter;
}

void MainInterface::incrementManagementSequenceCounter() {
  _managementSequenceCounter++;
}

uint16_t MainInterface::getKnxAddress() {
  return _knxAddress;
}

bool MainInterface::managementConnected() {
  return _managementConnected;
}

std::array<uint8_t, 4> MainInterface::getListenIpBytes() {
  return _listenIpBytes;
}

std::array<uint8_t, 2> MainInterface::getListenPortBytes() {
  return _listenPortBytes;
}

void MainInterface::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet) {
  try {
    if (!packet) {
      _out.printWarning("Warning: Packet was nullptr.");
      return;
    }
    if (!isOpen() || _stopped) {
      _out.printWarning(std::string("Warning: !!!Not!!! sending packet, because device is not connected or opened."));
      return;
    }
    if (_managementConnected) {
      _out.printWarning(std::string("Warning: !!!Not!!! sending packet, because a management connection is open."));
      return;
    }
    std::unique_lock<std::mutex> sendPacketGuard(_sendPacketMutex, std::defer_lock);
    std::unique_lock<std::mutex> requestsGuard(_requestsMutex, std::defer_lock);
    std::lock(sendPacketGuard, requestsGuard);

    //{{{ Prepare requests object
    auto request = std::make_shared<Request>();
    uint32_t serviceType = 0x2E0420;
    _requests[serviceType] = request;
    requestsGuard.unlock();
    std::unique_lock<std::mutex> lock(request->mutex);
    //}}}

    PCemi cemi = std::dynamic_pointer_cast<Cemi>(packet);
    PKnxIpPacket myIpPacket = std::make_shared<KnxIpPacket>(_channelId, _sequenceCounter++, cemi);
    std::vector<uint8_t> data = myIpPacket->getBinary();
    if (data.size() > 200) {
      if (_bl->debugLevel >= 2) _out.printError("Error: Tried to send packet larger than 200 bytes. That is not supported.");
      return;
    }
    std::vector<uint8_t> response;
    getResponse(ServiceType::TUNNELING_ACK, data, response);
    if (response.size() < 10) {
      if (response.empty()) _out.printError("Error: No TUNNELING_ACK packet received: " + BaseLib::HelperFunctions::getHexString(response));
      else _out.printError("Error: TUNNELING_ACK packet is too small: " + BaseLib::HelperFunctions::getHexString(response));
      return;
    }
    if (response.at(9) != (uint8_t)KnxIpErrorCodes::E_NO_ERROR) {
      _out.printError("Error in TUNNELING_ACK (" + std::to_string(response.at(9)) + "): " + KnxIpPacket::getErrorString((KnxIpErrorCodes)response.at(9)));
      return;
    }

    //{{{ Wait for 2E packet
    if (!request->conditionVariable.wait_for(lock, std::chrono::milliseconds(10000), [&] { return request->mutexReady; })) {
      _out.printError("Error: No data control packet received in response to packet.");
    } else if (request->response.size() > 8) sendAck(request->response.at(8), 0);

    requestsGuard.lock();
    _requests.erase(serviceType);
    requestsGuard.unlock();
    //}}}

    _lastPacketSent = BaseLib::HelperFunctions::getTime();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void MainInterface::startListening() {
  try {
    stopListening();
    setListenAddress();
    if (_listenIp.empty()) return;
    _out.printInfo("Info: Listen IP is: " + _listenIp);
    _socket = std::unique_ptr<BaseLib::UdpSocket>(new BaseLib::UdpSocket(_bl, _settings->host, _settings->port, _settings->listenPort));
    _socket->setAutoConnect(true);
    _out.printDebug("Connecting to device with hostname " + _settings->host + " on port " + _settings->port + "...");
    _socket->open();
    _listenPortBytes[0] = (uint8_t)((_socket->getListenPort() >> 8) & 0xFF);
    _listenPortBytes[1] = (uint8_t)(_socket->getListenPort() & 0xFF);
    _hostname = _settings->host;
    _ipAddress = _socket->getClientIp();
    _stopped = false;
    if (_settings->listenThreadPriority > -1) Gd::bl->threadManager.start(_listenThread, true, _settings->listenThreadPriority, _settings->listenThreadPolicy, &MainInterface::listen, this);
    else Gd::bl->threadManager.start(_listenThread, true, &MainInterface::listen, this);
    IPhysicalInterface::startListening();

    init();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void MainInterface::reconnect() {
  try {
    _socket->close();
    _initComplete = false;
    _out.printDebug("Debug: Connecting to device with hostname " + _settings->host + " on port " + _settings->port + "...");
    _socket->open();
    _listenPortBytes[0] = (uint8_t)((_socket->getListenPort() >> 8) & 0xFF);
    _listenPortBytes[1] = (uint8_t)(_socket->getListenPort() & 0xFF);
    _hostname = _settings->host;
    _ipAddress = _socket->getClientIp();
    _stopped = false;
    _out.printInfo("Info: Connected to device with hostname " + _settings->host + " on port " + _settings->port + ".");
    Gd::bl->threadManager.join(_initThread);
    _bl->threadManager.start(_initThread, true, &MainInterface::init, this);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void MainInterface::init() {
  try {
    _sequenceCounter = 0;
    _initComplete = false;

    { // DISCONNECT_REQUEST (just to make sure)
      if (_managementConnected) disconnectManagement();

      std::vector<uint8_t> disconnectPacket{0x06, 0x10, 0x02, 0x09, 0x00, 0x10, _channelId, 0x00, 0x08, 0x01, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2], _listenIpBytes[3], _listenPortBytes[0], _listenPortBytes[1]};
      _out.printInfo("Info: Sending packet " + BaseLib::HelperFunctions::getHexString(disconnectPacket));
      _socket->proofwrite((char *)disconnectPacket.data(), disconnectPacket.size());
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // {{{ CONNECT_REQUEST (0x0205)
    std::vector<uint8_t> data
        {0x06, 0x10, 0x02, 0x05, 0x00, 0x1A, 0x08, 0x01, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2], _listenIpBytes[3], _listenPortBytes[0], _listenPortBytes[1], 0x08, 0x01, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2],
         _listenIpBytes[3], _listenPortBytes[0], _listenPortBytes[1], 0x04, 0x04, 0x02, 0x00};
    std::vector<uint8_t> response;
    getResponse(ServiceType::CONNECT_RESPONSE, data, response);
    if (response.size() < 20) {
      if (response.size() > 7 && response.at(7) != (uint8_t)KnxIpErrorCodes::E_NO_ERROR) {
        _out.printError("Error in CONNECT_RESPONSE (" + std::to_string(response.at(7)) + "): " + KnxIpPacket::getErrorString((KnxIpErrorCodes)response.at(7)));
        _stopped = true;
        return;
      }
      if (response.empty()) _out.printError("Error: No CONNECT_RESPONSE packet received: " + BaseLib::HelperFunctions::getHexString(response));
      else _out.printError("Error: CONNECT_RESPONSE packet is too small: " + BaseLib::HelperFunctions::getHexString(response));
      _stopped = true;
      return;
    }
    if (response.at(17) != 4) {
      _out.printError("Error: Connection is not of the requested type. Does your gateway support the tunneling protocol?");
      _stopped = true;
      return;
    }
    _knxAddress = (((int32_t)(uint8_t)
        response.at(18)) << 8) | (uint8_t)response.at(19);
    _myAddress = _knxAddress;
    _channelId = response.at(6);
    _out.printInfo("Info: Connected. Gateway's KNX address is: " + Cemi::getFormattedPhysicalAddress(_knxAddress));
    // }}}

    _lastConnectionState = BaseLib::HelperFunctions::getTime();
    if (!getConnectionState()) return;

    _initComplete = true;
    _out.printInfo("Info: Init completed.");
    if (_reconnected) _reconnected();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void MainInterface::stopListening() {
  try {
    // {{{ DISCONNECT_REQUEST (0x0209)
    if (!_stopped && _initComplete) {
      std::vector<uint8_t> data{0x06, 0x10, 0x02, 0x09, 0x00, 0x10, _channelId, 0x00, 0x08, 0x01, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2], _listenIpBytes[3], _listenPortBytes[0], _listenPortBytes[1]};
      _out.printInfo("Info: Sending packet " + BaseLib::HelperFunctions::getHexString(data));
      _socket->proofwrite((char *)data.data(), data.size());
      _initComplete = false;
    }
    // }}}

    _stopCallbackThread = true;
    Gd::bl->threadManager.join(_initThread);
    Gd::bl->threadManager.join(_listenThread);
    _stopCallbackThread = false;
    _socket->close();
    _stopped = true;
    IPhysicalInterface::stopListening();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void MainInterface::setListenAddress() {
  try {
    if (!_settings->listenIp.empty() && !BaseLib::Net::isIp(_settings->listenIp)) {
      //Assume address is interface name
      _listenIp = BaseLib::Net::getMyIpAddress(_settings->listenIp);
    } else if (_settings->listenIp.empty()) {
      _listenIp = BaseLib::Net::getMyIpAddress();
      if (_listenIp.empty()) _out.printError("Error: No IP address could be found to bind the server to. Please specify the IP address manually in knx.conf.");
    } else _listenIp = _settings->listenIp;

    std::vector<std::string> ip = BaseLib::HelperFunctions::splitAll(_listenIp, '.');
    if (ip.size() != 4 || !BaseLib::Math::isNumber(ip[0], false) || !BaseLib::Math::isNumber(ip[1], false) || !BaseLib::Math::isNumber(ip[2], false) || !BaseLib::Math::isNumber(ip[3], false)) {
      _listenIp = "";
      _out.printError("Error: IP address is invalid: " + _listenIp);
      return;
    }
    int32_t block1 = BaseLib::Math::getNumber(ip[0], false);
    int32_t block2 = BaseLib::Math::getNumber(ip[1], false);
    int32_t block3 = BaseLib::Math::getNumber(ip[2], false);
    int32_t block4 = BaseLib::Math::getNumber(ip[3], false);
    if (block1 < 0 || block1 > 254 || block2 < 0 || block2 > 254 || block3 < 0 || block3 > 254 || block4 < 0 || block4 > 254) {
      _listenIp = "";
      _out.printError("Error: IP address is invalid: " + _listenIp);
      return;
    }
    _listenIpBytes[0] = (uint8_t)block1;
    _listenIpBytes[1] = (uint8_t)block2;
    _listenIpBytes[2] = (uint8_t)block3;
    _listenIpBytes[3] = (uint8_t)block4;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

bool MainInterface::getConnectionState() {
  try {
    if (!_initComplete) return true;
    std::vector<uint8_t> data{0x06, 0x10, 0x02, 0x07, 0x00, 0x10, _channelId, 0x00, 0x08, 0x01, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2], _listenIpBytes[3], _listenPortBytes[0], _listenPortBytes[1]};
    std::vector<uint8_t> response;
    getResponse(ServiceType::CONNECTIONSTATE_RESPONSE, data, response);
    if (response.size() < 8) {
      if (response.empty()) _out.printError("Error: No CONNECTIONSTATE_RES packet received: " + BaseLib::HelperFunctions::getHexString(response));
      else _out.printError("Error: CONNECTIONSTATE_RES packet is too small: " + BaseLib::HelperFunctions::getHexString(response));
      _stopped = true;
      return false;
    }
    if (response.at(7) != (uint8_t)KnxIpErrorCodes::E_NO_ERROR) {
      _out.printError("Error in CONNECTIONSTATE_RES (" + std::to_string(response.at(7)) + "): " + KnxIpPacket::getErrorString((KnxIpErrorCodes)response.at(7)));
      _stopped = true;
      return false;
    }
    return true;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  _stopped = true;
  return false;
}

void MainInterface::sendAck(uint8_t sequenceCounter, uint8_t error) {
  try {
    std::vector<uint8_t> ack{0x06, 0x10, 0x04, 0x21, 0x00, 0x0A, 0x04, _channelId, sequenceCounter, error};
    if (_bl->debugLevel >= 5) _out.printDebug("Debug: Sending packet " + BaseLib::HelperFunctions::getHexString(ack));
    _socket->proofwrite((char *)ack.data(), ack.size());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void MainInterface::sendDisconnectResponse(KnxIpErrorCodes status, uint8_t channelId) {
  try {
    std::vector<uint8_t> disconnectResponse{0x06, 0x10, 0x02, 0x0A, 0x00, 0x08, channelId, (uint8_t)status};
    if (_bl->debugLevel >= 5) _out.printDebug("Debug: Sending packet " + BaseLib::HelperFunctions::getHexString(disconnectResponse));
    _socket->proofwrite((char *)disconnectResponse.data(), disconnectResponse.size());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void MainInterface::connectManagement() {
  try {
    if (_managementConnected) disconnectManagement();
    _managementSequenceCounter = 0;

    // {{{ CONNECT_REQUEST (0x0205)
    std::vector<uint8_t> data
        {0x06, 0x10, 0x02, 0x05, 0x00, 0x18, 0x08, 0x01, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2], _listenIpBytes[3], _listenPortBytes[0], _listenPortBytes[1], 0x08, 0x01, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2],
         _listenIpBytes[3], _listenPortBytes[0], _listenPortBytes[1], 0x02, 0x03};
    std::vector<uint8_t> response;
    getResponse(ServiceType::CONNECT_RESPONSE, data, response);
    if (response.size() < 18) {
      if (response.size() > 7 && response.at(7) != (uint8_t)KnxIpErrorCodes::E_NO_ERROR) {
        _out.printError("Error in CONNECT_RESPONSE (" + std::to_string(response.at(7)) + "): " + KnxIpPacket::getErrorString((KnxIpErrorCodes)response.at(7)));
        _stopped = true;
        return;
      }
      if (response.empty()) _out.printError("Error: No CONNECT_RESPONSE packet received: " + BaseLib::HelperFunctions::getHexString(response));
      else _out.printError("Error: CONNECT_RESPONSE packet is too small: " + BaseLib::HelperFunctions::getHexString(response));
      return;
    }
    if (response.at(17) != 3) {
      _out.printError("Error: Connection is not of the requested type. Does your gateway support the tunneling protocol?");
      return;
    }
    _managementChannelId = response.at(6);
    _managementConnected = true;
    _out.printInfo("Info: Management connection established.");
    // }}}
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void MainInterface::disconnectManagement() {
  try {
    // {{{ DISCONNECT_REQUEST (0x0205)
    _managementConnected = false;
    std::vector<uint8_t> disconnectPacket{0x06, 0x10, 0x02, 0x09, 0x00, 0x10, _managementChannelId, 0x00, 0x08, 0x01, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2], _listenIpBytes[3], _listenPortBytes[0], _listenPortBytes[1]};
    std::vector<uint8_t> response;
    getResponse(ServiceType::DISCONNECT_RESPONSE, disconnectPacket, response);
    _out.printInfo("Info: Management connection closed.");
    // }}}
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void MainInterface::listen() {
  try {
    std::string ipAddress;
    uint32_t receivedBytes = 0;
    std::array<uint8_t, 2048> buffer{};

    while (!_stopCallbackThread) {
      if (_stopped || !_socket->isOpen()) {
        if (_stopCallbackThread) return;
        if (_stopped) _out.printWarning("Warning: Connection to device closed. Trying to reconnect...");
        _socket->close();
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        if (_stopCallbackThread) return;
        reconnect();
        continue;
      }
      std::vector<uint8_t> data;
      try {
        do {
          receivedBytes = _socket->proofread((char *)buffer.data(), buffer.size(), ipAddress);
          if (ipAddress != _socket->getClientIp() && ipAddress != "224.0.23.12") continue;
          if (receivedBytes > 0) {
            data.insert(data.end(), buffer.data(), buffer.data() + receivedBytes);
            if (data.size() > 1000000) {
              _out.printError("Could not read: Too much data.");
              break;
            }
          }
        } while (receivedBytes == buffer.size());
      }
      catch (const BaseLib::SocketTimeOutException &ex) {
        if (data.empty()) {
          if (BaseLib::HelperFunctions::getTime() - _lastConnectionState > 60000) {
            _lastConnectionState = BaseLib::HelperFunctions::getTime();
            _bl->threadManager.join(_keepAliveThread);
            _bl->threadManager.start(_keepAliveThread, false, &MainInterface::getConnectionState, this);
          }
          continue; //When receivedBytes is exactly 2048 bytes long, proofread will be called again, time out and the packet is received with a delay of 5 seconds. It doesn't matter as packets this big should never be received.
        }
      }
      catch (const BaseLib::SocketClosedException &ex) {
        _stopped = true;
        _out.printWarning("Warning: " + std::string(ex.what()));
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        continue;
      }
      catch (const BaseLib::SocketOperationException &ex) {
        _stopped = true;
        _out.printError("Error: " + std::string(ex.what()));
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        continue;
      }
      if (data.empty() || data.size() > 1000000) continue;

      if (_bl->debugLevel >= 4) _out.printInfo("Info: Packet received. Raw data: " + BaseLib::HelperFunctions::getHexString(data));

      processPacket(data);

      _lastPacketReceived = BaseLib::HelperFunctions::getTime();
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void MainInterface::processPacket(const std::vector<uint8_t> &data) {
  try {
    try {
      auto packet = std::make_shared<KnxIpPacket>(data);

      uint8_t messageCode = 0;
      if (packet->getServiceType() == ServiceType::TUNNELING_REQUEST) {
        auto packetData = packet->getTunnelingRequest();
        if (packetData) messageCode = packetData->cemi->getMessageCode();
      }
      std::unique_lock<std::mutex> requestsGuard(_requestsMutex);
      auto requestIterator = (messageCode == 0x2E) ? _requests.find(0x2E0000 | (uint32_t)packet->getServiceType()) : _requests.find((uint32_t)packet->getServiceType()); //Check for DATA_CONTROL (0x2E, needed in sendPacket)
      if (requestIterator != _requests.end()) {
        std::shared_ptr<Request> request = requestIterator->second;
        requestsGuard.unlock();
        request->response = data;
        {
          std::lock_guard<std::mutex> lock(request->mutex);
          request->mutexReady = true;
        }
        request->conditionVariable.notify_one();
        return;
      } else requestsGuard.unlock();

      if (packet->getServiceType() == ServiceType::TUNNELING_REQUEST) {
        auto packetData = packet->getTunnelingRequest();
        if (packetData) {
          sendAck(packetData->sequenceCounter, 0);
          if (packetData->cemi->getMessageCode() == 0x29) //DATA_IND (0x29)
          {
            raisePacketReceived(packetData->cemi);
          }
        }
      }
      if (packet->getServiceType() == ServiceType::DISCONNECT_REQUEST) //DISCONNECT_REQUEST
      {
        _out.printInfo("Info: Disconnect request received. Disconnecting...");
        auto packetData = packet->getDisconnectRequest();
        if (packetData) {
          if (packetData->channelId == _managementChannelId) {
            auto status = KnxIpErrorCodes::E_NO_ERROR;
            sendDisconnectResponse(status, packetData->channelId);
          } else {
            auto status = packetData->channelId == _channelId ? KnxIpErrorCodes::E_NO_ERROR : KnxIpErrorCodes::E_CONNECTION_ID;
            sendDisconnectResponse(status, packetData->channelId);
            _stopped = true;
          }
        }
      }

      if (_packetReceivedCallback) _packetReceivedCallback(packet);
    }
    catch (const InvalidKnxIpPacketException &ex) {
      _out.printWarning("Warning: Invalid KNX/IP packet received: " + BaseLib::HelperFunctions::getHexString(data));
    }
    catch (const InvalidKnxPacketException &ex) {
      _out.printWarning("Warning: Invalid KNX packet received: " + BaseLib::HelperFunctions::getHexString(data));
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void MainInterface::getResponse(ServiceType serviceType, const std::vector<uint8_t> &requestPacket, std::vector<uint8_t> &responsePacket, int32_t timeout) {
  try {
    if (_stopped) return;
    responsePacket.clear();

    std::unique_lock<std::mutex> getResponseGuard(_getResponseMutex, std::defer_lock);
    std::unique_lock<std::mutex> requestsGuard(_requestsMutex, std::defer_lock);
    std::lock(getResponseGuard, requestsGuard);
    auto request = std::make_shared<Request>();
    _requests[(uint32_t)serviceType] = request;
    requestsGuard.unlock();
    std::unique_lock<std::mutex> lock(request->mutex);

    try {
      _out.printInfo("Info: Sending packet " + BaseLib::HelperFunctions::getHexString(requestPacket));
      _socket->proofwrite((char *)requestPacket.data(), requestPacket.size());
    }
    catch (const BaseLib::SocketOperationException &ex) {
      _out.printError("Error sending packet to gateway: " + std::string(ex.what()));
      return;
    }

    if (!request->conditionVariable.wait_for(lock, std::chrono::milliseconds(timeout), [&] { return request->mutexReady || _stopCallbackThread; })) {
      if (timeout > 1000) {
        _out.printError("Error: No response received to packet: " + BaseLib::HelperFunctions::getHexString(requestPacket));
        _stopped = true; //Force reconnect
      } else _out.printInfo("Info: No response received to packet: " + BaseLib::HelperFunctions::getHexString(requestPacket));
    }
    responsePacket = request->response;

    requestsGuard.lock();
    _requests.erase((uint32_t)serviceType);
    requestsGuard.unlock();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void MainInterface::sendRaw(const std::vector<uint8_t> &packet) {
  try {
    if (_stopped) return;
    try {
      _out.printInfo("Info: Sending raw packet " + BaseLib::HelperFunctions::getHexString(packet));
      _socket->proofwrite((char *)packet.data(), packet.size());
    }
    catch (const BaseLib::SocketOperationException &ex) {
      _out.printError("Error sending packet to gateway: " + std::string(ex.what()));
      return;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

std::unique_lock<std::mutex> MainInterface::getSendPacketLock() {
  return std::unique_lock<std::mutex>(_sendPacketMutex, std::defer_lock);
}

}
