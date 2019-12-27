#include <utility>

/* Copyright 2013-2019 Homegear GmbH */

#include "KnxIpForwarder.h"
#include "GD.h"
#include "KnxIpPacket.h"

namespace Knx
{

KnxIpForwarder::KnxIpForwarder(std::string listenIp, uint16_t port, std::shared_ptr<MainInterface> interface) : _listenIpSetting(std::move(listenIp)), _port(port)
{
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "KNXNet/IP forwarder (port " + std::to_string(port) + "): ");

	signal(SIGPIPE, SIG_IGN);

    _interface = std::move(interface);
    _interface->registerPacketReceivedCallback(std::function<void(const PKnxIpPacket&)>(std::bind(&KnxIpForwarder::packetReceivedCallback, this, std::placeholders::_1)));
    _interface->setReconnected(std::function<void()>(std::bind(&KnxIpForwarder::reconnectedCallback, this)));
}

KnxIpForwarder::~KnxIpForwarder()
{
	try
	{
        _stopThreads = true;
        GD::bl->threadManager.join(_listenThread);
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void KnxIpForwarder::startListening()
{
	try
	{
		stopListening();
        _stopThreads = false;
        GD::bl->threadManager.start(_listenThread, true, &KnxIpForwarder::listen, this);
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void KnxIpForwarder::stopListening()
{
	try
	{
		_stopThreads = true;
		GD::bl->threadManager.join(_listenThread);
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void KnxIpForwarder::setListenAddress()
{
    try
    {
        if(!_listenIpSetting.empty() && !BaseLib::Net::isIp(_listenIpSetting))
        {
            //Assume address is interface name
            _listenIp = BaseLib::Net::getMyIpAddress(_listenIpSetting);
        }
        else if(_listenIpSetting.empty())
        {
            _listenIp = BaseLib::Net::getMyIpAddress();
            if(_listenIp.empty()) _out.printError("Error: No IP address could be found to bind the server to. Please specify the IP address manually in knx.conf.");
        }
        else _listenIp = _listenIpSetting;

        std::vector<std::string> ip = BaseLib::HelperFunctions::splitAll(_listenIp, '.');
        if(ip.size() != 4 || !BaseLib::Math::isNumber(ip[0], false) || !BaseLib::Math::isNumber(ip[1], false) || !BaseLib::Math::isNumber(ip[2], false) || !BaseLib::Math::isNumber(ip[3], false))
        {
            _listenIp = "";
            _out.printError("Error: IP address is invalid: " + _listenIp);
            return;
        }
        int32_t block1 = BaseLib::Math::getNumber(ip[0], false);
        int32_t block2 = BaseLib::Math::getNumber(ip[1], false);
        int32_t block3 = BaseLib::Math::getNumber(ip[2], false);
        int32_t block4 = BaseLib::Math::getNumber(ip[3], false);
        if(block1 < 0 || block1 > 254 || block2 < 0 || block2 > 254 || block3 < 0 || block3 > 254 || block4 < 0 || block4 > 254)
        {
            _listenIp = "";
            _out.printError("Error: IP address is invalid: " + _listenIp);
            return;
        }
        _listenIpBytes[0] = (char)(uint8_t)block1;
        _listenIpBytes[1] = (char)(uint8_t)block2;
        _listenIpBytes[2] = (char)(uint8_t)block3;
        _listenIpBytes[3] = (char)(uint8_t)block4;
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void KnxIpForwarder::listen()
{
    try
    {
        std::array<char, 1024> buffer{};
        int32_t bytesReceived = 0;
        struct sockaddr clientInfo{};
        socklen_t clientInfoSize = sizeof(clientInfo);
        fd_set readFileDescriptor;
        timeval socketTimeout{};
        int32_t nfds = 0;

        _serverSocketDescriptor = getSocketDescriptor();
        if(_serverSocketDescriptor && _serverSocketDescriptor->descriptor != -1)
        {
            _out.printInfo("Info: KnxIpForwarder is now listening.");
        }
        while(!_stopThreads)
        {
            try
            {
                if(!_serverSocketDescriptor || _serverSocketDescriptor->descriptor == -1)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    _out.printError("Error: Socket descriptor is invalid. Rebinding...");
                    _serverSocketDescriptor = getSocketDescriptor();
                    if(_serverSocketDescriptor && _serverSocketDescriptor->descriptor != -1)
                    {
                        _out.printInfo("Info: KnxIpForwarder is now listening.");
                    }
                    continue;
                }

                socketTimeout.tv_sec = 1;
                socketTimeout.tv_usec = 0;
                FD_ZERO(&readFileDescriptor);
                auto fileDescriptorGuard = GD::bl->fileDescriptorManager.getLock();
                fileDescriptorGuard.lock();
                nfds = _serverSocketDescriptor->descriptor + 1;
                if(nfds <= 0)
                {
                    fileDescriptorGuard.unlock();
                    _out.printError("Error: Socket closed (1).");
                    GD::bl->fileDescriptorManager.shutdown(_serverSocketDescriptor);
                    continue;
                }
                FD_SET(_serverSocketDescriptor->descriptor, &readFileDescriptor);
                fileDescriptorGuard.unlock();
                bytesReceived = select(nfds, &readFileDescriptor, nullptr, nullptr, &socketTimeout);
                if(bytesReceived == 0)
                {
                    if(_interface->managementConnected() && BaseLib::HelperFunctions::getTime() - _lastPacketReceived > 300000) _interface->disconnectManagement();
                    continue;
                }
                else if(bytesReceived != 1)
                {
                    _out.printError("Error: Socket closed (2).");
                    GD::bl->fileDescriptorManager.shutdown(_serverSocketDescriptor);
                    continue;
                }

                do
                {
                    bytesReceived = recvfrom(_serverSocketDescriptor->descriptor, buffer.data(), buffer.size(), 0, &clientInfo, &clientInfoSize);
                } while(bytesReceived < 0 && (errno == EAGAIN || errno == EINTR));

                if(bytesReceived == 0)
                {
                    _out.printError("Error: Socket closed (3).");
                    GD::bl->fileDescriptorManager.shutdown(_serverSocketDescriptor);
                    continue;
                }
                else if(bytesReceived == -1)
                {
                    _out.printError("Error: Socket closed (4).");
                    GD::bl->fileDescriptorManager.shutdown(_serverSocketDescriptor);
                    continue;
                }

                std::array<char, INET6_ADDRSTRLEN> ipStringBuffer{};
                uint16_t senderPort;
                if(clientInfo.sa_family == AF_INET)
                {
                    auto s = (struct sockaddr_in*) &clientInfo;
                    inet_ntop(AF_INET, &s->sin_addr, ipStringBuffer.data(), ipStringBuffer.size());
                    senderPort = ntohs(s->sin_port);
                }
                else
                { // AF_INET6
                    auto s = (struct sockaddr_in6*) &clientInfo;
                    inet_ntop(AF_INET6, &s->sin6_addr, ipStringBuffer.data(), ipStringBuffer.size());
                    senderPort = ntohs(s->sin6_port);
                }
                ipStringBuffer.back() = 0;
                auto senderIp = std::string(ipStringBuffer.data());

                if(GD::bl->debugLevel >= 4) _out.printInfo("Info: Packet received from " + senderIp + ": " + BaseLib::HelperFunctions::getHexString(buffer.data(), bytesReceived));
                processRawPacket(senderIp, senderPort, std::vector<uint8_t>(buffer.data(), buffer.data() + bytesReceived));
            }
            catch(const std::exception& ex)
            {
                _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
                GD::bl->fileDescriptorManager.shutdown(_serverSocketDescriptor);
            }
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    GD::bl->fileDescriptorManager.shutdown(_serverSocketDescriptor);
}

std::shared_ptr<BaseLib::FileDescriptor> KnxIpForwarder::getSocketDescriptor()
{
    std::shared_ptr<FileDescriptor> serverSocketDescriptor;
    try
    {
        setListenAddress();
        if(_listenIp.empty()) return serverSocketDescriptor;
        serverSocketDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_INET, SOCK_DGRAM, 0));
        if(serverSocketDescriptor->descriptor == -1)
        {
           _out.printError("Error: Could not create socket.");
            return serverSocketDescriptor;
        }

        if(GD::bl->debugLevel >= 5) _out.printInfo("Debug: SSDP server: Binding to address: " + _listenIp);

        char loopch = 0;
        if(setsockopt(serverSocketDescriptor->descriptor, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) == -1)
        {
            _out.printWarning("Warning: Could set SSDP socket options: " + std::string(strerror(errno)));
        }

        struct in_addr localInterface{};
        localInterface.s_addr = inet_addr(_listenIp.c_str());
        if(setsockopt(serverSocketDescriptor->descriptor, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) == -1)
        {
            _out.printWarning("Warning: Could set SSDP socket options: " + std::string(strerror(errno)));
        }

        struct sockaddr_in localSock{};
        memset((char *) &localSock, 0, sizeof(localSock));
        localSock.sin_family = AF_INET;
        localSock.sin_port = htons(_port);
        localSock.sin_addr.s_addr = inet_addr(_listenIp.c_str());

        if(bind(serverSocketDescriptor->descriptor.load(), (struct sockaddr*)&localSock, sizeof(localSock)) == -1)
        {
            _out.printError("Error: Binding to address " + _listenIp + " failed: " + std::string(strerror(errno)));
            GD::bl->fileDescriptorManager.close(serverSocketDescriptor);
            return serverSocketDescriptor;
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return serverSocketDescriptor;
}

void KnxIpForwarder::processRawPacket(const std::string& senderIp, uint16_t senderPort, const std::vector<uint8_t>& data)
{
    try
    {
        auto time = BaseLib::HelperFunctions::getTime();
        auto packet = std::make_shared<KnxIpPacket>(data);
        if(packet->getServiceType() == ServiceType::CONNECT_REQUEST)
        {
            std::vector<uint8_t> rawResponsePacket;
            if(senderIp != _lastSenderIp && time - _lastPacketReceived < 60000)
            {
                uint8_t status = (uint8_t)KnxIpErrorCodes::E_NO_MORE_CONNECTIONS;
                rawResponsePacket = std::vector<uint8_t>{6, 0x10, 2, 6, 0, 8, 0, status};
            }
            else
            {
                auto packetData = packet->getConnectRequest();
                if(!packetData) return;
                if(packetData->dataEndpointIpString != packetData->controlEndpointIpString || packetData->dataEndpointIpString != senderIp)
                {
                    GD::out.printError("Error: Can't process connect packet from " + senderIp + ". The IP addresses in the packet (" + packetData->dataEndpointIpString + " and " + packetData->controlEndpointIpString + ") do not match the sender's IP address. This is currently not supported.");
                    auto status = (uint8_t)KnxIpErrorCodes::E_CONNECTION_OPTION;
                    rawResponsePacket = std::vector<uint8_t>{6, 0x10, 2, 6, 0, 8, 0, status};
                }
                else
                {
                    if(packetData->connectionTypeCode == 3 && _interface->managementConnected()) _interface->disconnectManagement();

                    _lastSenderIp = senderIp;
                    _lastPacketReceived = time;
                    _senderConfigPort = packetData->controlEndpointPort;
                    _senderDataPort = packetData->dataEndpointPort;
                    auto knxAddress = _interface->getKnxAddress();
                    auto status = (uint8_t) KnxIpErrorCodes::E_NO_ERROR;
                    if(packetData->connectionTypeCode == 4) rawResponsePacket = std::vector<uint8_t>{6, 0x10, 2, 6, 0, 0x14, ++_channelId, status, 8, 1, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2], _listenIpBytes[3], (uint8_t) (_port >> 8), (uint8_t) (_port & 0xFF), 4, 4, (uint8_t) (knxAddress >> 8), (uint8_t) (knxAddress & 0xFF)};
                    else if(packetData->connectionTypeCode == 3) rawResponsePacket = std::vector<uint8_t>{6, 0x10, 2, 6, 0, 0x12, ++_managementChannelId, status, 8, 1, _listenIpBytes[0], _listenIpBytes[1], _listenIpBytes[2], _listenIpBytes[3], (uint8_t) (_port >> 8), (uint8_t) (_port & 0xFF), 2, 3};

                    if(packetData->connectionTypeCode == 3)
                    {
                        _managementSequenceCounterOut = 0;
                        _managementConnected = true;
                    }
                    else if(packetData->connectionTypeCode == 4)
                    {
                        _sequenceCounterOut = 0;
                        _connected = true;
                    }
                }

                if(packetData->connectionTypeCode == 3 && !_interface->managementConnected())
                {
                    _interface->connectManagement();
                }
            }

            sendPacket(senderIp, senderPort, std::make_shared<KnxIpPacket>(rawResponsePacket), true);
            return;
        }
        else if(packet->getServiceType() == ServiceType::DESCRIPTION_REQUEST)
        {
            auto dataCopy = data;

            auto listenIpBytes = _interface->getListenIpBytes();
            auto listenPortBytes = _interface->getListenPortBytes();
            //if(dataCopy.at(8) != 0 || dataCopy.at(9) != 0 || dataCopy.at(10) != 0 || dataCopy.at(11) != 0)
            //{
                //Always send non-broadcast request
                dataCopy.at(8) = listenIpBytes.at(0);
                dataCopy.at(9) = listenIpBytes.at(1);
                dataCopy.at(10) = listenIpBytes.at(2);
                dataCopy.at(11) = listenIpBytes.at(3);
            //}
            //if(dataCopy.at(12) != 0 || dataCopy.at(13) != 0)
            //{
                dataCopy.at(12) = listenPortBytes.at(0);
                dataCopy.at(13) = listenPortBytes.at(1);
            //}

            std::vector<uint8_t> rawResponsePacket;
            _interface->getResponse(ServiceType::DESCRIPTION_RESPONSE, dataCopy, rawResponsePacket);
            if(rawResponsePacket.empty()) return;
            sendPacket(senderIp, senderPort, std::make_shared<KnxIpPacket>(rawResponsePacket), true);
            return;
        }

        if(senderIp != _lastSenderIp || (senderPort != _senderConfigPort && senderPort != _senderDataPort))
        {
            _out.printWarning("Warning: Not processing packet, because the client is not connected: " + BaseLib::HelperFunctions::getHexString(data));
            return;
        }

        if(packet->getServiceType() == ServiceType::CONNECTIONSTATE_REQUEST)
        {
            auto packetData = packet->getConnectionStateRequest();
            if(!packetData) return;

            auto status = (uint8_t)KnxIpErrorCodes::E_NO_ERROR;
            if(packetData->channelId != _channelId && packetData->channelId != _managementChannelId) status = (uint8_t)KnxIpErrorCodes::E_CONNECTION_ID;
            else _lastPacketReceived = time;

            std::vector<uint8_t> rawResponsePacket = std::vector<uint8_t>{6, 0x10, 2, 8, 0, 8, packetData->channelId, status};
            sendPacket(senderIp, _senderConfigPort, std::make_shared<KnxIpPacket>(rawResponsePacket));
        }
        else if(packet->getServiceType() == ServiceType::DISCONNECT_REQUEST)
        {
            auto packetData = packet->getDisconnectRequest();
            if(!packetData) return;

            auto status = (uint8_t)KnxIpErrorCodes::E_NO_ERROR;
            if(packetData->channelId != _channelId && packetData->channelId != _managementChannelId) status = (uint8_t)KnxIpErrorCodes::E_CONNECTION_ID;

            std::vector<uint8_t> rawResponsePacket = std::vector<uint8_t>{6, 0x10, 2, 0x0A, 0, 8, packetData->channelId, status};
            sendPacket(senderIp, senderPort, std::make_shared<KnxIpPacket>(rawResponsePacket), true);

            if(packetData->channelId == _managementChannelId)
            {
                _managementConnected = false;
                if(_interface->managementConnected()) _interface->disconnectManagement();
                _lastManagementSequenceCounterIn = 0;
            }
            else if(packetData->channelId != _channelId) _connected = false;

            if(status == (uint8_t)KnxIpErrorCodes::E_NO_ERROR && !_managementConnected && !_connected)
            {
                _lastPacketReceived = 0;
                _lastSenderIp = "";
            }
        }
        else if(packet->getServiceType() == ServiceType::TUNNELING_REQUEST)
        {
            auto packetData = packet->getTunnelingRequest();
            if(!packetData) return;
            auto sendPacketLock = _interface->getSendPacketLock();
            sendPacketLock.lock();
            packetData->channelId = _interface->getChannelId();
            uint8_t storedSequenceCounter = packetData->sequenceCounter;
            packetData->sequenceCounter = _interface->getSequenceCounter();
            _interface->incrementSequenceCounter();
            packet->clearBinaryCache();

            std::vector<uint8_t> rawResponsePacket;
            _interface->getResponse(ServiceType::TUNNELING_ACK, packet->getBinary(), rawResponsePacket);
            if(rawResponsePacket.empty()) return;
            auto responsePacket = std::make_shared<KnxIpPacket>(rawResponsePacket);
            auto responsePacketData = responsePacket->getTunnelingAck();
            if(!responsePacketData) return;
            responsePacketData->channelId = _channelId;
            responsePacketData->sequenceCounter = storedSequenceCounter;
            responsePacket->clearBinaryCache();
            sendPacket(senderIp, senderPort, responsePacket);
        }
        else if(packet->getServiceType() == ServiceType::CONFIG_REQUEST)
        {
            auto sendPacketLock = _interface->getSendPacketLock();
            sendPacketLock.lock();
            auto dataCopy = data;
            dataCopy.at(7) = _interface->getManagementChannelId();
            uint8_t storedSequenceCounter = dataCopy.at(8);
            dataCopy.at(8) = _interface->getManagementSequenceCounter();
            _interface->incrementManagementSequenceCounter();

            std::vector<uint8_t> rawResponsePacket;
            _interface->getResponse(ServiceType::CONFIG_ACK, dataCopy, rawResponsePacket);
            if(rawResponsePacket.empty()) return;
            rawResponsePacket.at(7) = _managementChannelId;
            rawResponsePacket.at(8) = storedSequenceCounter;
            sendPacket(senderIp, senderPort, std::make_shared<KnxIpPacket>(rawResponsePacket));
        }
        else if(packet->getServiceType() == ServiceType::CONFIG_ACK)
        {
            auto dataCopy = data;
            dataCopy.at(7) = _interface->getManagementChannelId();
            dataCopy.at(8) = _lastManagementSequenceCounterOut;
            _interface->sendRaw(dataCopy);
        }
        else _out.printInfo("Info: Dropping packet " + BaseLib::HelperFunctions::getHexString(data));
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void KnxIpForwarder::sendPacket(const std::string& destinationIp, uint16_t destinationPort, const PKnxIpPacket& packet, bool forceSending)
{
    try
    {
        if(!forceSending && ((!_managementConnected && !_connected) || BaseLib::HelperFunctions::getTime() - _lastPacketReceived > 300000)) return;

        struct sockaddr_in addessInfo{};
        addessInfo.sin_family = AF_INET;
        addessInfo.sin_addr.s_addr = inet_addr(destinationIp.c_str());
        addessInfo.sin_port = htons(destinationPort);

        auto rawPacket = packet->getBinary();
        _out.printInfo("Info: Sending packet " + BaseLib::HelperFunctions::getHexString(rawPacket));
        if(sendto(_serverSocketDescriptor->descriptor, (char*)rawPacket.data(), rawPacket.size(), 0, (struct sockaddr*)&addessInfo, sizeof(addessInfo)) == -1)
        {
            _out.printWarning("Warning: Error sending: " + std::string(strerror(errno)));
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void KnxIpForwarder::reconnectedCallback()
{
    try
    {
        _lastManagementSequenceCounterIn = 0;
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void KnxIpForwarder::packetReceivedCallback(const PKnxIpPacket& packet)
{
    try
    {
        if(packet->getServiceType() == ServiceType::TUNNELING_REQUEST)
        {
            auto packetData = packet->getTunnelingRequest();
            if(!packetData) return;
            packetData->channelId = _channelId;
            packetData->sequenceCounter = _sequenceCounterOut++;
            packet->clearBinaryCache();
            sendPacket(_lastSenderIp, _senderDataPort, packet);
        }
        else if(packet->getServiceType() == ServiceType::CONFIG_REQUEST)
        {
            auto data = packet->getBinary();
            data.at(7) = _managementChannelId;
            _lastManagementSequenceCounterOut = data.at(8);
            data.at(8) = _managementSequenceCounterOut++;
            sendPacket(_lastSenderIp, _senderDataPort, std::make_shared<KnxIpPacket>(data));
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

}
