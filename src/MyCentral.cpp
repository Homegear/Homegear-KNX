/* Copyright 2013-2017 Homegear UG (haftungsbeschränkt) */

#include "MyCentral.h"
#include "GD.h"

namespace MyFamily
{

MyCentral::MyCentral(ICentralEventSink* eventHandler) : BaseLib::Systems::ICentral(MY_FAMILY_ID, GD::bl, eventHandler)
{
	init();
}

MyCentral::MyCentral(uint32_t deviceID, std::string serialNumber, ICentralEventSink* eventHandler) : BaseLib::Systems::ICentral(MY_FAMILY_ID, GD::bl, deviceID, serialNumber, -1, eventHandler)
{
	init();
}

MyCentral::~MyCentral()
{
	dispose();
}

void MyCentral::dispose(bool wait)
{
	try
	{
		if(_disposing) return;
		_disposing = true;

        _stopWorkerThread = true;

		auto peers = getPeers();
		for(auto& peer : peers)
		{
			auto myPeer = std::dynamic_pointer_cast<MyPeer>(peer);
			myPeer->stopWorkerThread();
		}

        GD::out.printDebug("Debug: Waiting for worker thread of device " + std::to_string(_deviceId) + "...");
        GD::bl->threadManager.join(_workerThread);

		GD::out.printDebug("Removing device " + std::to_string(_deviceId) + " from physical device's event queue...");
		for(std::map<std::string, std::shared_ptr<MainInterface>>::iterator i = GD::physicalInterfaces.begin(); i != GD::physicalInterfaces.end(); ++i)
		{
			//Just to make sure cycle through all physical devices. If event handler is not removed => segfault
			i->second->removeEventHandler(_physicalInterfaceEventhandlers[i->first]);
		}
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
}

void MyCentral::init()
{
	try
    {
        if (_initialized) return; //Prevent running init two times
        _initialized = true;

        _search.reset(new Search(GD::bl));

        for (std::map<std::string, std::shared_ptr<MainInterface>>::iterator i = GD::physicalInterfaces.begin(); i != GD::physicalInterfaces.end(); ++i)
        {
            _physicalInterfaceEventhandlers[i->first] = i->second->addEventHandler((BaseLib::Systems::IPhysicalInterface::IPhysicalInterfaceEventSink*) this);
            i->second->setReconnected(std::function<void()>(std::bind(&MyCentral::interfaceReconnected, this)));
        }

        GD::bl->threadManager.start(_workerThread, true, _bl->settings.workerThreadPriority(), _bl->settings.workerThreadPolicy(), &MyCentral::worker, this);
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void MyCentral::interfaceReconnected()
{
    try
    {
        auto peers = getPeers();
        for(auto& peer : peers)
        {
            auto myPeer = std::dynamic_pointer_cast<MyPeer>(peer);
            myPeer->interfaceReconnected();
        }
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void MyCentral::worker()
{
    try
    {
        std::chrono::milliseconds sleepingTime(100);
        uint32_t counter = 0;
        uint64_t lastPeer;
        lastPeer = 0;

        while(!_stopWorkerThread && !GD::bl->shuttingDown)
        {
            try
            {
                std::this_thread::sleep_for(sleepingTime);
                if(_stopWorkerThread || GD::bl->shuttingDown) return;
                if(counter > 1000)
                {
                    counter = 0;

                    {
                        std::lock_guard<std::mutex> peersGuard(_peersMutex);
                        if(_peersById.size() > 0)
                        {
                            int32_t windowTimePerPeer = _bl->settings.workerThreadWindow() / _peersById.size();
                            sleepingTime = std::chrono::milliseconds(windowTimePerPeer);
                        }
                    }
                }

                std::shared_ptr<MyPeer> peer;

                {
                    std::lock_guard<std::mutex> peersGuard(_peersMutex);
                    if(!_peersById.empty())
                    {
                        if(!_peersById.empty())
                        {
                            std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator nextPeer = _peersById.find(lastPeer);
                            if(nextPeer != _peersById.end())
                            {
                                nextPeer++;
                                if(nextPeer == _peersById.end()) nextPeer = _peersById.begin();
                            }
                            else nextPeer = _peersById.begin();
                            lastPeer = nextPeer->first;
                            peer = std::dynamic_pointer_cast<MyPeer>(nextPeer->second);
                        }
                    }
                }

                if(peer && !peer->deleting) peer->worker();
                counter++;
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
        }
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
}

void MyCentral::loadPeers()
{
	try
	{
		std::shared_ptr<BaseLib::Database::DataTable> rows = _bl->db->getPeers(_deviceId);
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			uint64_t peerID = row->second.at(0)->intValue;
			GD::out.printMessage("Loading KNX peer " + std::to_string(peerID));
			std::shared_ptr<MyPeer> peer(new MyPeer(peerID, row->second.at(2)->intValue, row->second.at(3)->textValue, _deviceId, this));
			if(!peer->load(this))
			{
				if(peer->getDeviceType() == 0)
                {
                    GD::out.printError("Deleting peer " + std::to_string(peerID) + " with invalid device type.");
                    peer->deleteFromDatabase();
                }
				continue;
			}
			if(!peer->getRpcDevice()) continue;
			std::lock_guard<std::mutex> peersGuard(_peersMutex);
			if(!peer->getSerialNumber().empty()) _peersBySerial[peer->getSerialNumber()] = peer;
			_peersById[peerID] = peer;
			std::vector<uint16_t> groupAddresses = peer->getGroupAddresses();
			for(std::vector<uint16_t>::iterator i = groupAddresses.begin(); i != groupAddresses.end(); ++i)
			{
				auto peersIterator = _peersByGroupAddress.find(*i);
				if(peersIterator == _peersByGroupAddress.end()) _peersByGroupAddress.emplace(*i, std::make_shared<std::map<uint64_t, PMyPeer>>());
				_peersByGroupAddress[*i]->emplace(peerID, peer);
			}
		}
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
}

std::shared_ptr<MyPeer> MyCentral::getPeer(uint64_t id)
{
	try
	{
		std::lock_guard<std::mutex> peersGuard(_peersMutex);
		std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator peersIterator = _peersById.find(id);
		if(peersIterator != _peersById.end()) return std::dynamic_pointer_cast<MyPeer>(peersIterator->second);
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
    return std::shared_ptr<MyPeer>();
}

std::shared_ptr<MyPeer> MyCentral::getPeer(std::string serialNumber)
{
	try
	{
		std::lock_guard<std::mutex> peersGuard(_peersMutex);
		std::unordered_map<std::string, std::shared_ptr<BaseLib::Systems::Peer>>::iterator peersIterator = _peersBySerial.find(serialNumber);
		if(peersIterator != _peersBySerial.end()) return std::dynamic_pointer_cast<MyPeer>(peersIterator->second);
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
    return std::shared_ptr<MyPeer>();
}

PGroupAddressPeers MyCentral::getPeer(uint16_t groupAddress)
{
	try
	{
		std::lock_guard<std::mutex> peersGuard(_peersMutex);
		auto peersIterator = _peersByGroupAddress.find(groupAddress);
		if(peersIterator != _peersByGroupAddress.end()) return peersIterator->second;
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
    return PGroupAddressPeers();
}

bool MyCentral::onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(_disposing) return false;
		std::shared_ptr<MyPacket> myPacket(std::dynamic_pointer_cast<MyPacket>(packet));
		if(!myPacket) return false;

		if(_bl->debugLevel >= 4) GD::out.printInfo("Packet received from " + MyPacket::getFormattedPhysicalAddress() + " to " + myPacket->getFormattedDestinationAddress() + ". Operation: " + myPacket->getOperationString() + ". Payload: " + BaseLib::HelperFunctions::getHexString(myPacket->getPayload()));

		auto peers = getPeer(myPacket->getDestinationAddress());
		if(!peers) return false;
		for(auto& peer : *peers)
		{
			peer.second->packetReceived(myPacket);
		}

		return true;
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
    return false;
}

void MyCentral::savePeers(bool full)
{
	try
	{
		std::lock_guard<std::mutex> peersGuard(_peersMutex);
		for(std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peersById.begin(); i != _peersById.end(); ++i)
		{
			GD::out.printInfo("Info: Saving KNX peer " + std::to_string(i->second->getID()));
			i->second->save(full, full, full);
		}
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
}

void MyCentral::removePeerFromGroupAddresses(uint16_t groupAddress, uint64_t peerId)
{
	try
	{
		std::lock_guard<std::mutex> peersGuard(_peersMutex);
		auto peersIterator = _peersByGroupAddress.find(groupAddress);
		if(peersIterator == _peersByGroupAddress.end()) return;
		peersIterator->second->erase(peerId);
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
}

void MyCentral::deletePeer(uint64_t id)
{
	try
	{
		std::shared_ptr<MyPeer> peer(getPeer(id));
		if(!peer) return;
		peer->deleting = true;
		PVariable deviceAddresses(new Variable(VariableType::tArray));
		deviceAddresses->arrayValue->push_back(PVariable(new Variable(peer->getSerialNumber())));

		PVariable deviceInfo(new Variable(VariableType::tStruct));
		deviceInfo->structValue->insert(StructElement("ID", PVariable(new Variable((int32_t)peer->getID()))));
		PVariable channels(new Variable(VariableType::tArray));
		deviceInfo->structValue->insert(StructElement("CHANNELS", channels));

		for(Functions::iterator i = peer->getRpcDevice()->functions.begin(); i != peer->getRpcDevice()->functions.end(); ++i)
		{
			deviceAddresses->arrayValue->push_back(PVariable(new Variable(peer->getSerialNumber() + ":" + std::to_string(i->first))));
			channels->arrayValue->push_back(PVariable(new Variable(i->first)));
		}

        std::vector<uint64_t> deletedIds{ id };
        raiseRPCDeleteDevices(deletedIds, deviceAddresses, deviceInfo);

		std::vector<uint16_t> groupAddresses;
		{
			std::lock_guard<std::mutex> peersGuard(_peersMutex);
			if(_peersBySerial.find(peer->getSerialNumber()) != _peersBySerial.end()) _peersBySerial.erase(peer->getSerialNumber());
			if(_peersById.find(id) != _peersById.end()) _peersById.erase(id);

			groupAddresses = peer->getGroupAddresses();
		}

		for(const uint16_t& address : groupAddresses)
		{
			removePeerFromGroupAddresses(address, id);
		}

        if(_currentPeer && _currentPeer->getID() == id) _currentPeer.reset();

        int32_t i = 0;
        while(peer.use_count() > 1 && i < 600)
        {
            if(_currentPeer && _currentPeer->getID() == id) _currentPeer.reset();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            i++;
        }
        if(i == 600) GD::out.printError("Error: Peer deletion took too long.");

		peer->deleteFromDatabase();

		GD::out.printInfo("Info: Deleting XML file \"" + peer->getRpcDevice()->getPath() + "\"");
		GD::bl->io.deleteFile(peer->getRpcDevice()->getPath());
		GD::out.printMessage("Removed KNX peer " + std::to_string(peer->getID()));
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
}

std::string MyCentral::handleCliCommand(std::string command)
{
	try
	{
		std::vector<std::string> arguments;
		bool showHelp = false;
		std::ostringstream stringStream;
		if(_currentPeer)
		{
			if(command == "unselect" || command == "u")
			{
				_currentPeer.reset();
				return "Peer unselected.\n";
			}
			return _currentPeer->handleCliCommand(command);
		}
		if(BaseLib::HelperFunctions::checkCliCommand(command, "help", "h", "", 0, arguments, showHelp))
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the individual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "peers list (ls)    List all peers" << std::endl;
			stringStream << "peers remove (pr)  Remove a peer" << std::endl;
			stringStream << "peers select (ps)  Select a peer" << std::endl;
			stringStream << "peers setname (pn) Name a peer" << std::endl;
			stringStream << "search (sp)        Searches for new devices" << std::endl;
			stringStream << "unselect (u)       Unselect this device" << std::endl;
			return stringStream.str();
		}
		else if(BaseLib::HelperFunctions::checkCliCommand(command, "peers remove", "pr", "", 0, arguments, showHelp))
		{
			if(showHelp)
			{
				stringStream << "Description: This command removes a peer." << std::endl;
				stringStream << "Usage: peers unpair PEERID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID: The id of the peer to remove. Example: 513" << std::endl;
				return stringStream.str();
			}

			uint64_t peerID = (arguments.size() > 0 ? BaseLib::Math::getNumber64(arguments.at(0)) : 0);

			if(!peerExists(peerID)) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				if(_currentPeer && _currentPeer->getID() == peerID) _currentPeer.reset();
				stringStream << "Removing peer " << std::to_string(peerID) << std::endl;
				deletePeer(peerID);
			}
			return stringStream.str();
		}
		else if(command.compare(0, 10, "peers list") == 0 || command.compare(0, 2, "pl") == 0 || command.compare(0, 2, "ls") == 0)
		{
			try
			{
				std::string filterType;
				std::string filterValue;

				std::stringstream stream(command);
				std::string element;
				int32_t offset = (command.at(1) == 'l' || command.at(1) == 's') ? 0 : 1;
				int32_t index = 0;
				while(std::getline(stream, element, ' '))
				{
					if(index < 1 + offset)
					{
						index++;
						continue;
					}
					else if(index == 1 + offset)
					{
						if(element == "help")
						{
							index = -1;
							break;
						}
						filterType = BaseLib::HelperFunctions::toLower(element);
					}
					else if(index == 2 + offset)
					{
						filterValue = element;
						if(filterType == "name") BaseLib::HelperFunctions::toLower(filterValue);
					}
					index++;
				}
				if(index == -1)
				{
					stringStream << "Description: This command lists information about all peers." << std::endl;
					stringStream << "Usage: peers list [FILTERTYPE] [FILTERVALUE]" << std::endl << std::endl;
					stringStream << "Parameters:" << std::endl;
					stringStream << "  FILTERTYPE:  See filter types below." << std::endl;
					stringStream << "  FILTERVALUE: Depends on the filter type. If a number is required, it has to be in hexadecimal format." << std::endl << std::endl;
					stringStream << "Filter types:" << std::endl;
					stringStream << "  ID: Filter by id." << std::endl;
					stringStream << "      FILTERVALUE: The id of the peer to filter (e. g. 513)." << std::endl;
					stringStream << "  SERIAL: Filter by serial number." << std::endl;
					stringStream << "      FILTERVALUE: The serial number of the peer to filter (e. g. JEQ0554309)." << std::endl;
					stringStream << "  NAME: Filter by name." << std::endl;
					stringStream << "      FILTERVALUE: The part of the name to search for (e. g. \"1st floor\")." << std::endl;
					stringStream << "  TYPE: Filter by device type." << std::endl;
					stringStream << "      FILTERVALUE: The 2 byte device type in hexadecimal format." << std::endl;
					return stringStream.str();
				}

				if(_peersById.empty())
				{
					stringStream << "No peers are paired to this central." << std::endl;
					return stringStream.str();
				}
				std::string bar(" │ ");
				const int32_t idWidth = 8;
				const int32_t nameWidth = 25;
				const int32_t addressWidth = 9;
				const int32_t serialWidth = 13;
				const int32_t typeWidth1 = 7;
				const int32_t typeWidth2 = 25;
				std::string nameHeader("Name");
				nameHeader.resize(nameWidth, ' ');
				std::string typeStringHeader("Type String");
				typeStringHeader.resize(typeWidth2, ' ');
				stringStream << std::setfill(' ')
					<< std::setw(idWidth) << "ID" << bar
					<< nameHeader << bar
					<< std::setw(addressWidth) << "Address" << bar
					<< std::setw(serialWidth) << "Serial Number" << bar
					<< std::setw(typeWidth1) << "Type" << bar
					<< typeStringHeader
					<< std::endl;
				stringStream << "─────────┼───────────────────────────┼───────────┼───────────────┼─────────┼───────────────────────────" << std::endl;
				stringStream << std::setfill(' ')
					<< std::setw(idWidth) << " " << bar
					<< std::setw(nameWidth) << " " << bar
					<< std::setw(addressWidth) << " " << bar
					<< std::setw(serialWidth) << " " << bar
					<< std::setw(typeWidth1) << " " << bar
					<< std::setw(typeWidth2)
					<< std::endl;
				_peersMutex.lock();
				for(std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peersById.begin(); i != _peersById.end(); ++i)
				{
					if(filterType == "id")
					{
						uint64_t id = BaseLib::Math::getNumber(filterValue, false);
						if(i->second->getID() != id) continue;
					}
					else if(filterType == "name")
					{
						std::string name = i->second->getName();
						if((signed)BaseLib::HelperFunctions::toLower(name).find(filterValue) == (signed)std::string::npos) continue;
					}
					else if(filterType == "serial")
					{
						if(i->second->getSerialNumber() != filterValue) continue;
					}
					else if(filterType == "type")
					{
						int32_t deviceType = BaseLib::Math::getNumber(filterValue, true);
						if((int32_t)i->second->getDeviceType() != deviceType) continue;
					}

					stringStream << std::setw(idWidth) << std::setfill(' ') << std::to_string(i->second->getID()) << bar;
					std::string name = i->second->getName();
					size_t nameSize = BaseLib::HelperFunctions::utf8StringSize(name);
					if(nameSize > (unsigned)nameWidth)
					{
						name = BaseLib::HelperFunctions::utf8Substring(name, 0, nameWidth - 3);
						name += "...";
					}
					else name.resize(nameWidth + (name.size() - nameSize), ' ');
					stringStream << name << bar
						<< std::setw(addressWidth) << MyPacket::getFormattedPhysicalAddress(i->second->getAddress()) << bar
						<< std::setw(serialWidth) << i->second->getSerialNumber() << bar
						<< std::setw(typeWidth1) << BaseLib::HelperFunctions::getHexString(i->second->getDeviceType()) << bar;
					if(i->second->getRpcDevice())
					{
						PSupportedDevice type = i->second->getRpcDevice()->getType(i->second->getDeviceType(), i->second->getFirmwareVersion());
						std::string typeID;
						if(type) typeID = type->id;
						if(typeID.size() > (unsigned)typeWidth2)
						{
							typeID.resize(typeWidth2 - 3);
							typeID += "...";
						}
						else typeID.resize(typeWidth2, ' ');
						stringStream << typeID << bar;
					}
					else stringStream << std::setw(typeWidth2);
					stringStream << std::endl << std::dec;
				}
				_peersMutex.unlock();
				stringStream << "─────────┴───────────────────────────┴───────────┴───────────────┴─────────┴───────────────────────────" << std::endl;

				return stringStream.str();
			}
			catch(const std::exception& ex)
			{
				_peersMutex.unlock();
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_peersMutex.unlock();
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_peersMutex.unlock();
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
		else if(command.compare(0, 13, "peers setname") == 0 || command.compare(0, 2, "pn") == 0)
		{
			uint64_t peerID = 0;
			std::string name;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'n') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					else
					{
						peerID = BaseLib::Math::getNumber(element, false);
						if(peerID == 0) return "Invalid id.\n";
					}
				}
				else if(index == 2 + offset) name = element;
				else name += ' ' + element;
				index++;
			}
			if(index == 1 + offset)
			{
				stringStream << "Description: This command sets or changes the name of a peer to identify it more easily." << std::endl;
				stringStream << "Usage: peers setname PEERID NAME" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID:\tThe id of the peer to set the name for. Example: 513" << std::endl;
				stringStream << "  NAME:\tThe name to set. Example: \"1st floor light switch\"." << std::endl;
				return stringStream.str();
			}

			if(!peerExists(peerID)) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				std::shared_ptr<MyPeer> peer = getPeer(peerID);
				peer->setName(name);
				stringStream << "Name set to \"" << name << "\"." << std::endl;
			}
			return stringStream.str();
		}
		else if(command.compare(0, 12, "peers select") == 0 || command.compare(0, 2, "ps") == 0)
		{
			uint64_t id = 0;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 's') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					id = BaseLib::Math::getNumber(element, false);
					if(id == 0) return "Invalid id.\n";
				}
				index++;
			}
			if(index == 1 + offset)
			{
				stringStream << "Description: This command selects a peer." << std::endl;
				stringStream << "Usage: peers select PEERID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID:\tThe id of the peer to select. Example: 513" << std::endl;
				return stringStream.str();
			}

			_currentPeer = getPeer(id);
			if(!_currentPeer) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				stringStream << "Peer with id " << std::hex << std::to_string(id) << " and device type 0x" << _bl->hf.getHexString(_currentPeer->getDeviceType()) << " selected." << std::dec << std::endl;
				stringStream << "For information about the peer's commands type: \"help\"" << std::endl;
			}
			return stringStream.str();
		}
		else if(command.compare(0, 6, "search") == 0 || command.compare(0, 2, "sp") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'p') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help")
					{
						stringStream << "Description: This command imports new or changed devices from ETS project files in \"" + _bl->settings.deviceDescriptionPath() + std::to_string(MY_FAMILY_ID) + "/\"." << std::endl;
						stringStream << "Usage: search" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			PVariable result = searchDevices(nullptr);
			if(result->errorStruct) stringStream << "Error: " << result->structValue->at("faultString")->stringValue << std::endl;
			else stringStream << "Search completed successfully." << std::endl;
			return stringStream.str();
		}
		else if(command == "test")
		{
			std::vector<char> rawPacket = _bl->hf.getBinary("061004200018044D02001100BCE00000210A0400800B3500");
			PMyPacket packet(new MyPacket(rawPacket));
			std::string interface = "MyInterface";
			onPacketReceived(interface, packet);
			stringStream << "Hallo" << std::endl;
			return stringStream.str();
		}
		else return "Unknown command.\n";
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
    return "Error executing command. See log file for more details.\n";
}

std::shared_ptr<MyPeer> MyCentral::createPeer(uint32_t deviceType, std::string serialNumber, bool save)
{
	try
	{
		std::shared_ptr<MyPeer> peer(new MyPeer(_deviceId, this));
		peer->setDeviceType(deviceType);
		peer->setSerialNumber(serialNumber);
		peer->setRpcDevice(GD::family->getRpcDevices()->find(deviceType, 0x10, -1));
		if(!peer->getRpcDevice()) return std::shared_ptr<MyPeer>();
		if(save) peer->save(true, true, false); //Save and create peerID
		return peer;
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
    return std::shared_ptr<MyPeer>();
}

PVariable MyCentral::deleteDevice(BaseLib::PRpcClientInfo clientInfo, std::string serialNumber, int32_t flags)
{
	try
	{
		if(serialNumber.empty()) return Variable::createError(-2, "Unknown device.");
		std::shared_ptr<MyPeer> peer = getPeer(serialNumber);
		if(!peer) return PVariable(new Variable(VariableType::tVoid));

		return deleteDevice(clientInfo, peer->getID(), flags);
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
    return Variable::createError(-32500, "Unknown application error.");
}

PVariable MyCentral::deleteDevice(BaseLib::PRpcClientInfo clientInfo, uint64_t peerID, int32_t flags)
{
	try
	{
		if(peerID == 0) return Variable::createError(-2, "Unknown device.");
		std::shared_ptr<MyPeer> peer = getPeer(peerID);
		if(!peer) return PVariable(new Variable(VariableType::tVoid));
		uint64_t id = peer->getID();
        peer.reset();

		deletePeer(id);

		if(peerExists(id)) return Variable::createError(-1, "Error deleting peer. See log for more details.");

		return PVariable(new Variable(VariableType::tVoid));
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
    return Variable::createError(-32500, "Unknown application error.");
}

PVariable MyCentral::searchDevices(BaseLib::PRpcClientInfo clientInfo)
{
	try
	{
		std::lock_guard<std::mutex> searchGuard(_searchMutex);
		{
			std::lock_guard<std::mutex> peersGuard(_peersMutex);
			_peersBySerial.clear();
			_peersById.clear();
			_peersByGroupAddress.clear();
		}

		std::unordered_set<uint32_t> usedTypeNumbers = GD::family->getRpcDevices()->getKnownTypeNumbers();
		std::unordered_map<std::string, uint32_t> idTypeNumberMap = GD::family->getRpcDevices()->getIdTypeNumberMap();

		std::vector<Search::PeerInfo> peerInfo = _search->search(usedTypeNumbers, idTypeNumberMap);
		GD::out.printInfo("Info: Search completed. Found " + std::to_string(peerInfo.size()) + " devices.");

		GD::family->reloadRpcDevices();

		loadPeers();

		std::vector<std::shared_ptr<MyPeer>> newPeers;
		for(std::vector<Search::PeerInfo>::iterator i = peerInfo.begin(); i != peerInfo.end(); ++i)
        {
            {
                std::lock_guard<std::mutex> peersGuard(_peersMutex);
                auto peersIterator = _peersBySerial.find(i->serialNumber);
                if(peersIterator != _peersBySerial.end())
                {
                    peersIterator->second->setAddress(i->address);
                    if(!i->room.empty())
                    {
                        uint64_t roomId = raiseGetRoomIdByName(i->room);
                        if(roomId > 0) peersIterator->second->setRoom(roomId, -1);
                    }
                    if(!i->name.empty()) peersIterator->second->setName(i->name);
                    else peersIterator->second->setName(MyPacket::getFormattedPhysicalAddress(i->address));
                    continue;
                }
            }
			std::shared_ptr<MyPeer> peer = createPeer(i->type, i->serialNumber, true);
			if(!peer)
			{
				GD::out.printError("Error: Could not add device with type " + BaseLib::HelperFunctions::getHexString(i->type) + ". No matching XML file was found.");
				continue;
			}

			peer->initializeCentralConfig();
			peer->initParametersByGroupAddress();

			std::lock_guard<std::mutex> peersGuard(_peersMutex);
			if(!i->name.empty()) peer->setName(i->name);
			else peer->setName(MyPacket::getFormattedPhysicalAddress(i->address));
			peer->setAddress(i->address);
			if(!i->room.empty())
			{
				uint64_t roomId = raiseGetRoomIdByName(i->room);
				if(roomId > 0) peer->setRoom(roomId, -1);
			}
			_peersBySerial[peer->getSerialNumber()] = peer;
			_peersById[peer->getID()] = peer;
			std::vector<uint16_t> groupAddresses = peer->getGroupAddresses();
			for(std::vector<uint16_t>::iterator j = groupAddresses.begin(); j != groupAddresses.end(); ++j)
			{
				auto peersIterator = _peersByGroupAddress.find(*j);
				if(peersIterator == _peersByGroupAddress.end()) _peersByGroupAddress.emplace(*j, std::make_shared<std::map<uint64_t, PMyPeer>>());
				_peersByGroupAddress[*j]->emplace(peer->getID(), peer);
			}
			newPeers.push_back(peer);
		}

		GD::out.printInfo("Info: Found " + std::to_string(newPeers.size()) + " new devices.");

		if(newPeers.size() > 0)
		{
            std::vector<uint64_t> newIds;
            newIds.reserve(newPeers.size());
			PVariable deviceDescriptions(new Variable(VariableType::tArray));
			for(std::vector<std::shared_ptr<MyPeer>>::iterator i = newPeers.begin(); i != newPeers.end(); ++i)
			{
				std::shared_ptr<std::vector<PVariable>> descriptions = (*i)->getDeviceDescriptions(clientInfo, true, std::map<std::string, bool>());
				if(!descriptions) continue;
                newIds.push_back((*i)->getID());
				for(std::vector<PVariable>::iterator j = descriptions->begin(); j != descriptions->end(); ++j)
				{
					deviceDescriptions->arrayValue->push_back(*j);
				}
			}
			raiseRPCNewDevices(newIds, deviceDescriptions);
		}

		return PVariable(new Variable((uint32_t)newPeers.size()));
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
	return Variable::createError(-32500, "Unknown application error.");
}

PVariable MyCentral::setInterface(BaseLib::PRpcClientInfo clientInfo, uint64_t peerId, std::string interfaceId)
{
	try
	{
		std::shared_ptr<MyPeer> peer(getPeer(peerId));
		if(!peer) return Variable::createError(-2, "Unknown device.");
		return peer->setInterface(clientInfo, interfaceId);
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
    return Variable::createError(-32500, "Unknown application error.");
}

}
