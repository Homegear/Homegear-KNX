/* Copyright 2013-2019 Homegear GmbH */

#ifndef FORWARDER_H_
#define FORWARDER_H_

#include <homegear-base/BaseLib.h>
#include "KnxIpPacket.h"

namespace Knx {

class MainInterface;

class KnxIpForwarder {
 public:
  KnxIpForwarder(std::string listenIp, uint16_t port, std::shared_ptr<MainInterface> interface);
  ~KnxIpForwarder();

  void startListening();
  void stopListening();
 protected:
  BaseLib::Output _out;
  std::shared_ptr<BaseLib::FileDescriptor> _serverSocketDescriptor;

  std::shared_ptr<MainInterface> _interface;
  std::string _listenIpSetting;
  uint16_t _port = 3672;
  std::string _listenIp;
  std::array<uint8_t, 4> _listenIpBytes;
  std::thread _listenThread;
  std::atomic_bool _stopThreads{false};

  std::atomic_bool _connected{false};
  std::atomic_bool _managementConnected{false};
  int64_t _lastPacketReceived = 0;
  std::string _lastSenderIp;
  uint16_t _senderConfigPort = 0;
  uint16_t _senderDataPort = 0;
  uint8_t _channelId = 0;
  uint8_t _managementChannelId = 0;
  uint8_t _sequenceCounterOut = 0;
  uint8_t _managementSequenceCounterOut = 0;
  std::atomic_uchar _lastManagementSequenceCounterIn{0};
  std::atomic_uchar _lastManagementSequenceCounterOut{0};

  void setListenAddress();
  void packetReceivedCallback(const PKnxIpPacket &packet);
  void reconnectedCallback();
  void listen();
  std::shared_ptr<BaseLib::FileDescriptor> getSocketDescriptor();
  void processRawPacket(const std::string &senderIp, uint16_t senderPort, const std::vector<uint8_t> &data);
  void sendPacket(const std::string &destinationIp, uint16_t destinationPort, const PKnxIpPacket &packet, bool forceSending = false);
};

}

#endif
