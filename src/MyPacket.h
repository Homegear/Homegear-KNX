/* Copyright 2013-2017 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

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
