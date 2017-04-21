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

#ifndef MAININTERFACE_H_
#define MAININTERFACE_H_

#include "../MyPacket.h"
#include <homegear-base/BaseLib.h>

namespace MyFamily {

class MainInterface : public BaseLib::Systems::IPhysicalInterface
{
public:
	MainInterface(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings);
	virtual ~MainInterface();

	void startListening();
	void stopListening();

	bool isOpen() { return _socket->isOpen(); }

	void sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet);
protected:
	std::map<char, std::string> _connectResponseStatusCodes;
	std::map<char, std::string> _connectionStateResponseStatusCodes;
	std::map<char, std::string> _tunnelingAckStatusCodes;

	class Request
	{
	public:
		std::mutex mutex;
		std::condition_variable conditionVariable;
		bool mutexReady = false;
		std::vector<char> response;

		Request() {}
		virtual ~Request() {}
	private:
	};

	BaseLib::Output _out;
	std::atomic_bool _initComplete;
	std::string _port;
	std::string _listenIp;
	char _listenIpBytes[4];
	char _listenPortBytes[2];
	std::atomic_char _knxAddress;
	std::atomic_char _channelId;
	std::unique_ptr<BaseLib::UdpSocket> _socket;

	std::mutex _sendPacketMutex;
	std::mutex _getResponseMutex;

	std::mutex _requestsMutex;
	std::map<uint32_t, std::shared_ptr<Request>> _requests;

	std::atomic_char _sequenceCounter;
	int64_t _lastConnectionState = 0;
	std::thread _keepAliveThread;
	std::thread _initThread;

	void setListenAddress();
	void reconnect();
	void init();
	void listen();
	void processPacket(std::vector<char>& data);
	void getSystemResponse(uint16_t serviceType, const std::vector<char>& requestPacket, std::vector<char>& responsePacket);
	void sendAck(char sequenceCounter, char error);
	void sendDisconnectResponse(char sequenceCounter, char error);
	bool getConnectionState();
};

}

#endif
