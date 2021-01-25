/* Copyright 2013-2019 Homegear GmbH */

#ifndef HOMEGEAR_KNX_KNXIPPACKET_H
#define HOMEGEAR_KNX_KNXIPPACKET_H

#include <homegear-base/BaseLib.h>

namespace Knx {

class Cemi;
typedef std::shared_ptr<Cemi> PCemi;

class InvalidKnxIpPacketException : public BaseLib::Exception {
 public:
  explicit InvalidKnxIpPacketException(const std::string &message) : Exception(message) {}
};

enum class ServiceType : uint16_t {
  UNSET = 0,
  SEARCH_REQUEST = 0x0201, //0x02 is the service type "core"
  SEARCH_RESPONSE = 0x0202,
  DESCRIPTION_REQUEST = 0x0203,
  DESCRIPTION_RESPONSE = 0x0204,
  CONNECT_REQUEST = 0x0205,
  CONNECT_RESPONSE = 0x0206,
  CONNECTIONSTATE_REQUEST = 0x0207,
  CONNECTIONSTATE_RESPONSE = 0x0208,
  DISCONNECT_REQUEST = 0x0209,
  DISCONNECT_RESPONSE = 0x020A,

  CONFIG_REQUEST = 0x0310,
  CONFIG_ACK = 0x0311,

  TUNNELING_REQUEST = 0x0420,
  TUNNELING_ACK = 0x0421,

  ROUTING_LOST_MESSAGE = 0x0531,
  ROUTING_BUSY = 0x0532
};

enum class KnxIpErrorCodes : uint8_t {
  E_NO_ERROR = 0x00,
  E_HOST_PROTOCOL_TYPE = 0x01,
  E_VERSION_NOT_SUPPORTED = 0x02,
  E_SEQUENCE_NUMBER = 0x03,
  E_CONNECTION_ID = 0x21,
  E_CONNECTION_TYPE = 0x22,
  E_CONNECTION_OPTION = 0x23,
  E_NO_MORE_CONNECTIONS = 0x24,
  E_DATA_CONNECTION = 0x26,
  E_KNX_CONNECTION = 0x27,
  E_TUNNELLING_LAYER = 0x29
};

/**
 * Describes a KNXnet/IP packet.
 *
 * The general specification of the KNX IP communication is found in section 3.2.6 of the KNX Standard. The frame format
 * and the core packets are described in section 3.8.2 chapter 7, the tunneling packets in section 3.8.4.
 */
class KnxIpPacket : public BaseLib::Systems::Packet {
 public:
  struct ConnectRequest {
    uint8_t controlHostProtocolCode;
    uint32_t controlEndpointIp;
    std::string controlEndpointIpString;
    uint16_t controlEndpointPort;
    uint8_t dataHostProtocolCode;
    uint32_t dataEndpointIp;
    std::string dataEndpointIpString;
    uint16_t dataEndpointPort;
    uint8_t connectionTypeCode;
    uint8_t knxLayer;
  };

  struct ConnectResponse {
    uint8_t channelId;
    KnxIpErrorCodes status;
    uint8_t hostProtocolCode;
    uint32_t dataEndpointIp;
    std::string dataEndpointIpString;
    uint16_t dataEndpointPort;
    uint8_t connectionTypeCode;
    uint16_t knxAddress;
  };

  struct ConnectionStateRequest {
    uint8_t channelId;
    uint8_t hostProtocolCode;
    uint32_t controlEndpointIp;
    std::string controlEndpointIpString;
    uint16_t controlEndpointPort;
  };

  struct ConnectionStateResponse {
    uint8_t channelId;
    KnxIpErrorCodes status;
  };

  struct DisconnectRequest {
    uint8_t channelId;
    uint8_t hostProtocolCode;
    uint32_t controlEndpointIp;
    std::string controlEndpointIpString;
    uint16_t controlEndpointPort;
  };

  struct DisconnectResponse {
    uint8_t channelId;
    KnxIpErrorCodes status;
  };

  struct ConfigRequest {
    uint8_t channelId;
    uint8_t sequenceCounter;
    PCemi cemi;
  };

  struct ConfigAck {
    uint8_t channelId;
    uint8_t sequenceCounter;
    KnxIpErrorCodes status;
  };

  struct TunnelingRequest {
    uint8_t channelId;
    uint8_t sequenceCounter;
    PCemi cemi;
  };

  struct TunnelingAck {
    uint8_t channelId;
    uint8_t sequenceCounter;
    KnxIpErrorCodes status;
  };

  KnxIpPacket();
  explicit KnxIpPacket(const std::vector<uint8_t> &binaryPacket);
  KnxIpPacket(uint8_t channelId, uint8_t sequenceCounter, const PCemi &cemi);
  virtual ~KnxIpPacket() = default;

  BaseLib::PVariable toVariable() override;

  ServiceType getServiceType() { return _serviceType; }
  std::string getServiceIdentifierString();

  std::vector<uint8_t> getBinary();
  void clearBinaryCache();
  static std::string getErrorString(KnxIpErrorCodes code);

  std::shared_ptr<ConnectRequest> getConnectRequest() { return _connectRequest; }
  std::shared_ptr<ConnectResponse> getConnectResponse() { return _connectResponse; }
  std::shared_ptr<ConnectionStateRequest> getConnectionStateRequest() { return _connectionStateRequest; }
  std::shared_ptr<ConnectionStateResponse> getConnectionStateResponse() { return _connectionStateResponse; }
  std::shared_ptr<DisconnectRequest> getDisconnectRequest() { return _disconnectRequest; }
  std::shared_ptr<DisconnectResponse> getDisconnectResponse() { return _disconnectResponse; }

  std::shared_ptr<ConfigRequest> getConfigRequest() { return _configRequest; }
  std::shared_ptr<ConfigAck> getConfigAck() { return _configAck; }

  std::shared_ptr<TunnelingRequest> getTunnelingRequest() { return _tunnelingRequest; }
  std::shared_ptr<TunnelingAck> getTunnelingAck() { return _tunnelingAck; }
 protected:
  std::vector<uint8_t> _rawPacket;
  static const std::array<std::string, 0x30> _errorCodes;
  ServiceType _serviceType = ServiceType::UNSET;
  size_t _length = 0;

  std::shared_ptr<ConnectRequest> _connectRequest;
  std::shared_ptr<ConnectResponse> _connectResponse;
  std::shared_ptr<ConnectionStateRequest> _connectionStateRequest;
  std::shared_ptr<ConnectionStateResponse> _connectionStateResponse;
  std::shared_ptr<DisconnectRequest> _disconnectRequest;
  std::shared_ptr<DisconnectResponse> _disconnectResponse;

  std::shared_ptr<ConfigRequest> _configRequest;
  std::shared_ptr<ConfigAck> _configAck;

  std::shared_ptr<TunnelingRequest> _tunnelingRequest;
  std::shared_ptr<TunnelingAck> _tunnelingAck;
};

typedef std::shared_ptr<KnxIpPacket> PKnxIpPacket;

}

#endif //HOMEGEAR_KNX_KNXIPPACKET_H
