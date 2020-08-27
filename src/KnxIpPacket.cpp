/* Copyright 2013-2019 Homegear GmbH */

#include "KnxIpPacket.h"
#include "Cemi.h"

namespace Knx {

const std::array<std::string, 0x30> KnxIpPacket::_errorCodes
    {
        // 0x00
        "E_NO_ERROR",
        // 0x01
        "E_HOST_PROTOCOL_TYPE - The requested protocol is not supported by the KNXnet/IP device",
        // 0x02
        "E_VERSION_NOT_SUPPORTED - The requested protocol version is not supported by the KNXnet/IP device",
        // 0x03
        "E_SEQUENCE_NUMBER - The received sequence number is out of order",
        // 0x04
        "",
        // 0x05
        "",
        // 0x06
        "",
        // 0x07
        "",
        // 0x08
        "",
        // 0x09
        "",
        // 0x0A
        "",
        // 0x0B
        "",
        // 0x0C
        "",
        // 0x0D
        "",
        // 0x0E
        "",
        // 0x0F
        "",
        // 0x10
        "",
        // 0x11
        "",
        // 0x12
        "",
        // 0x13
        "",
        // 0x14
        "",
        // 0x15
        "",
        // 0x16
        "",
        // 0x17
        "",
        // 0x18
        "",
        // 0x19
        "",
        // 0x1A
        "",
        // 0x1B
        "",
        // 0x1C
        "",
        // 0x1D
        "",
        // 0x1E
        "",
        // 0x1F
        "",
        // 0x20
        "",
        // 0x21
        "E_CONNECTION_ID - The KNXnet/IP gateway could not find an active data connection with the specified ID",
        // 0x22
        "E_CONNECTION_TYPE - The KNXnet/IP gateway does not support the requested connection type",
        // 0x23
        "E_CONNECTION_OPTION - The KNXnet/IP gateway does not support one or more requested connection options",
        // 0x24
        "E_NO_MORE_CONNECTIONS - The KNXnet/IP gateway is currently in use and does not accept a new data connection",
        // 0x25
        "",
        // 0x26
        "E_DATA_CONNECTION - The KNXnet/IP gateway detected an error concerning the data connection with the specified ID",
        // 0x27
        "E_KNX_CONNECTION - The KNXnet/IP gateway detected an error concerning the EIB bus / KNX subsystem connection with the specified ID",
        // 0x28
        "",
        // 0x29
        "E_TUNNELLING_LAYER - The KNXnet/IP server device does not support the requested tunnelling layer",
        // 0x2A
        "",
        // 0x2B
        "",
        // 0x2C
        "",
        // 0x2D
        "",
        // 0x2E
        "",
        // 0x2F
        "",
    };

KnxIpPacket::KnxIpPacket() {
}

KnxIpPacket::KnxIpPacket(const std::vector<uint8_t> &binaryPacket) {
  if (binaryPacket.size() < 8) throw InvalidKnxIpPacketException("Packet too small.");
  // "This constant with value 06h shall identify the KNXnet/IP header as defined in protocol version 1.0."
  if (binaryPacket.at(0) != 0x06) throw InvalidKnxIpPacketException("Invalid header size.");
  // "This constant with value 10h shall identify the KNXnet/IP protocol version 1.0."
  if (binaryPacket.at(1) != 0x10) throw InvalidKnxIpPacketException("Invalid protocol version.");
  _serviceType = (ServiceType)((((uint16_t)binaryPacket.at(2)) << 8) | binaryPacket.at(3));
  _length = (((uint16_t)binaryPacket.at(4)) << 8) | binaryPacket.at(5);
  if (binaryPacket.size() != _length) throw InvalidKnxIpPacketException("Invalid packet length.");
  if (_serviceType == ServiceType::TUNNELING_REQUEST) {
    if (binaryPacket.size() < 10) throw InvalidKnxIpPacketException("Packet too small.");
    if (binaryPacket.at(6) != 4) throw InvalidKnxIpPacketException("Invalid structure length.");
    _tunnelingRequest = std::make_shared<TunnelingRequest>();
    _tunnelingRequest->channelId = binaryPacket.at(7);
    _tunnelingRequest->sequenceCounter = binaryPacket.at(8);
    //Byte 9 is reserved
    if (binaryPacket.size() >= 10) {
      std::vector<uint8_t> cemi(binaryPacket.begin() + 10, binaryPacket.end());
      _tunnelingRequest->cemi = std::make_shared<Cemi>(cemi);
    } else _tunnelingRequest->cemi = std::make_shared<Cemi>();
  } else if (_serviceType == ServiceType::TUNNELING_ACK) {
    if (binaryPacket.size() < 10) throw InvalidKnxIpPacketException("Packet too small.");
    if (binaryPacket.at(6) != 4) throw InvalidKnxIpPacketException("Invalid structure length.");
    _tunnelingAck = std::make_shared<TunnelingAck>();
    _tunnelingAck->channelId = binaryPacket.at(7);
    _tunnelingAck->sequenceCounter = binaryPacket.at(8);
    _tunnelingAck->status = (KnxIpErrorCodes)binaryPacket.at(9);
  } else if (_serviceType == ServiceType::CONNECT_REQUEST) {
    if (binaryPacket.size() < 24) throw InvalidKnxIpPacketException("Packet too small.");
    _connectRequest = std::make_shared<ConnectRequest>();
    _connectRequest->controlHostProtocolCode = binaryPacket.at(7);
    std::array<uint32_t, 4> ipBytes = {binaryPacket.at(8), binaryPacket.at(9), binaryPacket.at(10), binaryPacket.at(11)};
    _connectRequest->controlEndpointIp = (ipBytes[0] << 24) | (ipBytes[1] << 16) | (ipBytes[2] << 8) | ipBytes[3];
    _connectRequest->controlEndpointIpString = std::to_string(ipBytes[0]).append(".").append(std::to_string(ipBytes[1])).append(".").append(std::to_string(ipBytes[2])).append(".").append(std::to_string(ipBytes[3]));
    _connectRequest->controlEndpointPort = (((uint16_t)binaryPacket.at(12)) << 8) | binaryPacket.at(13);
    _connectRequest->dataHostProtocolCode = binaryPacket.at(15);
    ipBytes = {binaryPacket.at(16), binaryPacket.at(17), binaryPacket.at(18), binaryPacket.at(19)};
    _connectRequest->dataEndpointIp = (ipBytes[0] << 24) | (ipBytes[1] << 16) | (ipBytes[2] << 8) | ipBytes[3];
    _connectRequest->dataEndpointIpString = std::to_string(ipBytes[0]).append(".").append(std::to_string(ipBytes[1])).append(".").append(std::to_string(ipBytes[2])).append(".").append(std::to_string(ipBytes[3]));
    _connectRequest->dataEndpointPort = (((uint16_t)binaryPacket.at(20)) << 8) | binaryPacket.at(21);
    _connectRequest->connectionTypeCode = binaryPacket.at(23);
    if (binaryPacket.size() > 24) {
      _connectRequest->knxLayer = binaryPacket.at(24);
    }
  } else if (_serviceType == ServiceType::CONNECT_RESPONSE) {
    _connectResponse = std::make_shared<ConnectResponse>();
    if (binaryPacket.size() < 8) throw InvalidKnxIpPacketException("Packet too small.");
    _connectResponse->channelId = binaryPacket.at(6);
    _connectResponse->status = (KnxIpErrorCodes)binaryPacket.at(7);
    if (_connectResponse->status == KnxIpErrorCodes::E_NO_ERROR) {
      if (binaryPacket.size() < 18) throw InvalidKnxIpPacketException("Packet too small.");
      _connectResponse->hostProtocolCode = binaryPacket.at(9); //1 = IPv4 UDP, 2 = IPv4 TCP
      std::array<uint32_t, 4> ipBytes = {binaryPacket.at(10), binaryPacket.at(11), binaryPacket.at(12), binaryPacket.at(13)};
      _connectResponse->dataEndpointIp = (ipBytes[0] << 24) | (ipBytes[1] << 16) | (ipBytes[2] << 8) | ipBytes[3];
      _connectResponse->dataEndpointIpString = std::to_string(ipBytes[0]).append(".").append(std::to_string(ipBytes[1])).append(".").append(std::to_string(ipBytes[2])).append(".").append(std::to_string(ipBytes[3]));
      _connectResponse->dataEndpointPort = (((uint16_t)binaryPacket.at(14)) << 8) | binaryPacket.at(15);
      _connectResponse->connectionTypeCode = binaryPacket.at(17);
      if (binaryPacket.size() > 19) {
        _connectResponse->knxAddress = (((uint16_t)binaryPacket.at(18)) << 8) | binaryPacket.at(19);
      }
    }
  } else if (_serviceType == ServiceType::DISCONNECT_REQUEST) {
    if (binaryPacket.size() < 16) throw InvalidKnxIpPacketException("Packet too small.");
    _disconnectRequest = std::make_shared<DisconnectRequest>();
    _disconnectRequest->channelId = binaryPacket.at(6);
    _disconnectRequest->hostProtocolCode = binaryPacket.at(9); //1 = IPv4 UDP, 2 = IPv4 TCP
    std::array<uint32_t, 4> ipBytes = {binaryPacket.at(10), binaryPacket.at(11), binaryPacket.at(12), binaryPacket.at(13)};
    _disconnectRequest->controlEndpointIp = (ipBytes[0] << 24) | (ipBytes[1] << 16) | (ipBytes[2] << 8) | ipBytes[3];
    _disconnectRequest->controlEndpointIpString = std::to_string(ipBytes[0]).append(".").append(std::to_string(ipBytes[1])).append(".").append(std::to_string(ipBytes[2])).append(".").append(std::to_string(ipBytes[3]));
    _disconnectRequest->controlEndpointPort = (((uint16_t)binaryPacket.at(14)) << 8) | binaryPacket.at(15);
  } else if (_serviceType == ServiceType::DISCONNECT_RESPONSE) {
    if (binaryPacket.size() < 8) throw InvalidKnxIpPacketException("Packet too small.");
    _disconnectResponse = std::make_shared<DisconnectResponse>();
    _disconnectResponse->channelId = binaryPacket.at(6);
    _disconnectResponse->status = (KnxIpErrorCodes)binaryPacket.at(7);
  } else if (_serviceType == ServiceType::CONNECTIONSTATE_REQUEST) {
    if (binaryPacket.size() < 16) throw InvalidKnxIpPacketException("Packet too small.");
    _connectionStateRequest = std::make_shared<ConnectionStateRequest>();
    _connectionStateRequest->channelId = binaryPacket.at(6);
    _connectionStateRequest->hostProtocolCode = binaryPacket.at(9); //1 = IPv4 UDP, 2 = IPv4 TCP
    std::array<uint32_t, 4> ipBytes = {binaryPacket.at(10), binaryPacket.at(11), binaryPacket.at(12), binaryPacket.at(13)};
    _connectionStateRequest->controlEndpointIp = (ipBytes[0] << 24) | (ipBytes[1] << 16) | (ipBytes[2] << 8) | ipBytes[3];
    _connectionStateRequest->controlEndpointIpString = std::to_string(ipBytes[0]).append(".").append(std::to_string(ipBytes[1])).append(".").append(std::to_string(ipBytes[2])).append(".").append(std::to_string(ipBytes[3]));
    _connectionStateRequest->controlEndpointPort = (((uint16_t)binaryPacket.at(14)) << 8) | binaryPacket.at(15);
  } else if (_serviceType == ServiceType::CONNECTIONSTATE_RESPONSE) {
    if (binaryPacket.size() < 8) throw InvalidKnxIpPacketException("Packet too small.");
    _connectionStateResponse = std::make_shared<ConnectionStateResponse>();
    _connectionStateResponse->channelId = binaryPacket.at(6);
    _connectionStateResponse->status = (KnxIpErrorCodes)binaryPacket.at(7);
  } else if (_serviceType == ServiceType::CONFIG_REQUEST) {
    if (binaryPacket.size() < 10) throw InvalidKnxIpPacketException("Packet too small.");
    if (binaryPacket.at(6) != 4) throw InvalidKnxIpPacketException("Invalid structure length.");
    _configRequest = std::make_shared<ConfigRequest>();
    _configRequest->channelId = binaryPacket.at(7);
    _configRequest->sequenceCounter = binaryPacket.at(8);
    //Byte 9 is reserved
    if (binaryPacket.size() >= 10) {
      std::vector<uint8_t> cemi(binaryPacket.begin() + 10, binaryPacket.end());
      _configRequest->cemi = std::make_shared<Cemi>(cemi);
    } else _configRequest->cemi = std::make_shared<Cemi>();
  } else if (_serviceType == ServiceType::TUNNELING_ACK) {
    if (binaryPacket.size() < 10) throw InvalidKnxIpPacketException("Packet too small.");
    if (binaryPacket.at(6) != 4) throw InvalidKnxIpPacketException("Invalid structure length.");
    _configAck = std::make_shared<ConfigAck>();
    _configAck->channelId = binaryPacket.at(7);
    _configAck->sequenceCounter = binaryPacket.at(8);
    _configAck->status = (KnxIpErrorCodes)binaryPacket.at(9);
  }

  _rawPacket = binaryPacket;
}

KnxIpPacket::KnxIpPacket(uint8_t channelId, uint8_t sequenceCounter, const PCemi &cemi) : _serviceType(ServiceType::TUNNELING_REQUEST) {
  _tunnelingRequest = std::make_shared<TunnelingRequest>();
  _tunnelingRequest->channelId = channelId;
  _tunnelingRequest->sequenceCounter = sequenceCounter;
  _tunnelingRequest->cemi = cemi;
  if (!_tunnelingRequest->cemi) _tunnelingRequest->cemi = std::make_shared<Cemi>();
}

std::string KnxIpPacket::getErrorString(KnxIpErrorCodes code) {
  if ((uint8_t)code > _errorCodes.size()) return "";
  return _errorCodes.at((uint8_t)code);
}

std::string KnxIpPacket::getServiceIdentifierString() {
  switch (_serviceType) {
    case ServiceType::UNSET:return "UNSET";
    case ServiceType::SEARCH_REQUEST:return "SEARCH_REQUEST";
    case ServiceType::SEARCH_RESPONSE:return "SEARCH_RESPONSE";
    case ServiceType::DESCRIPTION_REQUEST:return "DESCRIPTION_REQUEST";
    case ServiceType::DESCRIPTION_RESPONSE:return "DESCRIPTION_RESPONSE";
    case ServiceType::CONNECT_REQUEST:return "CONNECT_REQUEST";
    case ServiceType::CONNECT_RESPONSE:return "CONNECT_RESPONSE";
    case ServiceType::CONNECTIONSTATE_REQUEST:return "CONNECTIONSTATE_REQUEST";
    case ServiceType::CONNECTIONSTATE_RESPONSE:return "CONNECTIONSTATE_RESPONSE";
    case ServiceType::DISCONNECT_REQUEST:return "DISCONNECT_REQUEST";
    case ServiceType::DISCONNECT_RESPONSE:return "DISCONNECT_RESPONSE";
    case ServiceType::CONFIG_REQUEST:return "CONFIG_REQUEST";
    case ServiceType::CONFIG_ACK:return "CONFIG_ACK";
    case ServiceType::TUNNELING_REQUEST:return "TUNNELING_REQUEST";
    case ServiceType::TUNNELING_ACK:return "TUNNELING_ACK";
    case ServiceType::ROUTING_LOST_MESSAGE:return "ROUTING_LOST_MESSAGE";
    case ServiceType::ROUTING_BUSY:return "ROUTING_BUSY";
  }

  return "";
}

std::vector<uint8_t> KnxIpPacket::getBinary() {
  if (!_rawPacket.empty()) return _rawPacket;
  std::vector<uint8_t> packet;

  if (_serviceType == ServiceType::TUNNELING_REQUEST) //Most probable service type => place at top
  {
    if (!_tunnelingRequest) throw InvalidKnxIpPacketException("Packet is not initialized as this service type.");
    std::vector<uint8_t> cemi = _tunnelingRequest->cemi->getBinary();
    uint16_t size = 10 + cemi.size();
    packet.reserve(size);

    //{{{ KNXnet/IP header
    packet.push_back(0x06); //Header size
    packet.push_back(0x10); //Protocol version
    packet.push_back((uint16_t)_serviceType >> 8);
    packet.push_back((uint16_t)_serviceType & 0xFF);
    packet.push_back((uint8_t)(size >> 8));
    packet.push_back((uint8_t)(size & 0xFF));
    //}}}

    //{{{Connection header
    packet.push_back(0x04); //Structure length
    packet.push_back(_tunnelingRequest->channelId);
    packet.push_back(_tunnelingRequest->sequenceCounter);
    packet.push_back(0); //Reserved
    //}}}

    //{{{ Body
    if (!cemi.empty()) packet.insert(packet.end(), cemi.begin(), cemi.end());
    //}}}
  } else if (_serviceType == ServiceType::TUNNELING_ACK) {
    if (!_tunnelingAck) throw InvalidKnxIpPacketException("Packet is not initialized as this service type.");
    uint16_t size = 10;
    packet.reserve(size);

    //{{{ KNXnet/IP header
    packet.push_back(0x06); //Header size
    packet.push_back(0x10); //Protocol version
    packet.push_back((uint16_t)_serviceType >> 8);
    packet.push_back((uint16_t)_serviceType & 0xFF);
    packet.push_back((uint8_t)(size >> 8));
    packet.push_back((uint8_t)(size & 0xFF));
    //}}}

    //{{{ Connection header
    packet.push_back(0x04); //Structure length
    packet.push_back(_tunnelingAck->channelId);
    packet.push_back(_tunnelingAck->sequenceCounter);
    packet.push_back((uint8_t)KnxIpErrorCodes::E_NO_ERROR);
    //}}}
  }

  _rawPacket = std::move(packet);
  return _rawPacket;
}

void KnxIpPacket::clearBinaryCache() {
  _rawPacket.clear();
}

}