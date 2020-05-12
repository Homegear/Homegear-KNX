#include <memory>

/* Copyright 2013-2019 Homegear GmbH */

#include "KnxPeer.h"

#include "GD.h"
#include "Cemi.h"
#include "KnxCentral.h"

#include <iomanip>

namespace Knx
{
std::shared_ptr<BaseLib::Systems::ICentral> KnxPeer::getCentral()
{
	try
	{
		if(_central) return _central;
		_central = GD::family->getCentral();
		return _central;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return std::shared_ptr<BaseLib::Systems::ICentral>();
}

KnxPeer::KnxPeer(uint32_t parentID, IPeerEventSink* eventHandler) : BaseLib::Systems::Peer(GD::bl, parentID, eventHandler)
{
	init();
}

KnxPeer::KnxPeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, IPeerEventSink* eventHandler) : BaseLib::Systems::Peer(GD::bl, id, address, serialNumber, parentID, eventHandler)
{
	init();
}

KnxPeer::~KnxPeer()
{
	try
	{
		dispose();
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

void KnxPeer::init()
{
	try
	{
		_readVariables = false;
        _stopWorkerThread = false;
		_dptConverter.reset(new DptConverter(GD::bl));
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

void KnxPeer::dispose()
{
	if(_disposing) return;
	Peer::dispose();
}

void KnxPeer::worker()
{
    try
    {
		for(auto& interface : GD::physicalInterfaces)
		{
			if(!interface.second->isOpen()) return;
		}

        if(_readVariables)
        {
            _readVariables = false;
            for(Functions::iterator i = _rpcDevice->functions.begin(); i != _rpcDevice->functions.end(); ++i)
            {
                if(i->first == 0) continue;

                PParameterGroup parameterGroup = getParameterSet(i->first, ParameterGroup::Type::variables);
                if(!parameterGroup) continue;

                for(Parameters::iterator j = parameterGroup->parameters.begin(); j != parameterGroup->parameters.end(); ++j)
                {
                    if(_stopWorkerThread) return;
                    if(!j->second->readable) continue;
                    if(GD::bl->debugLevel >= 4) GD::out.printInfo("Info: Reading " + j->second->id + " of peer " + std::to_string(_peerID) + " on channel " + std::to_string(i->first));
                    getValueFromDevice(j->second, i->first, false);
                }
            }
        }

        if(!serviceMessages->getUnreach()) serviceMessages->checkUnreach(_rpcDevice->timeout, getLastPacketReceived());
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void KnxPeer::homegearStarted()
{
	try
	{
		Peer::homegearStarted();
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

void KnxPeer::homegearShuttingDown()
{
	try
	{
		Peer::homegearShuttingDown();
        _stopWorkerThread = true;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

std::string KnxPeer::getFormattedAddress()
{
    if(_address < 0) return "";
    return std::to_string(_address >> 12) + '.' + std::to_string((_address >> 8) & 0x0F) + '.' + std::to_string(_address & 0xFF);
}

std::string KnxPeer::handleCliCommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;

		if(command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the individual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "unselect\t\tUnselect this peer" << std::endl;
			stringStream << "channel count\t\tPrint the number of channels of this peer" << std::endl;
			stringStream << "config print\t\tPrints all configuration parameters and their values" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 13, "channel count") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints this peer's number of channels." << std::endl;
						stringStream << "Usage: channel count" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			stringStream << "Peer has " << _rpcDevice->functions.size() << " channels." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 12, "config print") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints all configuration parameters of this peer. The values are in BidCoS packet format." << std::endl;
						stringStream << "Usage: config print" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			return printConfig();
		}
		else return "Unknown command.\n";
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return "Error executing command. See log file for more details.\n";
}

std::string KnxPeer::printConfig()
{
	try
	{
		std::ostringstream stringStream;
		stringStream << "MASTER" << std::endl;
		stringStream << "{" << std::endl;
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>>::iterator i = configCentral.begin(); i != configCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringStream << "\t\t[" << j->first << "]: ";
				if(!j->second.rpcParameter) stringStream << "(No RPC parameter) ";
				std::vector<uint8_t> parameterData = j->second.getBinaryData();
				for(std::vector<uint8_t>::const_iterator k = parameterData.begin(); k != parameterData.end(); ++k)
				{
					stringStream << std::hex << std::setfill('0') << std::setw(2) << (int32_t)*k << " ";
				}
				stringStream << std::endl;
			}
			stringStream << "\t}" << std::endl;
		}
		stringStream << "}" << std::endl << std::endl;

		stringStream << "VALUES" << std::endl;
		stringStream << "{" << std::endl;
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>>::iterator i = valuesCentral.begin(); i != valuesCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
                ParameterCast::PGeneric parameterCast;
                if(!j->second.rpcParameter->casts.empty()) parameterCast = std::dynamic_pointer_cast<ParameterCast::Generic>(j->second.rpcParameter->casts.at(0));

				stringStream << "\t\t[" << j->first << (i->first == 0 || !j->second.rpcParameter ? "" : ", " + Cemi::getFormattedGroupAddress(j->second.rpcParameter->physical->address)) + (parameterCast ? ", " + parameterCast->type : "") + "]: ";
				if(!j->second.rpcParameter)
				{
					stringStream << "(No RPC parameter)";
					continue;
				}
				std::vector<uint8_t> parameterData = j->second.getBinaryData();
				for(std::vector<uint8_t>::const_iterator k = parameterData.begin(); k != parameterData.end(); ++k)
				{
					stringStream << std::hex << std::setfill('0') << std::setw(2) << (int32_t)*k << " ";
				}

                if(!parameterCast)
                {
                    stringStream << std::endl;
                    continue;
                }

                PVariable value;
                std::string::size_type pos = j->first.find('.');
                if(pos != std::string::npos)
                {
                    if(j->second.rpcParameter->physical->bitSize < 1)
                    {
                        stringStream << std::endl;
                        continue;
                    }

                    std::vector<uint8_t> groupedParameterData = BaseLib::BitReaderWriter::getPosition(parameterData, j->second.rpcParameter->physical->address, j->second.rpcParameter->physical->bitSize);
                    value = _dptConverter->getVariable(parameterCast->type, groupedParameterData, j->second.invert());
                }
                else value = _dptConverter->getVariable(parameterCast->type, parameterData, j->second.invert());

				if(!value)
                {
                    stringStream << std::endl;
                    continue;
                }

				stringStream << "(" << (value->type == VariableType::tString ? "\"" : "") << value->toString() << (value->type == VariableType::tString ? "\"" : "") << ")" << std::endl;
			}
			stringStream << "\t}" << std::endl;
		}
		stringStream << "}" << std::endl << std::endl;

		return stringStream.str();
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return "";
}

std::vector<uint16_t> KnxPeer::getGroupAddresses()
{
	std::vector<uint16_t> addresses;
	try
	{
		if(!_rpcDevice) return addresses;
		addresses.reserve(100);
		for(Functions::iterator i = _rpcDevice->functions.begin(); i != _rpcDevice->functions.end(); ++i)
		{
			if(i->second->channel == 0) continue;
			for(Parameters::iterator j = i->second->variables->parameters.begin(); j != i->second->variables->parameters.end(); ++j)
			{
				if(j->second->physical->operationType != BaseLib::DeviceDescription::IPhysical::OperationType::command) continue;
				if(addresses.size() + 1 > addresses.capacity()) addresses.reserve(addresses.size() + 100);
				addresses.push_back(j->second->physical->address);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return addresses;
}

void KnxPeer::loadVariables(BaseLib::Systems::ICentral* central, std::shared_ptr<BaseLib::Database::DataTable>& rows)
{
	try
	{
		if(!rows) rows = _bl->db->getPeerVariables(_peerID);
		Peer::loadVariables(central, rows);

		_rpcDevice = GD::family->getRpcDevices()->find(_deviceType, _firmwareVersion, -1);
		if(!_rpcDevice) return;
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

bool KnxPeer::load(BaseLib::Systems::ICentral* central)
{
	try
	{
		std::shared_ptr<BaseLib::Database::DataTable> rows;
		loadVariables(central, rows);
		if(!_rpcDevice)
		{
			GD::out.printError("Error loading peer " + std::to_string(_peerID) + ": Device type not found: 0x" + BaseLib::HelperFunctions::getHexString(_deviceType) + " Firmware version: " + std::to_string(_firmwareVersion));
			return false;
		}

		initializeTypeString();
		std::string entry;
		loadConfig();
		initializeCentralConfig();

		serviceMessages.reset(new BaseLib::Systems::ServiceMessages(_bl, _peerID, _serialNumber, this));
		serviceMessages->load();

		initParametersByGroupAddress();

        _readVariables = true;

		return true;
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return false;
}

void KnxPeer::initParametersByGroupAddress()
{
	try
	{
		if(!_rpcDevice) return;
		_parametersByGroupAddress.clear();
		for(Functions::iterator i = _rpcDevice->functions.begin(); i != _rpcDevice->functions.end(); ++i)
		{
			if(i->second->channel == 0) continue;
			for(Parameters::iterator j = i->second->variables->parameters.begin(); j != i->second->variables->parameters.end(); ++j)
			{
				if(j->second->casts.empty()) continue;

				std::string::size_type pos = j->first.find('.');
				if(pos != std::string::npos)
				{
					std::string baseName = j->first.substr(0, pos);
					std::string extension = j->first.substr(pos);
					if(extension == ".RAW")
					{
						_groupedParameters[i->first][baseName].rawParameter = j->second;

						ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(j->second->casts.at(0));
						if(!cast) continue;
						ParametersByGroupAddressInfo info;
						info.channel = i->first;
						info.cast = cast;
						info.parameter = j->second;
						_parametersByGroupAddress[j->second->physical->address].push_back(info);
					}
					else if(extension == ".SUBMIT") _groupedParameters[i->first][baseName].submitParameter = j->second;
					else _groupedParameters[i->first][baseName].parameters.push_back(j->second);
				}
				else
				{
					if(j->second->physical->operationType != BaseLib::DeviceDescription::IPhysical::OperationType::command) continue;
					ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(j->second->casts.at(0));
					if(!cast) continue;
					ParametersByGroupAddressInfo info;
					info.channel = i->first;
					info.cast = cast;
					info.parameter = j->second;
					_parametersByGroupAddress[j->second->physical->address].push_back(info);
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void KnxPeer::sendPacket(const PCemi& packet)
{
    try
    {
        if(_rpcDevice->interface.empty())
        {
            for(auto& interface : GD::physicalInterfaces)
            {
                interface.second->sendPacket(packet);
            }
        }
        else
        {
            auto interfaceIterator = GD::physicalInterfaces.find(_rpcDevice->interface);
            if(interfaceIterator == GD::physicalInterfaces.end())
            {
                GD::out.printError("Error: Communication interface \"" + _rpcDevice->interface + "\" required by peer " + std::to_string(_peerID) + " was not found. Could not send packet.");
                return;
            }
            interfaceIterator->second->sendPacket(packet);
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void KnxPeer::packetReceived(PCemi& packet)
{
	try
	{
		if(_disposing || !_rpcDevice) return;
		setLastPacketReceived();

		auto parametersIterator = _parametersByGroupAddress.find(packet->getDestinationAddress());
		if(parametersIterator == _parametersByGroupAddress.end()) return;

        if(packet->getOperation() == Cemi::Operation::groupValueWrite)
        {
            for(auto& parameterIterator : parametersIterator->second)
            {
                BaseLib::Systems::RpcConfigurationParameter& parameter = valuesCentral[parameterIterator.channel][parameterIterator.parameter->id];
                if(!parameter.rpcParameter) return;

                std::vector<uint8_t> parameterData = packet->getPayload();
                parameter.setBinaryData(parameterData);
                if(parameter.databaseId > 0) saveParameter(parameter.databaseId, parameterData);
                else saveParameter(0, ParameterGroup::Type::Enum::variables, parameterIterator.channel, parameterIterator.parameter->id, parameterData);
                if(_bl->debugLevel >= 4) GD::out.printInfo("Info: " + parameterIterator.parameter->id + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(parameterIterator.channel) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(parameterData) + ".");

                PVariable variable = _dptConverter->getVariable(parameterIterator.cast->type, packet->getPayload(), parameter.invert());
                if(!variable) return;

                std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{parameterIterator.parameter->id});
                std::shared_ptr<std::vector<PVariable>> values(new std::vector<PVariable>{variable});

                std::string::size_type pos = parameterIterator.parameter->id.find('.');
                if(pos != std::string::npos)
                {
                    std::string baseName = parameterIterator.parameter->id.substr(0, pos);
                    std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>>::iterator channelIterator = valuesCentral.find(parameterIterator.channel);
                    if(channelIterator != valuesCentral.end())
                    {
                        std::map<int32_t, std::map<std::string, GroupedParametersInfo>>::iterator groupedParametersChannelIterator = _groupedParameters.find(parameterIterator.channel);
                        if(groupedParametersChannelIterator != _groupedParameters.end())
                        {
                            std::map<std::string, GroupedParametersInfo>::iterator groupedParametersIterator = groupedParametersChannelIterator->second.find(baseName);
                            if(groupedParametersIterator != groupedParametersChannelIterator->second.end())
                            {
                                for(std::vector<PParameter>::iterator i = groupedParametersIterator->second.parameters.begin(); i != groupedParametersIterator->second.parameters.end(); ++i)
                                {
                                    std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator groupedParameterIterator = channelIterator->second.find((*i)->id);
                                    if(groupedParameterIterator != channelIterator->second.end())
                                    {
                                        BaseLib::Systems::RpcConfigurationParameter& groupedParameter = groupedParameterIterator->second;

                                        if((*i)->casts.empty() || (*i)->physical->bitSize < 1) continue;
                                        ParameterCast::PGeneric groupedCast = std::dynamic_pointer_cast<ParameterCast::Generic>((*i)->casts.at(0));
                                        if(!groupedCast) continue;

                                        std::vector<uint8_t> groupedParameterData = BaseLib::BitReaderWriter::getPosition(parameter.getBinaryData(), (*i)->physical->address, (*i)->physical->bitSize);

                                        PVariable groupedVariable = _dptConverter->getVariable(groupedCast->type, groupedParameterData, groupedParameter.invert());
                                        if(!groupedVariable) continue;
                                        if(_getValueFromDeviceInfo.requested && parameterIterator.channel == _getValueFromDeviceInfo.channel && (*i)->id == _getValueFromDeviceInfo.variableName)
                                        {
                                            _getValueFromDeviceInfo.requested = false;
                                            _getValueFromDeviceInfo.value = groupedVariable;
                                            {
                                                std::lock_guard<std::mutex> lock(_getValueFromDeviceInfo.mutex);
                                                _getValueFromDeviceInfo.mutexReady = true;
                                            }
                                            _getValueFromDeviceInfo.conditionVariable.notify_one();
                                        }

                                        if(groupedParameter.equals(groupedParameterData)) continue;
                                        groupedParameter.setBinaryData(groupedParameterData);
                                        if(groupedParameter.databaseId > 0) saveParameter(groupedParameter.databaseId, groupedParameterData);
                                        else saveParameter(0, ParameterGroup::Type::Enum::variables, parameterIterator.channel, (*i)->id, groupedParameterData);
                                        if(_bl->debugLevel >= 4) GD::out.printInfo("Info: " + (*i)->id + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(parameterIterator.channel) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(groupedParameterData) + ".");

                                        valueKeys->push_back((*i)->id);
                                        values->push_back(groupedVariable);
                                    }
                                }
                            }
                        }
                    }
                }

                if(_getValueFromDeviceInfo.requested && parameterIterator.channel == _getValueFromDeviceInfo.channel && parameterIterator.parameter->id == _getValueFromDeviceInfo.variableName)
                {
                    _getValueFromDeviceInfo.requested = false;
                    _getValueFromDeviceInfo.value = variable;
                    {
                        std::lock_guard<std::mutex> lock(_getValueFromDeviceInfo.mutex);
                        _getValueFromDeviceInfo.mutexReady = true;
                    }
                    _getValueFromDeviceInfo.conditionVariable.notify_one();
                }

                std::string eventSource = "device-" + std::to_string(_peerID);
                std::string address(_serialNumber + ":" + std::to_string(parameterIterator.channel));
                raiseEvent(eventSource, _peerID, parameterIterator.channel, valueKeys, values);
                raiseRPCEvent(eventSource, _peerID, parameterIterator.channel, address, valueKeys, values);
            }
        }
        else if(packet->getOperation() == Cemi::Operation::groupValueRead)
        {
            //Check if groupValueRead realy needs to be implemented here.

            if(parametersIterator->second.empty()) return;
            int32_t channel = parametersIterator->second.front().channel;
            std::string parameterId = parametersIterator->second.front().parameter->id;
            BaseLib::Systems::RpcConfigurationParameter& parameter = valuesCentral[channel][parameterId];
            if(!parameter.rpcParameter) return;

            bool fitsInFirstByte = false;
            std::vector<uint8_t> parameterData = parameter.getBinaryData();
            if(!parameter.rpcParameter->casts.empty())
            {
                ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter.rpcParameter->casts.at(0));
                if(!cast)
                {
                    GD::out.printError("Error: No DPT conversion defined for parameter " + parameter.rpcParameter->id + ". Can't send response.");
                    return;
                }
                fitsInFirstByte = _dptConverter->fitsInFirstByte(cast->type);
            }

            if(_bl->debugLevel >= 4) GD::out.printInfo("Info: " + parameterId + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(channel) + " was requested. Current value is 0x" + BaseLib::HelperFunctions::getHexString(parameterData) + ".");

            auto responsePacket = std::make_shared<Cemi>(Cemi::Operation::groupValueResponse, 0, parameter.rpcParameter->physical->address, fitsInFirstByte, parameterData);
            sendPacket(responsePacket);
        }
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

PVariable KnxPeer::getValueFromDevice(PParameter& parameter, int32_t channel, bool asynchronous)
{
	try
	{
		if(!parameter) return Variable::createError(-32500, "parameter is nullptr.");
		std::unordered_map<uint32_t, std::unordered_map<std::string, Systems::RpcConfigurationParameter>>::iterator channelIterator = valuesCentral.find(channel);
		if(channelIterator == valuesCentral.end()) return Variable::createError(-2, "Unknown channel.");
		std::unordered_map<std::string, Systems::RpcConfigurationParameter>::iterator valuesIterator = channelIterator->second.find(parameter->id);
		if(valuesIterator == channelIterator->second.end() || !valuesIterator->second.rpcParameter) return Variable::createError(-5, "Unknown parameter.");

		if(valuesIterator->second.rpcParameter->casts.empty()) return Variable::createError(-7, "No DPT conversion defined.");
		ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(valuesIterator->second.rpcParameter->casts.at(0));
		if(!cast) return Variable::createError(-7, "No DPT conversion defined.");

		std::lock_guard<std::mutex> getValueFromDeviceGuard(_getValueFromDeviceMutex);

		_getValueFromDeviceInfo.channel = channel;
		_getValueFromDeviceInfo.variableName = parameter->id;
		_getValueFromDeviceInfo.requested = true;
		std::unique_lock<std::mutex> lock(_getValueFromDeviceInfo.mutex);
		_getValueFromDeviceInfo.mutexReady = false;

		auto packet = std::make_shared<Cemi>(Cemi::Operation::groupValueRead, 0, valuesIterator->second.rpcParameter->physical->address);
		sendPacket(packet);

		if(!_getValueFromDeviceInfo.conditionVariable.wait_for(lock, std::chrono::milliseconds(2000), [&] { return _getValueFromDeviceInfo.mutexReady; }))
		{
			return std::make_shared<Variable>(VariableType::tVoid);
		}

		return _getValueFromDeviceInfo.value;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return Variable::createError(-32500, "Unknown application error.");
}

PParameterGroup KnxPeer::getParameterSet(int32_t channel, ParameterGroup::Type::Enum type)
{
	try
	{
		PFunction rpcChannel = _rpcDevice->functions.at(channel);
		if(type == ParameterGroup::Type::Enum::variables) return rpcChannel->variables;
		else if(type == ParameterGroup::Type::Enum::config) return rpcChannel->configParameters;
		else if(type == ParameterGroup::Type::Enum::link) return rpcChannel->linkParameters;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return PParameterGroup();
}

bool KnxPeer::getAllValuesHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters)
{
	try
	{
		if(channel == 1)
		{
			if(parameter->id == "PEER_ID")
			{
				std::vector<uint8_t> parameterData;
				auto& rpcConfigurationParameter = valuesCentral[channel][parameter->id];
				parameter->convertToPacket(PVariable(new Variable((int32_t)_peerID)), rpcConfigurationParameter.invert(), parameterData);
                rpcConfigurationParameter.setBinaryData(parameterData);
			}
		}
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return false;
}

bool KnxPeer::getParamsetHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters)
{
	try
	{
		if(channel == 1)
		{
			if(parameter->id == "PEER_ID")
			{
				std::vector<uint8_t> parameterData;
                auto& rpcConfigurationParameter = valuesCentral[channel][parameter->id];
				parameter->convertToPacket(PVariable(new Variable((int32_t)_peerID)), rpcConfigurationParameter.invert(), parameterData);
                rpcConfigurationParameter.setBinaryData(parameterData);
			}
		}
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return false;
}

bool KnxPeer::convertFromPacketHook(BaseLib::Systems::RpcConfigurationParameter& parameter, std::vector<uint8_t>& data, PVariable& result)
{
	try
	{
		if(!parameter.rpcParameter) return false;
		if(parameter.rpcParameter->casts.empty()) return false;
		ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter.rpcParameter->casts.at(0));
		if(!cast) return false;
		result = _dptConverter->getVariable(cast->type, data, parameter.invert());
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return true;
}

bool KnxPeer::convertToPacketHook(BaseLib::Systems::RpcConfigurationParameter& parameter, PVariable& data, std::vector<uint8_t>& result)
{
	try
	{
		if(!parameter.rpcParameter) return false;
		if(parameter.rpcParameter->casts.empty()) return false;
		ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter.rpcParameter->casts.at(0));
		if(!cast) return false;
		result = _dptConverter->getDpt(cast->type, data, parameter.invert());
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return true;
}

PVariable KnxPeer::putParamset(BaseLib::PRpcClientInfo clientInfo, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, PVariable variables, bool checkAcls, bool onlyPushing)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		Functions::iterator functionIterator = _rpcDevice->functions.find(channel);
		if(functionIterator == _rpcDevice->functions.end()) return Variable::createError(-2, "Unknown channel.");
		if(type == ParameterGroup::Type::none) type = ParameterGroup::Type::link;
		PParameterGroup parameterGroup = functionIterator->second->getParameterGroup(type);
		if(!parameterGroup) return Variable::createError(-3, "Unknown parameter set.");
		if(variables->structValue->empty()) return PVariable(new Variable(VariableType::tVoid));

        auto central = getCentral();
        if(!central) return Variable::createError(-32500, "Could not get central.");

		if(type == ParameterGroup::Type::Enum::config)
		{
			return Variable::createError(-3, "Parameter set type is not supported.");
		}
		else if(type == ParameterGroup::Type::Enum::variables)
		{
			for(Struct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;

                if(checkAcls && !clientInfo->acls->checkVariableWriteAccess(central->getPeer(_peerID), channel, i->first)) continue;

				setValue(clientInfo, channel, i->first, i->second, true);
			}
		}
		else
		{
			return Variable::createError(-3, "Parameter set type is not supported.");
		}
		return PVariable(new Variable(VariableType::tVoid));
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return Variable::createError(-32500, "Unknown application error.");
}

PVariable KnxPeer::setValue(BaseLib::PRpcClientInfo clientInfo, uint32_t channel, std::string valueKey, PVariable value, bool wait)
{
	try
	{
		Peer::setValue(clientInfo, channel, valueKey, value, wait); //Ignore result, otherwise setHomegerValue might not be executed
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(valueKey.empty()) return Variable::createError(-5, "Value key is empty.");
		if(channel == 0 && serviceMessages->set(valueKey, value->booleanValue)) return std::make_shared<Variable>(VariableType::tVoid);
		auto channelIterator = valuesCentral.find(channel);
		if(channelIterator == valuesCentral.end()) return Variable::createError(-2, "Unknown channel.");
		auto parameterIterator = channelIterator->second.find(valueKey);
		if(parameterIterator == channelIterator->second.end()) return Variable::createError(-5, "Unknown parameter.");
		PParameter rpcParameter = parameterIterator->second.rpcParameter;
		if(!rpcParameter) return Variable::createError(-5, "Unknown parameter.");
		BaseLib::Systems::RpcConfigurationParameter& parameter = parameterIterator->second;
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>());
		std::shared_ptr<std::vector<PVariable>> values(new std::vector<PVariable>());
		if(rpcParameter->readable)
		{
			valueKeys->push_back(valueKey);
			values->push_back(value);
		}

		bool fitsInFirstByte = false;
		std::vector<uint8_t> parameterData;

        {
            bool parameterConverted = false;
            if(!rpcParameter->casts.empty())
            {
                ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(rpcParameter->casts.at(0));
                if(!cast)
                {
                    if(rpcParameter->physical->operationType != IPhysical::OperationType::Enum::store)
                    {
                        return Variable::createError(-7, "No DPT conversion defined.");
                    }
                }
                else
                {
                    parameterData = _dptConverter->getDpt(cast->type, value, parameter.invert());
                    parameter.setBinaryData(parameterData);
                    fitsInFirstByte = _dptConverter->fitsInFirstByte(cast->type);
                    parameterConverted = true;
                }
            }

            if(!parameterConverted)
            {
                std::vector<uint8_t> parameterData;
                rpcParameter->convertToPacket(value, parameter.invert(), parameterData);
                parameter.setBinaryData(parameterData);
                if(parameter.databaseId > 0) saveParameter(parameter.databaseId, parameterData);
                else saveParameter(0, ParameterGroup::Type::Enum::variables, channel, valueKey, parameterData);
            }
        }

		if(rpcParameter->physical->operationType == IPhysical::OperationType::Enum::store)
		{
			if(parameter.databaseId > 0) saveParameter(parameter.databaseId, parameterData);
			else saveParameter(0, ParameterGroup::Type::Enum::variables, channel, valueKey, parameterData);
			if(_bl->debugLevel >= 4) GD::out.printInfo("Info: " + valueKey + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(channel) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(parameterData) + ".");

			std::string::size_type pos = valueKey.find('.');
			std::string::size_type posRv = valueKey.find(".RV.");
			if(posRv == std::string::npos && pos != std::string::npos)
			{
				std::string baseName = valueKey.substr(0, pos);
				std::string rawParameterName = baseName + ".RAW";
				auto rawParameterIterator = channelIterator->second.find(rawParameterName);
				if(rawParameterIterator == channelIterator->second.end()) return Variable::createError(-9, "Parameter " + rawParameterName + " not found.");
				PParameter rawRpcParameter = rawParameterIterator->second.rpcParameter;
				if(!rawRpcParameter) return Variable::createError(-9, "Parameter " + rawParameterName + " not found.");
				BaseLib::Systems::RpcConfigurationParameter& rawParameter = rawParameterIterator->second;

				if(rawRpcParameter->casts.empty()) return Variable::createError(-10, rawParameterName + " has no cast defined.");
				ParameterCast::PGeneric rawCast = std::dynamic_pointer_cast<ParameterCast::Generic>(rawRpcParameter->casts.at(0));
				if(!rawCast) return Variable::createError(-10, rawParameterName + " hast no cast of type generic defined.");

                if(rpcParameter->physical->bitSize < 1) return Variable::createError(-10, rawParameterName + " has no valid bit size defined.");

				std::vector<uint8_t> rawParameterData = rawParameter.getBinaryData();
				BaseLib::BitReaderWriter::setPosition(rpcParameter->physical->address, rpcParameter->physical->bitSize, rawParameterData, parameterData);
				rawParameter.setBinaryData(rawParameterData);
				if(rawParameter.databaseId > 0) saveParameter(rawParameter.databaseId, rawParameterData);
				else saveParameter(0, ParameterGroup::Type::Enum::variables, channel, rawParameterName, rawParameterData);
				if(_bl->debugLevel >= 4) GD::out.printInfo("Info: " + rawParameterName + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(channel) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(rawParameterData) + ".");

				valueKeys->push_back(rawParameterName);
				values->push_back(_dptConverter->getVariable(rawCast->type, rawParameterData, rawParameter.invert()));
			}

			if(!valueKeys->empty())
			{
                std::string address(_serialNumber + ":" + std::to_string(channel));
                raiseEvent(clientInfo->initInterfaceId, _peerID, channel, valueKeys, values);
                raiseRPCEvent(clientInfo->initInterfaceId, _peerID, channel, address, valueKeys, values);
			}
			return std::make_shared<Variable>(VariableType::tVoid);
		}
		else if(rpcParameter->physical->operationType != IPhysical::OperationType::Enum::command) return Variable::createError(-6, "Parameter is not settable.");
        if(rpcParameter->setPackets.empty() && !rpcParameter->writeable) return Variable::createError(-6, "parameter is read only");

		if(parameter.databaseId > 0) saveParameter(parameter.databaseId, parameterData);
		else saveParameter(0, ParameterGroup::Type::Enum::variables, channel, valueKey, parameterData);
		if(_bl->debugLevel >= 4) GD::out.printInfo("Info: " + valueKey + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(channel) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(parameterData) + ".");

		if(valueKey.size() > 4 && valueKey.compare(valueKey.size() - 4, 4, ".RAW") == 0)
		{
			std::string baseName = valueKey.substr(0, valueKey.size() - 4);
			auto groupedParametersChannelIterator = _groupedParameters.find(channel);
			if(groupedParametersChannelIterator == _groupedParameters.end()) return Variable::createError(-8, "No grouped parameters found.");
			auto groupedParametersIterator = groupedParametersChannelIterator->second.find(baseName);
			if(groupedParametersIterator == groupedParametersChannelIterator->second.end()) return Variable::createError(-8, "No grouped parameters found.");

			for(std::vector<PParameter>::iterator i = groupedParametersIterator->second.parameters.begin(); i != groupedParametersIterator->second.parameters.end(); ++i)
			{
				auto groupedParameterIterator = channelIterator->second.find((*i)->id);
				if(groupedParameterIterator == channelIterator->second.end()) continue;
				PParameter groupedRpcParameter = groupedParameterIterator->second.rpcParameter;
				if(!groupedRpcParameter) continue;
				BaseLib::Systems::RpcConfigurationParameter& groupedParameter = groupedParameterIterator->second;

				if(groupedRpcParameter->casts.empty() || groupedRpcParameter->physical->bitSize < 1) continue;
				ParameterCast::PGeneric groupedCast = std::dynamic_pointer_cast<ParameterCast::Generic>(groupedRpcParameter->casts.at(0));
				if(!groupedCast) continue;

				std::vector<uint8_t> groupedParameterData = BaseLib::BitReaderWriter::getPosition(parameterData, groupedRpcParameter->physical->address, groupedRpcParameter->physical->bitSize);
				if(groupedParameter.equals(groupedParameterData)) continue;
				groupedParameter.setBinaryData(groupedParameterData);
				if(groupedParameter.databaseId > 0) saveParameter(groupedParameter.databaseId, groupedParameterData);
				else saveParameter(0, ParameterGroup::Type::Enum::variables, channel, (*i)->id, groupedParameterData);
				if(_bl->debugLevel >= 4) GD::out.printInfo("Info: " + (*i)->id + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(channel) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(groupedParameterData) + ".");

				valueKeys->push_back((*i)->id);
				values->push_back(_dptConverter->getVariable(groupedCast->type, groupedParameterData, groupedParameter.invert()));
			}
		}

		PCemi cemi;
		if(valueKey.size() > 7 && valueKey.compare(valueKey.size() - 7, 7, ".SUBMIT") == 0 && rpcParameter->logical->type == ILogical::Type::tAction)
		{
			std::string baseName = valueKey.substr(0, valueKey.size() - 7);
			std::string rawParameterName = baseName + ".RAW";
			auto rawParameterIterator = channelIterator->second.find(rawParameterName);
			if(rawParameterIterator == channelIterator->second.end()) return Variable::createError(-9, "Parameter " + rawParameterName + " not found.");
			PParameter rawRpcParameter = rawParameterIterator->second.rpcParameter;
			if(!rawRpcParameter) return Variable::createError(-9, "Parameter " + rawParameterName + " not found.");
			BaseLib::Systems::RpcConfigurationParameter& rawParameter = rawParameterIterator->second;
			if(rawRpcParameter->casts.empty()) return Variable::createError(-10, rawParameterName + " hast no cast defined.");
			ParameterCast::PGeneric rawCast = std::dynamic_pointer_cast<ParameterCast::Generic>(rawRpcParameter->casts.at(0));
			if(!rawCast) return Variable::createError(-10, rawParameterName + " hast no cast of type generic defined.");

			std::vector<uint8_t> rawParameterData = rawParameter.getBinaryData();
            cemi = std::make_shared<Cemi>(Cemi::Operation::groupValueWrite, 0, rpcParameter->physical->address, _dptConverter->fitsInFirstByte(rawCast->type), rawParameterData);
		}
		else cemi = std::make_shared<Cemi>(Cemi::Operation::groupValueWrite, 0, rpcParameter->physical->address, fitsInFirstByte, parameterData);

		sendPacket(cemi);

		if(!valueKeys->empty())
		{
            std::string address(_serialNumber + ":" + std::to_string(channel));
            raiseEvent(clientInfo->initInterfaceId, _peerID, channel, valueKeys, values);
            raiseRPCEvent(clientInfo->initInterfaceId, _peerID, channel, address, valueKeys, values);
		}

		return std::make_shared<Variable>(VariableType::tVoid);
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return Variable::createError(-32500, "Unknown application error. See error log for more details.");
}

}
