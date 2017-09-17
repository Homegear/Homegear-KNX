/* Copyright 2013-2017 Homegear UG (haftungsbeschränkt) */

#ifndef MYPACKET_H_
#define MYPACKET_H_

#include <homegear-base/BaseLib.h>

namespace MyFamily
{

class MyPacket : public BaseLib::Systems::Packet
{
    public:
		enum class Operation
		{
			read = 0x00,
			write = 0x80
		};

        MyPacket();
        MyPacket(std::vector<char>& binaryPacket);
        MyPacket(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress);
        MyPacket(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress, bool payloadFitsInFirstByte, std::vector<uint8_t>& payload);
        MyPacket(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress, uint8_t tpduSequenceNumber, bool payloadFitsInFirstByte, std::vector<uint8_t>& payload);
        virtual ~MyPacket();

        std::vector<char> getBinary(char channelId, char sequenceCounter);
        uint16_t getSourceAddress() { return _sourceAddress; }
        uint16_t getDestinationAddress() { return _destinationAddress; }
        std::string getFormattedDestinationAddress() { return std::to_string(_destinationAddress >> 11) + "/" + std::to_string((_destinationAddress >> 8) & 0x7) + "/" + std::to_string(_destinationAddress & 0xFF); }
        std::vector<uint8_t>& getPayload() { return _payload; }
    protected:
        Operation _operation = Operation::write;
        uint16_t _sourceAddress = 0;
        uint16_t _destinationAddress = 0;

        bool _numbered = false;
        uint8_t _tpduSequenceNumber = 0;
        bool _payloadFitsInFirstByte = false;
        std::vector<uint8_t> _payload;
};

typedef std::shared_ptr<MyPacket> PMyPacket;

}
#endif
