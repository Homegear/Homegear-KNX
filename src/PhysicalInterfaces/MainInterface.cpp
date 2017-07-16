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

#include "MainInterface.h"
#include "../GD.h"

namespace MyFamily
{

MainInterface::MainInterface(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IPhysicalInterface(GD::bl, GD::family->getFamily(), settings)
{
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "KNXNet/IP \"" + settings->id + "\": ");

	signal(SIGPIPE, SIG_IGN);

	_socket = std::unique_ptr<BaseLib::UdpSocket>(new BaseLib::UdpSocket(_bl));

	if(settings->listenThreadPriority == -1)
	{
		settings->listenThreadPriority = 45;
		settings->listenThreadPolicy = SCHED_FIFO;
	}

	_connectResponseStatusCodes[0] = "E_NO_ERROR - The connection was established successfully";
	_connectResponseStatusCodes[0x22] = "E_CONNECTION_TYPE - The KNXnet/IP gateway does not support the requested connection type";
	_connectResponseStatusCodes[0x23] = "E_CONNECTION_OPTION - The KNXnet/IP gateway does not support one or more requested connection options";
	_connectResponseStatusCodes[0x24] = "E_NO_MORE_CONNECTIONS - The KNXnet/IP gateway is currently in use and does not accept a new data connection";

	_connectionStateResponseStatusCodes[0] = "E_NO_ERROR - The connection state is normal";
	_connectionStateResponseStatusCodes[0x21] = "E_CONNECTION_ID - The KNXnet/IP gateway could not find an active data connection with the specified ID";
	_connectionStateResponseStatusCodes[0x26] = "E_DATA_CONNECTION - The KNXnet/IP gateway detected an error concerning the data connection with the specified ID";
	_connectionStateResponseStatusCodes[0x27] = "E_KNX_CONNECTION - The KNXnet/IP gateway detected an error concerning the EIB bus / KNX subsystem connection with the specified ID";

	_tunnelingAckStatusCodes[0] = "E_NO_ERROR - The message was sent or received successfully.";
	_tunnelingAckStatusCodes[0x29] = "E_TUNNELLING_LAYER - The KNXnet/IP server device does not support the requested tunnelling layer.";

	_stopped = true;
	_sequenceCounter = 0;
	_initComplete = false;
	_knxAddress = 0;
	_channelId = 0;
}

MainInterface::~MainInterface()
{
	try
	{
		_stopCallbackThread = true;
		GD::bl->threadManager.join(_keepAliveThread);
		GD::bl->threadManager.join(_listenThread);
		GD::bl->threadManager.join(_initThread);
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void MainInterface::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			_out.printWarning("Warning: Packet was nullptr.");
			return;
		}
		if(!isOpen() || _stopped)
		{
			_out.printWarning(std::string("Warning: !!!Not!!! sending packet, because device is not connected or opened: ") + packet->hexString());
			return;
		}
		if(packet->payload()->size() > 50)
		{
			if(_bl->debugLevel >= 2) _out.printError("Error: Tried to send packet larger than 50 bytes. That is not supported.");
			return;
		}
		std::lock_guard<std::mutex> sendPacketGuard(_sendPacketMutex);

		//{{{ Prepare requests object
			std::shared_ptr<Request> request(new Request());
			uint32_t serviceType = 0x2E0420;
			_requestsMutex.lock();
			_requests[serviceType] = request;
			_requestsMutex.unlock();
			std::unique_lock<std::mutex> lock(request->mutex);
		//}}}

		PMyPacket myPacket = std::dynamic_pointer_cast<MyPacket>(packet);
		std::vector<char> data = myPacket->getBinary(_channelId, _sequenceCounter++);
		std::vector<char> response;
		getSystemResponse(0x0421, data, response);
		if(response.size() < 10)
		{
			if(response.empty()) _out.printError("Error: No TUNNELING_ACK packet received: " + BaseLib::HelperFunctions::getHexString(response));
			else _out.printError("Error: TUNNELING_ACK packet is too small: " + BaseLib::HelperFunctions::getHexString(response));
			return;
		}
		if(response.at(9) != 0)
		{
			std::map<char, std::string>::iterator statusIterator = _tunnelingAckStatusCodes.find(response.at(9));
			if(statusIterator != _tunnelingAckStatusCodes.end()) _out.printError("Error in TUNNELING_ACK: " + statusIterator->second);
			else _out.printError("Unknown error in TUNNELING_ACK: " + BaseLib::HelperFunctions::getHexString(response));
			return;
		}

		//{{{ Wait for 2E packet
			if(!request->conditionVariable.wait_for(lock, std::chrono::milliseconds(10000), [&] { return request->mutexReady; }))
			{
				_out.printError("Error: No data control packet received in response to packet.");
			}
			else if(request->response.size() > 8) sendAck(request->response.at(8), 0);

			_requestsMutex.lock();
			_requests.erase(serviceType);
			_requestsMutex.unlock();
		//}}}

		_lastPacketSent = BaseLib::HelperFunctions::getTime();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void MainInterface::startListening()
{
	try
	{
		stopListening();
		setListenAddress();
		if(_listenIp.empty()) return;
		_out.printInfo("Info: Listen IP is: " + _listenIp);
		_socket = std::unique_ptr<BaseLib::UdpSocket>(new BaseLib::UdpSocket(_bl, _settings->host, _settings->port));
		_socket->setAutoConnect(true);
		_out.printDebug("Connecting to device with hostname " + _settings->host + " on port " + _settings->port + "...");
		_socket->open();
		_listenPortBytes[0] = (char)(uint8_t)((_socket->getListenPort() >> 8) & 0xFF);
		_listenPortBytes[1] = (char)(uint8_t)(_socket->getListenPort() & 0xFF);
		_hostname = _settings->host;
		_ipAddress = _socket->getClientIp();
		_stopped = false;
		if(_settings->listenThreadPriority > -1) GD::bl->threadManager.start(_listenThread, true, _settings->listenThreadPriority, _settings->listenThreadPolicy, &MainInterface::listen, this);
		else GD::bl->threadManager.start(_listenThread, true, &MainInterface::listen, this);
		IPhysicalInterface::startListening();

		init();
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void MainInterface::reconnect()
{
	try
	{
		_socket->close();
		_initComplete = false;
		_out.printDebug("Connecting to device with hostname " + _settings->host + " on port " + _settings->port + "...");
		_socket->open();
		_listenPortBytes[0] = (char)(uint8_t)((_socket->getListenPort() >> 8) & 0xFF);
		_listenPortBytes[1] = (char)(uint8_t)(_socket->getListenPort() & 0xFF);
		_hostname = _settings->host;
		_ipAddress = _socket->getClientIp();
		_stopped = false;
		_out.printInfo("Connected to device with hostname " + _settings->host + " on port " + _settings->port + ".");
		GD::bl->threadManager.join(_initThread);
		_bl->threadManager.start(_initThread, true, &MainInterface::init, this);
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void MainInterface::init()
{
	try
	{
		_sequenceCounter = 0;

		// {{{ CONNECT_REQ (0x0205)
			std::vector<char> data{ 0x06, 0x10, 0x02, 0x05, 0x00, 0x1A, 0x08, 0x01, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2], _listenIpBytes[3], _listenPortBytes[0], _listenPortBytes[1], 0x08, 0x01, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2], _listenIpBytes[3], _listenPortBytes[0], _listenPortBytes[1], 0x04, 0x04, 0x02, 0x00 };
			std::vector<char> response;
			getSystemResponse(0x0206, data, response);
			if(response.size() < 20)
			{
				if(response.size() > 7 && response.at(7) != 0)
				{
					std::map<char, std::string>::iterator statusIterator = _connectResponseStatusCodes.find(response.at(7));
					if(statusIterator != _connectResponseStatusCodes.end()) _out.printError("Error in CONNECT_RES: " + statusIterator->second);
					else _out.printError("Unknown error in CONNECT_RES: " + BaseLib::HelperFunctions::getHexString(response));
					_stopped = true;
					return;
				}
				if(response.empty()) _out.printError("Error: No CONNECT_RES packet received: " + BaseLib::HelperFunctions::getHexString(response));
				else _out.printError("Error: CONNECT_RES packet is too small: " + BaseLib::HelperFunctions::getHexString(response));
				_stopped = true;
				return;
			}
			if(response.at(17) != 0x04)
			{
				_out.printError("Error: Connection is no tunneling connection. Does your gateway support the tunneling protocol?");
				_stopped = true;
				return;
			}
			_knxAddress = (((uint16_t)(uint8_t)response.at(18)) << 8) + (uint8_t)response.at(19);
			_myAddress = _knxAddress;
			_channelId = response.at(6);
			_out.printInfo("Info: Connected. Gateway's KNX address is: " + std::to_string(_knxAddress >> 12) + '.' + std::to_string((_knxAddress >> 8) & 0xf) + '.' + std::to_string(_knxAddress & 0xFF));
		// }}}

		_lastConnectionState = BaseLib::HelperFunctions::getTime();
		if(!getConnectionState()) return;

		_initComplete = true;
		_out.printInfo("Info: Init completed.");
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void MainInterface::stopListening()
{
	try
	{
		// {{{ DISCONNECT_REQ (0x0209)
		if(!_stopped)
		{
			std::vector<char> data{ 0x06, 0x10, 0x02, 0x09, 0x00, 0x10, _channelId, 0x00, 0x08, 0x01, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2], _listenIpBytes[3], _listenPortBytes[0], _listenPortBytes[1] };
			_out.printInfo("Info: Sending packet " + BaseLib::HelperFunctions::getHexString(data));
			_socket->proofwrite(data);
			_initComplete = false;
		}
		// }}}

		_stopCallbackThread = true;
		GD::bl->threadManager.join(_listenThread);
		_stopCallbackThread = false;
		_socket->close();
		_stopped = true;
		IPhysicalInterface::stopListening();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void MainInterface::setListenAddress()
{
	try
	{
		if(!_settings->listenIp.empty() && !BaseLib::Net::isIp(_settings->listenIp))
		{
			//Assume address is interface name
			_listenIp = BaseLib::Net::getMyIpAddress(_settings->listenIp);
		}
		else if(_settings->listenIp.empty())
		{
			_listenIp = BaseLib::Net::getMyIpAddress();
			if(_listenIp.empty()) _bl->out.printError("Error: No IP address could be found to bind the server to. Please specify the IP address manually in knx.conf.");
		}
		else _listenIp = _settings->listenIp;

		std::vector<std::string> ip = _bl->hf.splitAll(_listenIp, '.');
		if(ip.size() != 4 || !BaseLib::Math::isNumber(ip[0], false) || !BaseLib::Math::isNumber(ip[1], false) || !BaseLib::Math::isNumber(ip[2], false) || !BaseLib::Math::isNumber(ip[3], false))
		{
			_listenIp = "";
			_bl->out.printError("Error: IP address is invalid: " + _listenIp);
			return;
		}
		int32_t block1 = BaseLib::Math::getNumber(ip[0], false);
		int32_t block2 = BaseLib::Math::getNumber(ip[1], false);
		int32_t block3 = BaseLib::Math::getNumber(ip[2], false);
		int32_t block4 = BaseLib::Math::getNumber(ip[3], false);
		if(block1 < 0 || block1 > 254 || block2 < 0 || block2 > 254 || block3 < 0 || block3 > 254 || block4 < 0 || block4 > 254)
		{
			_listenIp = "";
			_bl->out.printError("Error: IP address is invalid: " + _listenIp);
			return;
		}
		_listenIpBytes[0] = (char)(uint8_t)block1;
		_listenIpBytes[1] = (char)(uint8_t)block2;
		_listenIpBytes[2] = (char)(uint8_t)block3;
		_listenIpBytes[3] = (char)(uint8_t)block4;
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

bool MainInterface::getConnectionState()
{
	try
    {
		if(!_initComplete) return true;
		std::vector<char> data{ 0x06, 0x10, 0x02, 0x07, 0x00, 0x10, _channelId, 0x00, 0x08, 0x01, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2], _listenIpBytes[3], _listenPortBytes[0], _listenPortBytes[1] };
		std::vector<char> response;
		getSystemResponse(0x0208, data, response);
		if(response.size() < 8)
		{
			if(response.empty()) _out.printError("Error: No CONNECTIONSTATE_RES packet received: " + BaseLib::HelperFunctions::getHexString(response));
			else _out.printError("Error: CONNECTIONSTATE_RES packet is too small: " + BaseLib::HelperFunctions::getHexString(response));
			_stopped = true;
			return false;
		}
		if(response.at(7) != 0)
		{
			std::map<char, std::string>::iterator statusIterator = _connectionStateResponseStatusCodes.find(response.at(7));
			if(statusIterator != _connectionStateResponseStatusCodes.end()) _out.printError("Error in CONNECTIONSTATE_RES: " + statusIterator->second);
			else _out.printError("Unknown error in CONNECTIONSTATE_RES: " + BaseLib::HelperFunctions::getHexString(response));
			_stopped = true;
			return false;
		}
		return true;
    }
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_stopped = true;
	return false;
}

void MainInterface::sendAck(char sequenceCounter, char error)
{
	try
    {
		std::vector<char> ack{ 0x06, 0x10, 0x04, 0x21, 0x00, 0x0A, 0x04, _channelId, sequenceCounter, error };
		if(_bl->debugLevel >= 5) _out.printDebug("Debug: Sending packet " + BaseLib::HelperFunctions::getHexString(ack));
		_socket->proofwrite(ack);
    }
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void MainInterface::sendDisconnectResponse(char sequenceCounter, char error)
{
	try
    {
		std::vector<char> disconnectResponse{ 0x06, 0x10, 0x02, 0x0A, 0x00, 0x08, _channelId, error };
		if(_bl->debugLevel >= 5) _out.printDebug("Debug: Sending packet " + BaseLib::HelperFunctions::getHexString(disconnectResponse));
		_socket->proofwrite(disconnectResponse);
    }
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void MainInterface::listen()
{
    try
    {
    	std::string ipAddress;
    	uint32_t receivedBytes = 0;
    	int32_t bufferMax = 2048;
		std::vector<char> buffer(bufferMax);

        while(!_stopCallbackThread)
        {
        	if(_stopped || !_socket->isOpen())
        	{
        		if(_stopCallbackThread) return;
        		if(_stopped) _out.printWarning("Warning: Connection to device closed. Trying to reconnect...");
        		_socket->close();
        		std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        		reconnect();
        		continue;
        	}
        	std::vector<char> data;
			try
			{
				do
				{
					receivedBytes = _socket->proofread(&buffer[0], bufferMax, ipAddress);
					if(ipAddress != _socket->getClientIp()) continue;
					if(receivedBytes > 0)
					{
						data.insert(data.end(), &buffer.at(0), &buffer.at(0) + receivedBytes);
						if(data.size() > 1000000)
						{
							_out.printError("Could not read: Too much data.");
							break;
						}
					}
				} while(receivedBytes == (unsigned)bufferMax);
			}
			catch(const BaseLib::SocketTimeOutException& ex)
			{
				if(data.empty())
				{
					if(BaseLib::HelperFunctions::getTime() - _lastConnectionState > 60000)
					{
						_lastConnectionState = BaseLib::HelperFunctions::getTime();
						_bl->threadManager.join(_keepAliveThread);
						_bl->threadManager.start(_keepAliveThread, false, &MainInterface::getConnectionState, this);
					}
					continue; //When receivedBytes is exactly 2048 bytes long, proofread will be called again, time out and the packet is received with a delay of 5 seconds. It doesn't matter as packets this big are only received at start up.
				}
			}
			catch(const BaseLib::SocketClosedException& ex)
			{
				_stopped = true;
				_out.printWarning("Warning: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}
			catch(const BaseLib::SocketOperationException& ex)
			{
				_stopped = true;
				_out.printError("Error: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}
			if(data.empty() || data.size() > 1000000) continue;

        	if(_bl->debugLevel >= 5) _out.printDebug("Debug: Packet received. Raw data: " + BaseLib::HelperFunctions::getHexString(data));

        	processPacket(data);

        	_lastPacketReceived = BaseLib::HelperFunctions::getTime();
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void MainInterface::processPacket(std::vector<char>& data)
{
	try
	{
		if(data.size() < 6)
		{
			_out.printError("Error: Too small packet received: " + BaseLib::HelperFunctions::getHexString(data));
			return;
		}
		uint8_t messagecode = (data.size() > 10) ? data[10] : 0;
		uint16_t serviceType = (((uint16_t)(uint8_t)data[2]) << 8) + (uint8_t)data[3];
		_requestsMutex.lock();
		std::map<uint32_t, std::shared_ptr<Request>>::iterator requestIterator = (messagecode == 0x2E) ? _requests.find(0x2E0000 | serviceType) : _requests.find(serviceType); //Check for DATA_CONTROL (0x2E, needed in sendPacket)
		if(requestIterator != _requests.end())
		{
			std::shared_ptr<Request> request = requestIterator->second;
			_requestsMutex.unlock();
			request->response = data;
			{
				std::lock_guard<std::mutex> lock(request->mutex);
				request->mutexReady = true;
			}
			request->conditionVariable.notify_one();
			return;
		}
		else _requestsMutex.unlock();

		if(data.size() > 8)
		{
			if(data.at(2) == 0x04 && data.at(3) == 0x20) //TUNNELLING_REQUEST
			{
				sendAck(data.at(8), 0);
				if(	data.size() >= 21 && //21 bytes is minimum length
					data[10] == 0x29) //DATA_IND (0x29)
				{
					PMyPacket packet(new MyPacket(data));
					raisePacketReceived(packet);
				}
			}
			if(data.at(2) == 0x02 && data.at(3) == 0x09) //DISCONNECT_REQUEST
			{
				sendDisconnectResponse(data.at(8), data.at(6) == _channelId ? 0 : 0x21);
				_stopped = true;
			}
		}
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void MainInterface::getSystemResponse(uint16_t serviceType, const std::vector<char>& requestPacket, std::vector<char>& responsePacket)
{
	try
    {
		if(_stopped) return;
		responsePacket.clear();

		std::lock_guard<std::mutex> getResponseGuard(_getResponseMutex);
		std::shared_ptr<Request> request(new Request());
		_requestsMutex.lock();
		_requests[serviceType] = request;
		_requestsMutex.unlock();
		std::unique_lock<std::mutex> lock(request->mutex);

		try
		{
			_out.printInfo("Info: Sending packet " + BaseLib::HelperFunctions::getHexString(requestPacket));
			_socket->proofwrite(requestPacket);
		}
		catch(BaseLib::SocketOperationException ex)
		{
			_out.printError("Error sending packet to gateway: " + ex.what());
			return;
		}

		if(!request->conditionVariable.wait_for(lock, std::chrono::milliseconds(10000), [&] { return request->mutexReady; }))
		{
			_out.printError("Error: No response received to packet: " + BaseLib::HelperFunctions::getHexString(requestPacket));
			_stopped = true; //Force reconnect
		}
		responsePacket = request->response;

		_requestsMutex.lock();
		_requests.erase(serviceType);
		_requestsMutex.unlock();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        _requestsMutex.unlock();
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        _requestsMutex.unlock();
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
        _requestsMutex.unlock();
    }
}

}
