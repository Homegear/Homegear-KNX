/* Copyright 2013-2019 Homegear GmbH */

#ifndef KNXPACKET_H_
#define KNXPACKET_H_

#include <homegear-base/BaseLib.h>

namespace Knx {

class InvalidKnxPacketException : public BaseLib::Exception {
 public:
  explicit InvalidKnxPacketException(const std::string &message) : Exception(message) {}
};

/**
 * EMI stands for "External Message Interface".
 *
 * See chapter 3.6.3 of the KNX Standard.
 */
class Cemi : public BaseLib::Systems::Packet {
 public:
  enum class Operation : int32_t {
    unset = -1,
    groupValueRead = 0x00,
    groupValueResponse = 0x01,
    groupValueWrite = 0x02,
    individualAddressWrite = 0x03,
    individualAddressRequest = 0x04,
    individualAddressResponse = 0x05,
    adcRead = 0x06,
    adcResponse = 0x07,
    memoryRead = 0x08,
    memoryResponse = 0x09,
    memoryWrite = 0x0A,
    userMessage = 0x0B,
    maskVersionRead = 0x0C,
    maskVersionResponse = 0x0D,
    restart = 0x0E,
    escape = 0x0F //APCI is defined by 6 following bits
  };

  Cemi() = default;
  explicit Cemi(const std::vector<uint8_t> &binaryPacket);
  Cemi(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress);
  Cemi(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress, bool payloadFitsInFirstByte, std::vector<uint8_t> &payload);
  Cemi(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress, uint8_t tpduSequenceNumber, bool payloadFitsInFirstByte, std::vector<uint8_t> &payload);
  virtual ~Cemi() = default;

  std::vector<uint8_t> getBinary();
  uint8_t getMessageCode() { return _messageCode; }
  uint16_t getSourceAddress() { return _sourceAddress; }
  uint16_t getDestinationAddress() { return _destinationAddress; }
  Operation getOperation() { return _operation; }
  std::string getOperationString();
  static std::string getFormattedPhysicalAddress(uint16_t address);
  static uint16_t parsePhysicalAddress(const std::string &address);
  std::string getFormattedSourceAddress() { return getFormattedPhysicalAddress(_sourceAddress); }
  static std::string getFormattedGroupAddress(int32_t address);
  std::string getFormattedDestinationAddress() { return getFormattedGroupAddress(_destinationAddress); }
  std::vector<uint8_t> &getPayload() { return _payload; }
 protected:
  std::vector<uint8_t> _rawPacket;
  uint8_t _messageCode = 0;
  Operation _operation = Operation::unset;
  uint16_t _sourceAddress = 0;
  uint16_t _destinationAddress = 0;

  bool _numbered = false;
  uint8_t _tpduSequenceNumber = 0;
  bool _payloadFitsInFirstByte = false;
  std::vector<uint8_t> _payload;
};

typedef std::shared_ptr<Cemi> PCemi;

}
#endif
