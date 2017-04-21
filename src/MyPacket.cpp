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
	_sourceAddress = ((uint16_t)(uint8_t)binaryPacket[14]) << 8 | binaryPacket[15];
	_destinationAddress = ((uint16_t)(uint8_t)binaryPacket[16]) << 8 | binaryPacket[17];
	if(binaryPacket.size() == 21) _payload.push_back(binaryPacket.at(20) & 0x3F);
	else _payload.insert(_payload.end(), (char*)&binaryPacket.at(21), (char*)&binaryPacket.at(21) + binaryPacket.size() - 21);
}

MyPacket::~MyPacket()
{
	_payload.clear();
}

std::vector<char> MyPacket::getBinary(char channelId, char sequenceCounter)
{
	std::vector<char> packet;
	try
	{
		uint32_t size = 21 + (_payloadFitsInFirstByte ? 0 : _payload.size());
		packet.reserve(21 + _payload.size());

		//{{{ Header
			packet.push_back(0x06); //Header size
			packet.push_back(0x10); //Protocol version
			packet.push_back(0x04); //TUNNELING_REQUEST
			packet.push_back(0x20); //TUNNELING_REQUEST
			packet.push_back(size >> 8);
			packet.push_back(size & 0xFF);
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
				packet.push_back(0xBC); //Controlfield 1
				packet.push_back(0xE0); //Controlfiled 2
				packet.push_back(_sourceAddress >> 8);
				packet.push_back(_sourceAddress & 0xFF);
				packet.push_back(_destinationAddress >> 8);
				packet.push_back(_destinationAddress & 0xFF);
				packet.push_back(_payloadFitsInFirstByte ? 1 : 1 + _payload.size());
				packet.push_back(_numbered ? 0x40 | ((_tpduSequenceNumber & 0xF) << 2) : 0); //TPCI: UDP or NDP
				if(_payloadFitsInFirstByte) packet.push_back(((char)(uint8_t)_operation) | _payload.at(0));
				else
				{
					packet.push_back((char)(uint8_t)_operation);
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

}
