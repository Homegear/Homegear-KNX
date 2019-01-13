/* Copyright 2013-2019 Homegear GmbH */

#include "MyPacket.h"

#include "GD.h"

namespace MyFamily
{
MyPacket::MyPacket()
{
}

MyPacket::MyPacket(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress) : _operation(operation), _sourceAddress(sourceAddress), _destinationAddress(destinationAddress)
{
	_payload.push_back(0);
	_payloadFitsInFirstByte = true;
}

MyPacket::MyPacket(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress, bool payloadFitsInFirstByte, std::vector<uint8_t>& payload) : _operation(operation), _sourceAddress(sourceAddress), _destinationAddress(destinationAddress), _payloadFitsInFirstByte(payloadFitsInFirstByte), _payload(payload)
{
	if(_payload.empty())
	{
		_payload.push_back(0);
		_payloadFitsInFirstByte = true;
	}
}

MyPacket::MyPacket(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress, uint8_t tpduSequenceNumber, bool payloadFitsInFirstByte, std::vector<uint8_t>& payload) : _operation(operation), _sourceAddress(sourceAddress), _destinationAddress(destinationAddress), _numbered(true), _tpduSequenceNumber(tpduSequenceNumber), _payloadFitsInFirstByte(payloadFitsInFirstByte), _payload(payload)
{
	if(_payload.empty())
	{
		_payload.push_back(0);
		_payloadFitsInFirstByte = true;
	}
}

MyPacket::MyPacket(std::vector<char>& binaryPacket)
{
	if(binaryPacket.size() < 21) return;
	_sourceAddress = (((uint16_t)(uint8_t)binaryPacket[14]) << 8) | (uint8_t)binaryPacket[15];
	_destinationAddress = (((uint16_t)(uint8_t)binaryPacket[16]) << 8) | (uint8_t)binaryPacket[17];
    _operation = (Operation)(((binaryPacket[19] & 0x03) << 2) | ((binaryPacket[20] & 0xC0) >> 6));
	if(binaryPacket.size() == 21) _payload.push_back((uint8_t)(binaryPacket.at(20) & 0x3F));
	else _payload.insert(_payload.end(), (char*)(binaryPacket.data() + 21), (char*)(binaryPacket.data() + binaryPacket.size()));
}

MyPacket::~MyPacket()
{
	_payload.clear();
}

std::string MyPacket::getOperationString()
{
    switch(_operation)
    {
        case Operation::groupValueRead:
            return "GroupValueRead";
        case Operation::groupValueResponse:
            return "GroupValueResponse";
        case Operation::groupValueWrite:
            return "GroupValueWrite";
        case Operation::individualAddressWrite:
            return "IndividualAddressWrite";
        case Operation::individualAddressRequest:
            return "IndividualAddressRequest";
        case Operation::individualAddressResponse:
            return "IndividualAddressResponse";
        case Operation::adcRead:
            return "AdcRead";
        case Operation::adcResponse:
            return "AdcResponse";
        case Operation::memoryRead:
            return "MemoryRead";
        case Operation::memoryResponse:
            return "MemoryResponse";
        case Operation::memoryWrite:
            return "MemoryWrite";
        case Operation::userMessage:
            return "UserMessage";
        case Operation::maskVersionRead:
            return "MaskVersionRead";
        case Operation::maskVersionResponse:
            return "MaskVersionResponse";
        case Operation::restart:
            return "Restart";
        case Operation::escape:
            return "Escape";
    }

	return "";
}

std::vector<char> MyPacket::getBinary(char channelId, char sequenceCounter)
{
	std::vector<char> packet;
	try
	{
		uint32_t size = 21 + (_payloadFitsInFirstByte ? 0 : (uint32_t)_payload.size());
		packet.reserve(21 + _payload.size());

		//{{{ Header
			packet.push_back(0x06); //Header size
			packet.push_back(0x10); //Protocol version
			packet.push_back(0x04); //TUNNELING_REQUEST
			packet.push_back(0x20); //TUNNELING_REQUEST
			packet.push_back((char)(uint8_t)(size >> 8));
			packet.push_back((char)(uint8_t)(size & 0xFF));
		//}}}

		//{{{ Body
			packet.push_back(0x04); //Structure length
			packet.push_back(channelId);
			packet.push_back(sequenceCounter);
			packet.push_back(0); //Reserved

			//{{{ cEMI
				/*
				Controlfield 1: 0xbc
					1... .... = Frametype: 1
					..1. .... = Repeat: 0
					...1 .... = System-Broadcast: 0
					.... 11.. = Priority: 0x03
					.... ..0. = Acknowledge-Request: 0
					.... ...0 = Confirm-Flag: 0
				Controlfield 2: 0xe0
					1... .... = Destination address type: 1
					.110 .... = Hop count: 6
					.... 0000 = Extended Frame Format: 0x00
				*/

				packet.push_back(0x11); //Message code (L_Data.req)
				packet.push_back(0); //Add information length
				packet.push_back((char)(uint8_t)0xBC); //Controlfield 1
				packet.push_back((char)(uint8_t)0xE0); //Controlfiled 2
				packet.push_back((char)(uint8_t)(_sourceAddress >> 8));
				packet.push_back((char)(uint8_t)(_sourceAddress & 0xFF));
				packet.push_back((char)(uint8_t)(_destinationAddress >> 8));
				packet.push_back((char)(uint8_t)(_destinationAddress & 0xFF));
				packet.push_back((char)(uint8_t)(_payloadFitsInFirstByte ? 1 : 1 + _payload.size()));
				packet.push_back(((char)(uint8_t)(_numbered ? 0x40 | ((_tpduSequenceNumber & 0xF) << 2) : 0)) | (((char)(uint8_t)(_operation)) >> 2)); //TPCI: UDP or NDP
				if(_payloadFitsInFirstByte) packet.push_back(((char)(((uint8_t)_operation) & 0x03) << 6) | _payload.at(0));
				else
				{
					packet.push_back((char)(((uint8_t)_operation) & 0x03) << 6);
					if(!_payload.empty()) packet.insert(packet.end(), _payload.begin(), _payload.end());
				}
			//}}}
		//}}}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return packet;
}

std::string MyPacket::getFormattedPhysicalAddress(int32_t address)
{
	if(address == -1) return "";
	return std::to_string(address >> 12) + '.' + std::to_string((address >> 8) & 0x0F) + '.' + std::to_string(address & 0xFF);
}

std::string MyPacket::getFormattedGroupAddress(int32_t address)
{
    return std::to_string(address >> 11) + "/" + std::to_string((address >> 8) & 0x7) + "/" + std::to_string(address & 0xFF);
}

}
