/* Copyright 2013-2019 Homegear GmbH */

#include "Cemi.h"

#include "GD.h"

namespace Knx {

Cemi::Cemi(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress) : _operation(operation), _sourceAddress(sourceAddress), _destinationAddress(destinationAddress) {
  _messageCode = 0x11;
  _payload.push_back(0);
  _payloadFitsInFirstByte = true;
}

Cemi::Cemi(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress, bool payloadFitsInFirstByte, std::vector<uint8_t> &payload)
    : _operation(operation), _sourceAddress(sourceAddress), _destinationAddress(destinationAddress), _payloadFitsInFirstByte(payloadFitsInFirstByte), _payload(payload) {
  _messageCode = 0x11;
  if (_payload.empty()) {
    _payload.push_back(0);
    _payloadFitsInFirstByte = true;
  }
}

Cemi::Cemi(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress, uint8_t tpduSequenceNumber, bool payloadFitsInFirstByte, std::vector<uint8_t> &payload)
    : _operation(operation), _sourceAddress(sourceAddress), _destinationAddress(destinationAddress), _numbered(true), _tpduSequenceNumber(tpduSequenceNumber), _payloadFitsInFirstByte(payloadFitsInFirstByte), _payload(payload) {
  _messageCode = 0x11;
  if (_payload.empty()) {
    _payload.push_back(0);
    _payloadFitsInFirstByte = true;
  }
}

Cemi::Cemi(const std::vector<uint8_t> &binaryPacket) {
  if (binaryPacket.size() < 1) throw InvalidKnxPacketException("Too small packet.");
  //Message always starts with the message code (section 4.1.3.1 of chapter 3.6.3)
  _messageCode = binaryPacket[0];
  if (_messageCode == 0x11 || _messageCode == 0x29) {
    if (binaryPacket.size() >= 11) {
      int32_t additionalInformationLength = binaryPacket[1]; //Always there (section 4.1.4.1 of chapter 3.6.3), except for local device management. Can be ignored, if we are not interested.
      if ((signed)binaryPacket.size() < 11 + additionalInformationLength) throw InvalidKnxPacketException("Too small packet.");
      _sourceAddress = (((uint16_t)(uint8_t)binaryPacket[4 + additionalInformationLength]) << 8) | (uint8_t)binaryPacket[5 + additionalInformationLength];
      _destinationAddress = (((uint16_t)(uint8_t)binaryPacket[6 + additionalInformationLength]) << 8) | (uint8_t)binaryPacket[7 + additionalInformationLength];
      _operation = (Operation)(((binaryPacket[9 + additionalInformationLength] & 0x03) << 2) | ((binaryPacket[10 + additionalInformationLength] & 0xC0) >> 6));
      if ((signed)binaryPacket.size() == 11 + additionalInformationLength) _payload.push_back((uint8_t)(binaryPacket.at(10 + additionalInformationLength) & 0x3F));
      else _payload.insert(_payload.end(), binaryPacket.begin() + 11 + additionalInformationLength, binaryPacket.end());
    }
  }

  _rawPacket = binaryPacket;
}

std::string Cemi::getOperationString() {
  switch (_operation) {
    case Operation::unset:return "Unset";
    case Operation::groupValueRead:return "GroupValueRead";
    case Operation::groupValueResponse:return "GroupValueResponse";
    case Operation::groupValueWrite:return "GroupValueWrite";
    case Operation::individualAddressWrite:return "IndividualAddressWrite";
    case Operation::individualAddressRequest:return "IndividualAddressRequest";
    case Operation::individualAddressResponse:return "IndividualAddressResponse";
    case Operation::adcRead:return "AdcRead";
    case Operation::adcResponse:return "AdcResponse";
    case Operation::memoryRead:return "MemoryRead";
    case Operation::memoryResponse:return "MemoryResponse";
    case Operation::memoryWrite:return "MemoryWrite";
    case Operation::userMessage:return "UserMessage";
    case Operation::maskVersionRead:return "MaskVersionRead";
    case Operation::maskVersionResponse:return "MaskVersionResponse";
    case Operation::restart:return "Restart";
    case Operation::escape:return "Escape";
  }

  return "";
}

std::vector<uint8_t> Cemi::getBinary() {
  if (!_rawPacket.empty()) return _rawPacket;

  if (_operation == Operation::unset) return std::vector<uint8_t>();

  std::vector<uint8_t> packet;
  uint32_t size = 11 + (_payloadFitsInFirstByte ? 0 : (uint32_t)_payload.size());
  packet.reserve(size);

  //{{{ cEMI
  /*
  Controlfield 1: 0xbc (section 4.1.5.3.2 of chapter 3.6.3)
      1... .... = Frametype: 1 (0: extended frame, 1: standard frame)
      .0.. .... = Always 0
      ..1. .... = Repeat: 0 (0: repeat frame on medium if error, 1: do not repeat)
      ...1 .... = System-Broadcast: 0 (This shall specify whether the frame is transmitted using system broadcast communication mode or broadcast communication mode (applicable only on open media))
      .... 01.. = Priority: 1 (3 => low, 1 => normal, 2 => urgent, 0 => system)
      .... ..0. = Acknowledge-Request: 0 (This shall specify whether a L2-acknowledge shall be requested for the L_Data.req frame or not. This is not valid for all media.)
      .... ...0 = Confirm-Flag: 0 (In L_Data.con this shall indicate whether there has been any error in the transmitted frame.)
  Controlfield 2: 0xe0
      1... .... = Destination address type: 1 (0: individual, 1: group)
      .110 .... = Hop count: 6 (If the RC is zero on reception of the Frame from the KNX Subnetwork or IP network, then the received KNX telegram shall not be routed. If the RC is seven on reception of the Frame from the KNX Subnetwork or IP network, then the telegram shall be routed without decrementing the RC.)
      .... 0000 = 0 (0b0000 for standard frame, 0b01xx for LTE frames)
  */

  packet.push_back(_messageCode); //Message code (L_Data.req)
  packet.push_back(0); //Additional information length
  packet.push_back((char)(uint8_t)0xB4); //Controlfield 1
  packet.push_back((char)(uint8_t)0xE0); //Controlfiled 2
  packet.push_back((char)(uint8_t)(_sourceAddress >> 8));
  packet.push_back((char)(uint8_t)(_sourceAddress & 0xFF));
  packet.push_back((char)(uint8_t)(_destinationAddress >> 8));
  packet.push_back((char)(uint8_t)(_destinationAddress & 0xFF));
  packet.push_back((char)(uint8_t)(_payloadFitsInFirstByte ? 1 : 1 + _payload.size()));
  packet.push_back(((char)(uint8_t)(_numbered ? 0x40 | ((_tpduSequenceNumber & 0xF) << 2) : 0)) | (((char)(uint8_t)(_operation)) >> 2)); //TPCI: UDP or NDP
  if (_payloadFitsInFirstByte) packet.push_back(((char)(((uint8_t)_operation) & 0x03) << 6) | _payload.at(0));
  else {
    packet.push_back((char)(((uint8_t)_operation) & 0x03) << 6);
    if (!_payload.empty()) packet.insert(packet.end(), _payload.begin(), _payload.end());
  }
  //}}}

  _rawPacket = std::move(packet);
  return _rawPacket;
}

std::string Cemi::getFormattedPhysicalAddress(uint16_t address) {
  if (address == -1) return "";
  return std::to_string(address >> 12) + '.' + std::to_string((address >> 8) & 0x0F) + '.' + std::to_string(address & 0xFF);
}

uint16_t Cemi::parsePhysicalAddress(const std::string &address) {
  auto addressParts = BaseLib::HelperFunctions::splitAll(address, '.');
  if (addressParts.size() != 3) return 0;
  return ((BaseLib::Math::getUnsignedNumber(addressParts.at(0)) & 0x0F) << 12) | ((BaseLib::Math::getUnsignedNumber(addressParts.at(1)) & 0x0F) << 8) | (BaseLib::Math::getUnsignedNumber(addressParts.at(2)) & 0xFF);
}

std::string Cemi::getFormattedGroupAddress(int32_t address) {
  return std::to_string(address >> 11) + "/" + std::to_string((address >> 8) & 0x7) + "/" + std::to_string(address & 0xFF);
}

}
