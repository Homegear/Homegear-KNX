/* Copyright 2013-2019 Homegear GmbH */

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

        MyPacket();
        MyPacket(std::vector<char>& binaryPacket);
        MyPacket(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress);
        MyPacket(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress, bool payloadFitsInFirstByte, std::vector<uint8_t>& payload);
        MyPacket(Operation operation, uint16_t sourceAddress, uint16_t destinationAddress, uint8_t tpduSequenceNumber, bool payloadFitsInFirstByte, std::vector<uint8_t>& payload);
        virtual ~MyPacket();

        std::vector<char> getBinary(char channelId, char sequenceCounter);
        uint16_t getSourceAddress() { return _sourceAddress; }
        uint16_t getDestinationAddress() { return _destinationAddress; }
        Operation getOperation() { return _operation; }
        std::string getOperationString();
        static std::string getFormattedPhysicalAddress(int32_t address);
        std::string getFormattedSourceAddress() { return getFormattedPhysicalAddress(_sourceAddress); }
	    static std::string getFormattedGroupAddress(int32_t address);
        std::string getFormattedDestinationAddress() { return getFormattedGroupAddress(_destinationAddress); }
        std::vector<uint8_t>& getPayload() { return _payload; }
    protected:
        Operation _operation = Operation::groupValueWrite;
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
