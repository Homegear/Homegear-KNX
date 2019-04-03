/* Copyright 2013-2019 Homegear GmbH */

#include "Search.h"
#include "GD.h"

namespace MyFamily
{

Search::Search(BaseLib::SharedObjects* baseLib) : _bl(baseLib)
{
}

void Search::addDeviceToPeerInfo(PHomegearDevice& device, int32_t address, std::string name, std::string room, std::vector<PeerInfo>& peerInfo, std::map<int32_t, std::string>& usedTypes)
{
	try
	{
		std::string filename = _xmlPath + GD::bl->hf.stringReplace(device->supportedDevices.at(0)->id, "/", "_") + ".xml";
		device->save(filename);

		PeerInfo info;
		info.type = device->supportedDevices.at(0)->typeNumber;
		if(info.type == 0)
		{
			GD::out.printError("Error: Not adding device \"" + device->supportedDevices.at(0)->id + "\" as no type ID was specified in the JSON defined in ETS. Please add a unique type ID there.");
			return;
		}
		if(usedTypes.find(info.type) != usedTypes.end())
		{
			GD::out.printError("Error: Type ID " + std::to_string(info.type) + " is used by at least two devices (" + device->supportedDevices.at(0)->id + " and " + usedTypes[info.type] + "). Type IDs need to be unique per device. Please correct the JSON in ETS.");
			return;
		}
		usedTypes.emplace(info.type, device->supportedDevices.at(0)->id);
		std::string paddedType = std::to_string(info.type);
		if(paddedType.size() < 9) paddedType.insert(0, 9 - paddedType.size(), '0');
		info.serialNumber = "KNX" + paddedType;
		info.address = address;
		info.name = std::move(name);
		info.room = std::move(room);
		peerInfo.push_back(info);
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

std::shared_ptr<HomegearDevice> Search::createHomegearDevice(const Search::DeviceXmlData& deviceInfo, std::unordered_set<uint32_t>& usedTypeNumbers, std::unordered_map<std::string, uint32_t>& idTypeNumberMap)
{
    try
    {
        if(deviceInfo.address == -1) return PHomegearDevice();
        std::shared_ptr<HomegearDevice> device = std::make_shared<HomegearDevice>(_bl);
        device->version = 1;
        PSupportedDevice supportedDevice = std::make_shared<SupportedDevice>(_bl, device.get());
        bool newDevice = true;
        auto typeNumberIterator = idTypeNumberMap.find(deviceInfo.id); //Backwards compatability
        if(typeNumberIterator != idTypeNumberMap.end() && typeNumberIterator->second > 0)
        {
            supportedDevice->typeNumber = typeNumberIterator->second;
            newDevice = false;
            std::string deviceId = deviceInfo.id;
            BaseLib::Io::deleteFile(_xmlPath + GD::bl->hf.stringReplace(deviceId, "/", "_") + ".xml");
        }
        else
        {
            typeNumberIterator = idTypeNumberMap.find(MyPacket::getFormattedPhysicalAddress(deviceInfo.address));
            if(typeNumberIterator != idTypeNumberMap.end() && typeNumberIterator->second > 0)
            {
                supportedDevice->typeNumber = typeNumberIterator->second;
                newDevice = false;
            }
        }
        if(newDevice)
        {
            for(uint32_t i = 1; i <= 65535; i++)
            {
                if(usedTypeNumbers.find(i) == usedTypeNumbers.end())
                {
                    supportedDevice->typeNumber = i;
                    usedTypeNumbers.emplace(i);
                    break;
                }
            }
        }
        if(supportedDevice->typeNumber == 0)
        {
            GD::out.printError("Error: Can't add KNX device. No free type number could be found. The maximum number of KNX devices is 65535.");
            return PHomegearDevice();
        }

        supportedDevice->id = MyPacket::getFormattedPhysicalAddress(deviceInfo.address);
        supportedDevice->description = deviceInfo.name;
        device->supportedDevices.push_back(supportedDevice);

        createXmlMaintenanceChannel(device);

        for(auto groupVariable : deviceInfo.variables)
        {
            if(deviceInfo.description->structValue->empty()) continue;

            int32_t channel = 1;
            std::string variableName;
            std::string unit;

            auto variableIterator = deviceInfo.description->structValue->find(std::to_string(groupVariable.first));
            if(variableIterator == deviceInfo.description->structValue->end()) continue;

            auto structIterator = variableIterator->second->structValue->find("channel");
            if(structIterator != variableIterator->second->structValue->end()) channel = structIterator->second->integerValue;

            structIterator = variableIterator->second->structValue->find("variable");
            if(structIterator != variableIterator->second->structValue->end()) variableName = _bl->hf.stringReplace(structIterator->second->stringValue, ".", "_");;

            structIterator = variableIterator->second->structValue->find("unit");
            if(structIterator != variableIterator->second->structValue->end()) unit = structIterator->second->stringValue;

            PFunction function;
            auto functionIterator = device->functions.find((uint32_t)channel);
            if(functionIterator == device->functions.end())
            {
                function.reset(new Function(_bl));
                function->channel = (uint32_t)channel;
                function->type = "KNX_CHANNEL_" + std::to_string(channel);
                function->variablesId = "knx_values_" + std::to_string(channel);
                device->functions[function->channel] = function;
            }
            else function = functionIterator->second;

            PParameter parameter = createParameter(function, variableName.empty() ? "VALUE" : variableName, groupVariable.second->datapointType, unit, IPhysical::OperationType::command, groupVariable.second->readFlag, groupVariable.second->writeFlag, groupVariable.second->address);
            if(!parameter) continue;
            parameter->transmitted = groupVariable.second->transmitFlag;

            parseDatapointType(function, groupVariable.second->datapointType, parameter);

            if(!parameter->casts.empty())
            {
                function->variables->parametersOrdered.push_back(parameter);
                function->variables->parameters[parameter->id] = parameter;
            }
        }

        return device;
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
    return PHomegearDevice();
}

std::vector<Search::PeerInfo> Search::search(std::unordered_set<uint32_t>& usedTypeNumbers, std::unordered_map<std::string, uint32_t>& idTypeNumberMap)
{
	std::vector<Search::PeerInfo> peerInfo;
	try
	{
		std::vector<std::shared_ptr<std::vector<char>>> knxProjectFiles = extractKnxProjectFiles();
		if(knxProjectFiles.empty())
		{
			GD::out.printError("Error: Could not search for KNX devices. No KNX project files were found.");
			return peerInfo;
		}

		auto xmlData = extractXmlData(knxProjectFiles);
		if(xmlData.groupVariableXmlData.empty() && xmlData.deviceXmlData.empty())
		{
			GD::out.printError("Error: Could not search for KNX devices. No group addresses were found in KNX project file.");
			return peerInfo;
		}

		createDirectories();

		//{{{ Group variables
			std::map<std::string, PHomegearDevice> rpcDevicesPlain;
			std::map<std::string, PHomegearDevice> rpcDevicesJson;
			for(auto& variableXml : xmlData.groupVariableXmlData)
			{
				std::string id;
				int32_t type = -1;
				int32_t channel = 1;
				std::string variableName;
				std::string unit;
				bool readable = true;
				bool writeable = true;

				if(variableXml->description)
				{
					auto structIterator = variableXml->description->structValue->find("id");
					if(structIterator != variableXml->description->structValue->end())
					{
						id = structIterator->second->stringValue;

						structIterator = variableXml->description->structValue->find("type");
						if(structIterator != variableXml->description->structValue->end()) type = structIterator->second->integerValue;
						if(type > 9999999)
						{
							GD::out.printError("Error: Type number too large: " + std::to_string(type));
							continue;
						}

						structIterator = variableXml->description->structValue->find("channel");
						if(structIterator != variableXml->description->structValue->end()) channel = structIterator->second->integerValue;
					}

					structIterator = variableXml->description->structValue->find("variable");
					if(structIterator != variableXml->description->structValue->end()) variableName = _bl->hf.stringReplace(structIterator->second->stringValue, ".", "_");;

					structIterator = variableXml->description->structValue->find("unit");
					if(structIterator != variableXml->description->structValue->end()) unit = structIterator->second->stringValue;

					structIterator = variableXml->description->structValue->find("readable");
					if(structIterator != variableXml->description->structValue->end()) readable = structIterator->second->booleanValue;

					structIterator = variableXml->description->structValue->find("writeable");
					if(structIterator != variableXml->description->structValue->end()) writeable = structIterator->second->booleanValue;
				}

				if(id.empty())
				{
					PHomegearDevice device(new HomegearDevice(_bl));
					device->version = 1;
					PSupportedDevice supportedDevice(new SupportedDevice(_bl, device.get()));
					supportedDevice->id = "KNX_" + std::to_string(variableXml->address >> 11) + "_" + std::to_string((variableXml->address >> 8) & 0x7) + "_" + std::to_string(variableXml->address & 0xFF);
					supportedDevice->description = "KNX_" + std::to_string(variableXml->address >> 11) + "/" + std::to_string((variableXml->address >> 8) & 0x7) + "/" + std::to_string(variableXml->address & 0xFF);
					supportedDevice->typeNumber = variableXml->address;
					device->supportedDevices.push_back(supportedDevice);
					rpcDevicesPlain[supportedDevice->id] = device;

					createXmlMaintenanceChannel(device);

					PFunction function(new Function(_bl));
					function->channel = 1;
					function->type = "KNX_GROUP_VARIABLE";
					function->variablesId = "knx_values";
					device->functions[function->channel] = function;

					PParameter parameter = createParameter(function, variableName.empty() ? "VALUE" : variableName, variableXml->datapointType, unit, IPhysical::OperationType::command, readable, writeable, variableXml->address);
					if(!parameter) continue;

					parseDatapointType(function, variableXml->datapointType, parameter);

					if(!parameter->casts.empty())
					{
						function->variables->parametersOrdered.push_back(parameter);
						function->variables->parameters[parameter->id] = parameter;
					}
				}
				else
				{
					std::shared_ptr<HomegearDevice> device;
					auto deviceIterator = rpcDevicesJson.find(id);
					if(deviceIterator == rpcDevicesJson.end())
					{
						device.reset(new HomegearDevice(_bl));
						device->version = 1;
						PSupportedDevice supportedDevice(new SupportedDevice(_bl, device.get()));
						supportedDevice->id = id;
						supportedDevice->description = supportedDevice->id;
						if(type != -1) supportedDevice->typeNumber = (uint32_t)type + 65535;
						device->supportedDevices.push_back(supportedDevice);
						rpcDevicesJson[supportedDevice->id] = device;

						createXmlMaintenanceChannel(device);
					}
					else
					{
						device = deviceIterator->second;
						if(type != -1)
						{
							if(device->supportedDevices.at(0)->typeNumber == 0) device->supportedDevices.at(0)->typeNumber = (uint32_t)type + 65535;
							else if((int32_t)device->supportedDevices.at(0)->typeNumber != type + 65535)
							{
								GD::out.printError("Error: Device with ID \"" + id + "\" has group variables with different type IDs specified (at least " + std::to_string(type) + " and " + std::to_string(device->supportedDevices.at(0)->typeNumber - 65535) + "). Please check the JSON defined in ETS. Only one unique type ID is allowed per device.");
							}
						}
					}

					PFunction function;
					auto functionIterator = device->functions.find((uint32_t)channel);
					if(functionIterator == device->functions.end())
					{
						function.reset(new Function(_bl));
						function->channel = (uint32_t)channel;
						function->type = "KNX_CHANNEL_" + std::to_string(channel);
						function->variablesId = "knx_values_" + std::to_string(channel);
						device->functions[function->channel] = function;
					}
					else function = functionIterator->second;

					PParameter parameter = createParameter(function, variableName.empty() ? "VALUE" : variableName, variableXml->datapointType, unit, IPhysical::OperationType::command, readable, writeable, variableXml->address);
					if(!parameter) continue;

					parseDatapointType(function, variableXml->datapointType, parameter);

					if(!parameter->casts.empty())
					{
						function->variables->parametersOrdered.push_back(parameter);
						function->variables->parameters[parameter->id] = parameter;
					}
				}
			}
		//}}}

		///{{{ Devices
			std::unordered_map<std::string, PHomegearDevice> rpcDevicesDevice;
			std::unordered_map<std::string, std::string> rooms;
			std::unordered_map<std::string, int32_t> addresses;
			for(auto& deviceXml : xmlData.deviceXmlData)
			{
			    if(deviceXml->address == -1)
                {
                    GD::out.printInfo("Info: Ignoring device with ID \"" + deviceXml->id + "\", because it has no physical address.");
                    continue;
                }
                auto device = createHomegearDevice(*deviceXml, usedTypeNumbers, idTypeNumberMap);
                if(device)
				{
					auto typeId = (*device->supportedDevices.begin())->id;
					rooms[typeId] = deviceXml->room;
					addresses[typeId] = deviceXml->address;
					rpcDevicesDevice[typeId] = device;
				}
			}
		//}}}

		std::map<int32_t, std::string> usedTypeIds;
		if(!xmlData.jsonExists) //Only add group variables without JSON when no JSON exists
		{
			for(auto& i : rpcDevicesPlain)
			{
				addDeviceToPeerInfo(i.second, -1, "", "", peerInfo, usedTypeIds);
			}
		}

		for(auto& i : rpcDevicesJson)
		{
			addDeviceToPeerInfo(i.second, -1, "", "", peerInfo, usedTypeIds);
		}

		for(auto& device : rpcDevicesDevice)
		{
			addDeviceToPeerInfo(device.second, addresses[device.first], device.second->supportedDevices.at(0)->description, rooms[device.first], peerInfo, usedTypeIds);
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
	return peerInfo;
}

Search::PeerInfo Search::updateDevice(std::unordered_set<uint32_t>& usedTypeNumbers, std::unordered_map<std::string, uint32_t>& idTypeNumberMap, BaseLib::PVariable deviceInfo)
{
    try
    {
        createDirectories();

        DeviceXmlData deviceXml;

        auto structIterator = deviceInfo->structValue->find("id");
        if(structIterator == deviceInfo->structValue->end())
        {
            GD::out.printError("Error: Could not create KNX device information: Field \"id\" is missing.");
            return PeerInfo();
        }
        deviceXml.id = structIterator->second->stringValue;

        structIterator = deviceInfo->structValue->find("name");
        if(structIterator == deviceInfo->structValue->end())
        {
            GD::out.printError("Error: Could not create KNX device information: Field \"name\" is missing.");
            return PeerInfo();
        }
        deviceXml.name = structIterator->second->stringValue;

        structIterator = deviceInfo->structValue->find("room");
        if(structIterator == deviceInfo->structValue->end())
        {
            GD::out.printError("Error: Could not create KNX device information: Field \"room\" is missing.");
            return PeerInfo();
        }
        deviceXml.room = structIterator->second->stringValue;

        structIterator = deviceInfo->structValue->find("address");
        if(structIterator == deviceInfo->structValue->end() || structIterator->second->integerValue == -1)
        {
            GD::out.printError("Error: Could not create KNX device information: Field \"address\" is missing or the address invalid.");
            return PeerInfo();
        }
        deviceXml.address = structIterator->second->integerValue;

        structIterator = deviceInfo->structValue->find("description");
        if(structIterator == deviceInfo->structValue->end())
        {
            GD::out.printError("Error: Could not create KNX device information: Field \"description\" is missing.");
            return PeerInfo();
        }
        std::string description = structIterator->second->stringValue;
        std::string::size_type jsonStartPos = description.find("$${");
        if(jsonStartPos != std::string::npos)
        {
            description = description.substr(jsonStartPos + 2);
            std::string jsonString;
            BaseLib::Html::unescapeHtmlEntities(description, jsonString);
            try
            {
                BaseLib::Rpc::JsonDecoder jsonDecoder(_bl);
                deviceXml.description = jsonDecoder.decode(jsonString);
            }
            catch(const Exception& ex)
            {
                _bl->out.printError("Error decoding JSON in KNX device information: " + ex.what());
                return PeerInfo();
            }
        }

        structIterator = deviceInfo->structValue->find("variables");
        if(structIterator == deviceInfo->structValue->end())
        {
            GD::out.printError("Error: Could not create KNX device information: Field \"variables\" is missing.");
            return PeerInfo();
        }

        for(auto& variableElement : *structIterator->second->structValue)
        {
            if(!BaseLib::Math::isNumber(variableElement.first))
            {
                GD::out.printError("Error: key within Struct \"variables\" is no number.");
                return PeerInfo();
            }

            auto variable = std::make_shared<GroupVariableXmlData>();

            auto variableIterator = variableElement.second->structValue->find("address");
            if(variableIterator == variableElement.second->structValue->end() || variableIterator->second->integerValue == -1)
            {
                GD::out.printError("Error: Group variable with index " + variableElement.first + " has no field \"address\" or the address is invalid.");
                return PeerInfo();
            }
            variable->address = variableIterator->second->integerValue;

            variableIterator = variableElement.second->structValue->find("datapointType");
            if(variableIterator == variableElement.second->structValue->end())
            {
                GD::out.printError("Error: Group variable with index " + variableElement.first + " has no field \"datapointType\".");
                return PeerInfo();
            }
            variable->datapointType = variableIterator->second->stringValue;

            variableIterator = variableElement.second->structValue->find("readFlag");
            if(variableIterator == variableElement.second->structValue->end())
            {
                GD::out.printError("Error: Group variable with index " + variableElement.first + " has no field \"readFlag\".");
                return PeerInfo();
            }
            variable->readFlag = variableIterator->second->booleanValue;

            variableIterator = variableElement.second->structValue->find("writeFlag");
            if(variableIterator == variableElement.second->structValue->end())
            {
                GD::out.printError("Error: Group variable with index " + variableElement.first + " has no field \"writeFlag\".");
                return PeerInfo();
            }
            variable->writeFlag = variableIterator->second->booleanValue;

            variableIterator = variableElement.second->structValue->find("transmitFlag");
            if(variableIterator == variableElement.second->structValue->end())
            {
                GD::out.printError("Error: Group variable with index " + variableElement.first + " has no field \"transmitFlag\".");
                return PeerInfo();
            }
            variable->transmitFlag = variableIterator->second->booleanValue;

            variable->index = BaseLib::Math::getNumber(variableElement.first);

            deviceXml.variables.emplace(variable->index, std::move(variable));
        }

        auto device = createHomegearDevice(deviceXml, usedTypeNumbers, idTypeNumberMap);
        if(!device)
        {
            GD::out.printError("Error: Could not create KNX device information: Could not generate device info.");
            return PeerInfo();
        }
        auto typeId = (*device->supportedDevices.begin())->id;

        std::vector<PeerInfo> peerInfo;
        std::map<int32_t, std::string> usedTypeIds;

        addDeviceToPeerInfo(device, deviceXml.address, (*device->supportedDevices.begin())->description, deviceXml.room, peerInfo, usedTypeIds);

        if(!peerInfo.empty()) return *peerInfo.begin();
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
    return PeerInfo();
}

void Search::createDirectories()
{
	try
	{
		uid_t localUserId = _bl->hf.userId(GD::bl->settings.dataPathUser());
		gid_t localGroupId = _bl->hf.groupId(GD::bl->settings.dataPathGroup());
		if(((int32_t)localUserId) == -1 || ((int32_t)localGroupId) == -1)
		{
			localUserId = _bl->userId;
			localGroupId = _bl->groupId;
		}

		std::string path1 = _bl->settings.familyDataPath();
		std::string path2 = path1 + std::to_string(GD::family->getFamily()) + "/";
		_xmlPath = path2 + "desc/";
		if(!BaseLib::Io::directoryExists(path1)) BaseLib::Io::createDirectory(path1, GD::bl->settings.dataPathPermissions());
		if(localUserId != 0 || localGroupId != 0)
		{
			if(chown(path1.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set owner on " << path1 << std::endl;
			if(chmod(path1.c_str(), GD::bl->settings.dataPathPermissions()) == -1) std::cerr << "Could not set permissions on " << path1 << std::endl;
		}
		if(!BaseLib::Io::directoryExists(path2)) BaseLib::Io::createDirectory(path2, GD::bl->settings.dataPathPermissions());
		if(localUserId != 0 || localGroupId != 0)
		{
			if(chown(path2.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set owner on " << path2 << std::endl;
			if(chmod(path2.c_str(), GD::bl->settings.dataPathPermissions()) == -1) std::cerr << "Could not set permissions on " << path2 << std::endl;
		}
		if(!BaseLib::Io::directoryExists(_xmlPath)) BaseLib::Io::createDirectory(_xmlPath, GD::bl->settings.dataPathPermissions());
		if(localUserId != 0 || localGroupId != 0)
		{
			if(chown(_xmlPath.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set owner on " << _xmlPath << std::endl;
			if(chmod(_xmlPath.c_str(), GD::bl->settings.dataPathPermissions()) == -1) std::cerr << "Could not set permissions on " << _xmlPath << std::endl;
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

void Search::createXmlMaintenanceChannel(PHomegearDevice& device)
{
	// {{{ Channel 0
		PFunction function(new Function(_bl));
		function->channel = 0;
		function->type = "KNX_MAINTENANCE";
		function->variablesId = "knx_maintenance_values";
		device->functions[function->channel] = function;

		PParameter parameter(new Parameter(_bl, function->variables.get()));
		parameter->id = "UNREACH";
		function->variables->parametersOrdered.push_back(parameter);
		function->variables->parameters[parameter->id] = parameter;
		parameter->writeable = false;
		parameter->service = true;
		parameter->logical = std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl);;
		parameter->physical = std::make_shared<BaseLib::DeviceDescription::PhysicalInteger>(_bl);
		parameter->physical->groupId = parameter->id;
		parameter->physical->operationType = IPhysical::OperationType::internal;

		parameter.reset(new Parameter(_bl, function->variables.get()));
		parameter->id = "STICKY_UNREACH";
		function->variables->parametersOrdered.push_back(parameter);
		function->variables->parameters[parameter->id] = parameter;
		parameter->sticky = true;
		parameter->service = true;
		parameter->logical = std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl);;
		parameter->physical = std::make_shared<BaseLib::DeviceDescription::PhysicalInteger>(_bl);
		parameter->physical->groupId = parameter->id;
		parameter->physical->operationType = IPhysical::OperationType::internal;
	// }}}
}

std::vector<std::shared_ptr<std::vector<char>>> Search::extractKnxProjectFiles()
{
	std::vector<std::shared_ptr<std::vector<char>>> contents;
	try
	{
		std::string projectPath = _bl->settings.deviceDescriptionPath() + std::to_string(GD::family->getFamily()) + '/';
		std::vector<std::string> files = _bl->io.getFiles(projectPath);
		std::vector<std::string> projectFilenames;
		for(auto& file : files)
		{
			if(file.size() > 8 && file.compare(file.size() - 8, 8, ".knxproj") == 0)
			{
				projectFilenames.push_back(projectPath + file);
			}
		}
		if(projectFilenames.empty())
		{
			GD::out.printError("Error: Could not find any project archive in " + GD::bl->settings.deviceDescriptionPath() + std::to_string(MY_FAMILY_ID) + "/.");
			return contents;
		}

		for(auto& projectFilename : projectFilenames)
		{
			std::shared_ptr<std::vector<char>> content(new std::vector<char>());

			int32_t error = 0;
			zip* projectArchive = zip_open(projectFilename.c_str(), 0, &error);
			if(!projectArchive)
			{
				GD::out.printError("Error: Could not open project archive. Error code: " + std::to_string(error));
				continue;
			}

			zip_int64_t filesInArchive = zip_get_num_entries(projectArchive, 0);
			for(zip_uint64_t i = 0; i < (zip_uint64_t)filesInArchive; ++i)
			{
				struct zip_stat st{};
				zip_stat_init(&st);
				if(zip_stat_index(projectArchive, i, 0, &st) == -1)
				{
					GD::out.printError("Error: Could not get information for project file at index " + std::to_string(i));
					continue;
				}
				if(!(st.valid & ZIP_STAT_NAME) || !(st.valid & ZIP_STAT_SIZE))
				{
					GD::out.printError("Error: Could not get name or size for project file at index " + std::to_string(i));
					continue;
				}

				std::string filename(st.name);
				if(filename.size() < 6) continue;
				if(filename.compare(filename.size() - 6, 6, "/0.xml") != 0) continue;

				content->resize(st.size + 1);
				zip_file* projectFile = zip_fopen(projectArchive, filename.c_str(), 0);
				if(!projectFile)
				{
					GD::out.printError("Error: Could not open project file in archive. Error code: " + std::to_string(error));
					continue;
				}
				if(zip_fread(projectFile, &content->at(0), st.size) != (signed)st.size)
				{
					GD::out.printError("Error: Could not read project file in archive. Error code: " + std::to_string(error));
					continue;
				}
				content->push_back(0);
				zip_fclose(projectFile);

				if(!content->empty()) contents.push_back(content);

				break;
			}

			zip_close(projectArchive);
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
	return contents;
}

void Search::assignRoomsToDevices(xml_node<>* currentNode, std::string currentRoom, std::unordered_map<std::string, std::shared_ptr<DeviceXmlData>>& devices)
{
	try
	{
		for(xml_node<>* buildingPartNode = currentNode->first_node("BuildingPart"); buildingPartNode; buildingPartNode = buildingPartNode->next_sibling("BuildingPart"))
		{
			std::string room;
			xml_attribute<>* attribute = buildingPartNode->first_attribute("Type");
			if(attribute)
			{
				std::string type = std::string(attribute->value());
				if(type == "Room")
				{
					attribute = buildingPartNode->first_attribute("Name");
					if(attribute) room = std::string(attribute->value());
				}
			}

			if(room.empty()) room = currentRoom;
			assignRoomsToDevices(buildingPartNode, room, devices);
		}

		if(!currentRoom.empty())
		{
			for(xml_node<>* deviceNode = currentNode->first_node("DeviceInstanceRef"); deviceNode; deviceNode = deviceNode->next_sibling("DeviceInstanceRef"))
			{
				xml_attribute<>* attribute = deviceNode->first_attribute("RefId");
				if(!attribute) continue;
				std::string deviceId = std::string(attribute->value());
				if(deviceId.empty()) continue;
				auto devicesIterator = devices.find(deviceId);
				if(devicesIterator == devices.end()) continue;
				devicesIterator->second->room = currentRoom;
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

Search::XmlData Search::extractXmlData(std::vector<std::shared_ptr<std::vector<char>>>& knxProjectFiles)
{
	XmlData xmlData;
	BaseLib::Rpc::JsonDecoder jsonDecoder(_bl);
	for(auto& knxProjectFile : knxProjectFiles)
	{
		xml_document<> doc;
		try
		{
			const char* startPos = (char*)memchr(&knxProjectFile->at(0), '<', 10);
			if(!startPos)
			{
				_bl->out.printError("Error: No '<' found in KNX project XML.");
				doc.clear();
				continue;
			}
			doc.parse<parse_no_entity_translation | parse_validate_closing_tags>(&knxProjectFile->at(0));
			xml_node<>* rootNode = doc.first_node("KNX");
			if(!rootNode)
			{
				_bl->out.printError("Error: KNX project XML does not start with \"KNX\".");
				doc.clear();
				continue;
			}

			std::unordered_map<std::string, std::shared_ptr<DeviceXmlData>> deviceById;
			std::unordered_map<std::string, std::set<std::shared_ptr<DeviceXmlData>>> deviceByGroupVariable;
			for(xml_node<>* projectNode = rootNode->first_node("Project"); projectNode; projectNode = projectNode->next_sibling("Project"))
			{
				for(xml_node<>* installationsNode = projectNode->first_node("Installations"); installationsNode; installationsNode = installationsNode->next_sibling("Installations"))
				{
					for(xml_node<>* installationNode = installationsNode->first_node("Installation"); installationNode; installationNode = installationNode->next_sibling("Installation"))
					{
						for(xml_node<>* topologyNode = installationNode->first_node("Topology"); topologyNode; topologyNode = topologyNode->next_sibling("Topology"))
						{
							int32_t currentArea = -1;
							int32_t currentLine = -1;
							int32_t currentAddress = -1;
							for(xml_node<>* areaNode = topologyNode->first_node("Area"); areaNode; areaNode = areaNode->next_sibling("Area"))
							{
								std::string attributeValue;
								xml_attribute<>* attribute = areaNode->first_attribute("Address");
								if(!attribute) continue;
								attributeValue = std::string(attribute->value());
								currentArea = Math::getNumber(attributeValue) & 0x0F;

								for(xml_node<>* lineNode = areaNode->first_node("Line"); lineNode; lineNode = lineNode->next_sibling("Line"))
								{
									attribute = lineNode->first_attribute("Address");
									if(!attribute) continue;
									attributeValue = std::string(attribute->value());
									currentLine = Math::getNumber(attributeValue) & 0x0F;

									for(xml_node<>* deviceNode = lineNode->first_node("DeviceInstance"); deviceNode; deviceNode = deviceNode->next_sibling("DeviceInstance"))
									{
										std::shared_ptr<DeviceXmlData> device = std::make_shared<DeviceXmlData>();

										attribute = deviceNode->first_attribute("Address");
										if(!attribute) continue;
										attributeValue = std::string(attribute->value());
										currentAddress = Math::getNumber(attributeValue) & 0xFF;
										device->address = (currentArea << 12) | (currentLine << 8) | currentAddress;

										attribute = deviceNode->first_attribute("Id");
										if(!attribute) continue;
										device->id = std::string(attribute->value());

										attribute = deviceNode->first_attribute("Name");
										if(attribute) device->name = std::string(attribute->value());

										attribute = deviceNode->first_attribute("Description");
										if(attribute) attributeValue = std::string(attribute->value()); else attributeValue = "";
										std::string::size_type jsonStartPos = attributeValue.find("$${");
										if(jsonStartPos != std::string::npos)
										{
											xmlData.jsonExists = true;
											attributeValue = attributeValue.substr(jsonStartPos + 2);
											std::string jsonString;
											BaseLib::Html::unescapeHtmlEntities(attributeValue, jsonString);
											try
											{
												BaseLib::PVariable json = jsonDecoder.decode(jsonString);
												device->description = json;
											}
											catch(const Exception& ex)
											{
												_bl->out.printError("Error decoding JSON of device \"" + device->name + "\" with ID \"" + device->id + "\": " + ex.what());
												continue;
											}
										}
										else continue;

										for(xml_node<>* comInstanceRefsNode = deviceNode->first_node("ComObjectInstanceRefs"); comInstanceRefsNode; comInstanceRefsNode = comInstanceRefsNode->next_sibling("ComObjectInstanceRefs"))
										{
											for(xml_node<>* comInstanceRefNode = comInstanceRefsNode->first_node("ComObjectInstanceRef"); comInstanceRefNode; comInstanceRefNode = comInstanceRefNode->next_sibling("ComObjectInstanceRef"))
											{
												GroupVariableInfo variableInfo;
												attribute = deviceNode->first_attribute("WriteFlag");
												if(attribute)
												{
													attributeValue = std::string(attribute->value());
													variableInfo.writeFlag = attributeValue != "Disabled";
												}

												attribute = deviceNode->first_attribute("ReadFlag");
												if(attribute)
												{
													attributeValue = std::string(attribute->value());
													variableInfo.readFlag = attributeValue != "Disabled";
												}

                                                attribute = deviceNode->first_attribute("TransmitFlag");
                                                if(attribute)
                                                {
                                                    attributeValue = std::string(attribute->value());
                                                    variableInfo.transmitFlag = attributeValue != "Disabled";
                                                }

												attribute = comInstanceRefNode->first_attribute("RefId");
												if(attribute)
												{
													attributeValue = std::string(attribute->value());
													std::vector<std::string> parts = BaseLib::HelperFunctions::splitAll(attributeValue, '_');
													if(parts.size() >= 3)
													{
														auto indexPair = BaseLib::HelperFunctions::splitLast(parts.at(2), '-');
														variableInfo.index = BaseLib::Math::getNumber(indexPair.second, false);
													}
												}

												if(variableInfo.index == -1) continue;

                                                attribute = comInstanceRefNode->first_attribute("Links");
                                                if(attribute) //>= ETS5.7
                                                {
                                                    attributeValue = std::string(attribute->value());
                                                    std::vector<std::string> groupAddresses = BaseLib::HelperFunctions::splitAll(attributeValue, ' ');
                                                    for(int32_t i = 0; i < groupAddresses.size(); i++)
                                                    {
                                                        auto& groupAddress = groupAddresses[i];
                                                        deviceByGroupVariable[groupAddress].emplace(device);

                                                        if(i > 0)
                                                        {
                                                            //Only first entry is writeable
                                                            GroupVariableInfo receiveVariableInfo = variableInfo;
                                                            receiveVariableInfo.writeFlag = false;
                                                        }

                                                        //Needs to be a list, because the same group variable might be assigned to one device more than once
                                                        device->variableInfo[groupAddress].push_back(variableInfo);
                                                    }
                                                }
                                                else
                                                {
                                                    for(xml_node<>* connectorsNode = comInstanceRefNode->first_node("Connectors"); connectorsNode; connectorsNode = connectorsNode->next_sibling("Connectors"))
                                                    {
                                                        for(xml_node<>* sendNode = connectorsNode->first_node("Send"); sendNode; sendNode = sendNode->next_sibling("Send"))
                                                        {
                                                            attribute = sendNode->first_attribute("GroupAddressRefId");
                                                            if(!attribute) continue;
                                                            attributeValue = std::string(attribute->value());
                                                            if(attributeValue.empty()) continue;
                                                            deviceByGroupVariable[attributeValue].emplace(device);
                                                            //Needs to be a list, because the same group variable might be assigned to one device more than once
                                                            device->variableInfo[attributeValue].push_back(variableInfo);
                                                        }

                                                        for(xml_node<>* receiveNode = connectorsNode->first_node("Receive"); receiveNode; receiveNode = receiveNode->next_sibling("Receive"))
                                                        {
                                                            attribute = receiveNode->first_attribute("GroupAddressRefId");
                                                            if(!attribute) continue;
                                                            attributeValue = std::string(attribute->value());
                                                            if(attributeValue.empty()) continue;
                                                            deviceByGroupVariable[attributeValue].emplace(device);
                                                            GroupVariableInfo receiveVariableInfo = variableInfo;
                                                            receiveVariableInfo.writeFlag = false;
                                                            //Needs to be a list, because the same group variable might be assigned to one device more than once
                                                            device->variableInfo[attributeValue].push_back(receiveVariableInfo);
                                                        }
                                                    }
                                                }
											}
										}

										deviceById.emplace(device->id, device);
										xmlData.deviceXmlData.emplace(device);
									}
								}
							}
						}
					}
				}
			}

			for(xml_node<>* projectNode = rootNode->first_node("Project"); projectNode; projectNode = projectNode->next_sibling("Project"))
			{
				for(xml_node<>* installationsNode = projectNode->first_node("Installations"); installationsNode; installationsNode = installationsNode->next_sibling("Installations"))
				{
					for(xml_node<>* installationNode = installationsNode->first_node("Installation"); installationNode; installationNode = installationNode->next_sibling("Installation"))
					{
						for(xml_node<>* buildingsNode = installationNode->first_node("Buildings"); buildingsNode; buildingsNode = buildingsNode->next_sibling("Buildings"))
						{
							assignRoomsToDevices(buildingsNode, "", deviceById);
						}
					}
				}
			}

			for(xml_node<>* projectNode = rootNode->first_node("Project"); projectNode; projectNode = projectNode->next_sibling("Project"))
			{
				for(xml_node<>* installationsNode = projectNode->first_node("Installations"); installationsNode; installationsNode = installationsNode->next_sibling("Installations"))
				{
					for(xml_node<>* installationNode = installationsNode->first_node("Installation"); installationNode; installationNode = installationNode->next_sibling("Installation"))
					{
						for(xml_node<>* groupAdressesNode = installationNode->first_node("GroupAddresses"); groupAdressesNode; groupAdressesNode = groupAdressesNode->next_sibling("GroupAddresses"))
						{
							for(xml_node<>* groupRangesNode = groupAdressesNode->first_node("GroupRanges"); groupRangesNode; groupRangesNode = groupRangesNode->next_sibling("GroupRanges"))
							{
								for(xml_node<>* mainGroupNode = groupRangesNode->first_node("GroupRange"); mainGroupNode; mainGroupNode = mainGroupNode->next_sibling("GroupRange"))
								{
									xml_attribute<>* attribute = mainGroupNode->first_attribute("Name");
									if(!attribute) continue;
									std::string mainGroupName(attribute->value());

									for(xml_node<>* middleGroupNode = mainGroupNode->first_node("GroupRange"); middleGroupNode; middleGroupNode = middleGroupNode->next_sibling("GroupRange"))
									{
										attribute = middleGroupNode->first_attribute("Name");
										if(!attribute) continue;
										std::string middleGroupName(attribute->value());

										for(xml_node<>* groupAddressNode = middleGroupNode->first_node("GroupAddress"); groupAddressNode; groupAddressNode = groupAddressNode->next_sibling("GroupAddress"))
										{
											std::shared_ptr<GroupVariableXmlData> element = std::make_shared<GroupVariableXmlData>();
											element->mainGroupName = mainGroupName;
											element->middleGroupName = middleGroupName;

											attribute = groupAddressNode->first_attribute("Id");
											if(!attribute) continue;
											std::string id = std::string(attribute->value());
											if(id.empty()) continue;
											std::string shortId = BaseLib::HelperFunctions::splitLast(id, '_').second;
											if(shortId.empty()) continue;

											attribute = groupAddressNode->first_attribute("Name");
											if(attribute) element->groupVariableName = std::string(attribute->value());

											attribute = groupAddressNode->first_attribute("Address");
											if(!attribute) continue;
											std::string attributeValue(attribute->value());
											element->address = (uint16_t)BaseLib::Math::getNumber(attributeValue);

											attribute = groupAddressNode->first_attribute("DatapointType");
											if(!attribute)
											{
												GD::out.printWarning("Warning: Group variable has no datapoint type: " + std::to_string(element->address >> 11) + "/" + std::to_string((element->address >> 8) & 0x7) + "/" + std::to_string(element->address & 0xFF) + ". The group variable does not work.");
												continue;
											}
											element->datapointType = std::string(attribute->value());

											attribute = groupAddressNode->first_attribute("Description");
											if(attribute) attributeValue = std::string(attribute->value()); else attributeValue = "";
											std::string::size_type jsonStartPos = attributeValue.find("$${");
											if(jsonStartPos != std::string::npos)
											{
												xmlData.jsonExists = true;
												attributeValue = attributeValue.substr(jsonStartPos + 2);
												std::string jsonString;
												BaseLib::Html::unescapeHtmlEntities(attributeValue, jsonString);
												try
												{
													BaseLib::PVariable json = jsonDecoder.decode(jsonString);
													element->description = json;
												}
												catch(const Exception& ex)
												{
													_bl->out.printError("Error decoding JSON of group variable \"" + element->groupVariableName + "\": " + ex.what());
													continue;
												}
											}

											xmlData.groupVariableXmlData.emplace(element);

											//{{{ Assign group variable to device
												auto variableIterator = deviceByGroupVariable.find(id);
												if(variableIterator == deviceByGroupVariable.end()) variableIterator = deviceByGroupVariable.find(shortId);
												if(variableIterator != deviceByGroupVariable.end())
												{
													for(auto& device : variableIterator->second)
													{
														auto infoIterator = device->variableInfo.find(id);
														if(infoIterator == device->variableInfo.end()) infoIterator = device->variableInfo.find(shortId);
														if(infoIterator != device->variableInfo.end())
														{
                                                            for(auto& variableInfoElement : infoIterator->second)
                                                            {
                                                                {
                                                                    //Delete old element if necessary

                                                                    auto deviceVariableElement = device->variables.find((uint32_t)variableInfoElement.index);
                                                                    if(deviceVariableElement != device->variables.end())
                                                                    {
                                                                        if(!deviceVariableElement->second->autocreated && !variableInfoElement.writeFlag) continue; //Ignore

                                                                        //If current element has write flag (= sending variable) delete old variable to be able to insert the sending variable.
                                                                        //Also delete an autocreated variable to overwrite it with the current one.
                                                                        device->variables.erase(deviceVariableElement);
                                                                    }
                                                                }

                                                                std::shared_ptr<GroupVariableXmlData> variableInfo = std::make_shared<GroupVariableXmlData>();
                                                                *variableInfo = *element;

                                                                variableInfo->readFlag = variableInfoElement.readFlag;
                                                                variableInfo->writeFlag = variableInfoElement.writeFlag;
                                                                variableInfo->transmitFlag = variableInfoElement.transmitFlag;
                                                                variableInfo->index = variableInfoElement.index;

                                                                device->variables.emplace(variableInfo->index, variableInfo);
                                                            }
														}
														else
                                                        {
                                                            std::shared_ptr<GroupVariableXmlData> variableInfo = std::make_shared<GroupVariableXmlData>();
                                                            *variableInfo = *element;
                                                            variableInfo->autocreated = true;
                                                            device->variables.emplace(variableInfo->index, variableInfo);
                                                        }
													}
												}
											//}}}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		catch(const std::exception& ex)
		{
			_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(const Exception& ex)
		{
			_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		doc.clear();
	}
	return xmlData;
}

PParameter Search::createParameter(PFunction& function, std::string name, std::string metadata, std::string unit, IPhysical::OperationType::Enum operationType, bool readable, bool writeable, uint16_t address, int32_t size, std::shared_ptr<ILogical> logical, bool noCast)
{
	try
	{
		PParameter parameter(new Parameter(_bl, function->variables.get()));
		parameter->id = std::move(name);
		parameter->metadata = metadata;
		parameter->unit = std::move(unit);
		parameter->readable = readable;
		parameter->writeable = writeable;
		if(logical) parameter->logical = logical;
		parameter->physical = std::make_shared<BaseLib::DeviceDescription::Physical>(_bl);
		parameter->physical->operationType = operationType;
		parameter->physical->address = address;
		parameter->physical->bitSize = size;
		if(!noCast)
		{
			ParameterCast::PGeneric cast(new ParameterCast::Generic(_bl));
			parameter->casts.push_back(cast);
			cast->type = metadata;
		}
		return parameter;
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return PParameter();
}

void Search::parseDatapointType(PFunction& function, std::string& datapointType, PParameter& parameter)
{
	try
	{
		std::vector<PParameter> additionalParameters;
		ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

		//1-bit
		if(datapointType == "DPT-1" || datapointType.compare(0, 7, "DPST-1-") == 0)
		{
			cast->type = "DPT-1";
			PLogicalBoolean logical(new LogicalBoolean(_bl));
			parameter->logical = logical;
		}
		//1-bit controlled
		else if(datapointType == "DPT-2" || datapointType.compare(0, 7, "DPST-2-") == 0)
		{
			cast->type = "DPT-2";
			PLogicalEnumeration logical(new LogicalEnumeration(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 3;
			logical->values.emplace_back("NoControlFalse", 0);
			logical->values.emplace_back("NoControlTrue", 1);
			logical->values.emplace_back("ControlFalse", 2);
			logical->values.emplace_back("ControlTrue", 3);

			std::string baseName = parameter->id;
			parameter->id = baseName + ".RAW";
			if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, (uint16_t)parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

			additionalParameters.push_back(createParameter(function, baseName + ".CONTROL", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			additionalParameters.push_back(createParameter(function, baseName + ".STATE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
		}
		//3-bit controlled
		else if(datapointType == "DPT-3" || datapointType.compare(0, 7, "DPST-3-") == 0)
		{
			cast->type = "DPT-3";
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 15;

			std::string baseName = parameter->id;
			parameter->id = baseName + ".RAW";
			if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, (uint16_t)parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

			additionalParameters.push_back(createParameter(function, baseName + ".CONTROL", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));

			PLogicalInteger stepCode(new LogicalInteger(_bl));
			stepCode->minimumValue = 0;
			stepCode->minimumValue = 7;
			additionalParameters.push_back(createParameter(function, baseName + ".STEP_CODE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 3, stepCode));
		}
		//character
		else if(datapointType == "DPT-4" || datapointType.compare(0, 7, "DPST-4-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			cast->type = datapointType;
			logical->minimumValue = 0;
			logical->maximumValue = 255;
			//ASCII
			if(datapointType == "DPST-4-1") logical->maximumValue = 127;
			//ISO-8859-1
			else if(datapointType == "DPST-4-2") {}
			else cast->type = "DPT-4";
		}
		//8-bit unsigned value
		else if(datapointType == "DPT-5" || datapointType.compare(0, 7, "DPST-5-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 255;
			cast->type = datapointType;
			// Percentage (0..100%)
			if(datapointType == "DPST-5-1")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 100;
				if(parameter->unit.empty()) parameter->unit = "%";
			}
			// Angle
			else if(datapointType == "DPST-5-3")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 360;
				if(parameter->unit.empty()) parameter->unit = "°";
			}
			// Percentage (0..255%)
			else if(datapointType == "DPST-5-4") { if(parameter->unit.empty()) parameter->unit = "%"; }
			// Tariff (0..255)
			else if(datapointType == "DPST-5-6")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 254; //Don't know if this is correct it says "0..255" but also "MaxInclusive=254"
			}
			// Counter pulses (0..255)
			else if(datapointType == "DPST-5-10") { if(parameter->unit.empty()) parameter->unit = "counter pulses"; }
			else cast->type = "DPT-5";
		}
		//8-bit signed value
		else if(datapointType == "DPT-6" || datapointType.compare(0, 7, "DPST-6-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = -128;
			logical->maximumValue = 127;
			cast->type = datapointType;
			// Percentage (-128..127%)
			if(datapointType == "DPST-6-1") { if(parameter->unit.empty()) parameter->unit = "%"; }
			else if(datapointType == "DPST-6-10") { if(parameter->unit.empty()) parameter->unit = "counter pulses"; }
			// Status with mode
			else if(datapointType == "DPST-6-20")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_A", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_B", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 1, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_C", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 2, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_D", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 3, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_E", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				PLogicalEnumeration enumeration(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".MODE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 3, enumeration));
				enumeration->minimumValue = 1;
				enumeration->maximumValue = 4;
				enumeration->values.emplace_back("Mode 0", 1);
				enumeration->values.emplace_back("Mode 1", 2);
				enumeration->values.emplace_back("Mode 2", 4);
			}
			else cast->type = "DPT-6";
		}
		//2-byte unsigned value
		else if(datapointType == "DPT-7" || datapointType.compare(0, 7, "DPST-7-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 65535;
			cast->type = datapointType;
			//Pulses
			if(datapointType == "DPST-7-1") { if(parameter->unit.empty()) parameter->unit = "pulses"; }
			//Time (ms)
			else if(datapointType == "DPST-7-2") { if(parameter->unit.empty()) parameter->unit = "ms"; }
			//Time (10 ms)
			else if(datapointType == "DPST-7-3") { if(parameter->unit.empty()) parameter->unit = "* 10 ms"; }
			//Time (100 ms)
			else if(datapointType == "DPST-7-4") { if(parameter->unit.empty()) parameter->unit = "* 100 ms)"; }
			//Time (s)
			else if(datapointType == "DPST-7-5") { if(parameter->unit.empty()) parameter->unit = "s"; }
			//Time (min)
			else if(datapointType == "DPST-7-6") { if(parameter->unit.empty()) parameter->unit = "min"; }
			//Time (h)
			else if(datapointType == "DPST-7-7") { if(parameter->unit.empty()) parameter->unit = "h"; }
			//Length (mm)
			else if(datapointType == "DPST-7-11") { if(parameter->unit.empty()) parameter->unit = "mm"; }
			//Current (mA)
			else if(datapointType == "DPST-7-12") { if(parameter->unit.empty()) parameter->unit = "mA"; }
			//Brightness (lux)
			else if(datapointType == "DPST-7-13") { if(parameter->unit.empty()) parameter->unit = "lux"; }
			else cast->type = "DPT-7";
		}
		//2-byte signed value
		else if(datapointType == "DPT-8" || datapointType.compare(0, 7, "DPST-8-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = -32768;
			logical->maximumValue = 32767;
			cast->type = datapointType;
			//Pulses difference
			if(datapointType == "DPST-8-1") { if(parameter->unit.empty()) parameter->unit = "pulses"; }
			//Time lag (ms)
			else if(datapointType == "DPST-8-2") { if(parameter->unit.empty()) parameter->unit = "ms"; }
			//Time lag (10 ms)
			else if(datapointType == "DPST-8-3") { if(parameter->unit.empty()) parameter->unit = "* 10 ms"; }
			//Time lag (100 ms)
			else if(datapointType == "DPST-8-4") { if(parameter->unit.empty()) parameter->unit = "* 100 ms)"; }
			//Time lag (s)
			else if(datapointType == "DPST-8-5") { if(parameter->unit.empty()) parameter->unit = "s"; }
			//Time lag (min)
			else if(datapointType == "DPST-8-6") { if(parameter->unit.empty()) parameter->unit = "min"; }
			//Time lag (h)
			else if(datapointType == "DPST-8-7") { if(parameter->unit.empty()) parameter->unit = "h"; }
			//Percentage difference (%)
			else if(datapointType == "DPST-8-10") { if(parameter->unit.empty()) parameter->unit = "%"; }
			//Rotation angle
			else if(datapointType == "DPST-8-11") { if(parameter->unit.empty()) parameter->unit = "°"; }
			else cast->type = "DPT-8";
		}
		//2-byte float value
		else if(datapointType == "DPT-9" || datapointType.compare(0, 7, "DPST-9-") == 0)
		{
			PLogicalDecimal logical(new LogicalDecimal(_bl));
			parameter->logical = logical;
			logical->minimumValue = -670760;
			logical->maximumValue = 670760;
			cast->type = datapointType;
			//Temperature (°C)
			if(datapointType == "DPST-9-1")
			{
				if(parameter->unit.empty()) parameter->unit = "°C";
				logical->minimumValue = -273;
			}
			//Temperature difference (K)
			else if(datapointType == "DPST-9-2") { if(parameter->unit.empty()) parameter->unit = "K"; }
			//Kelvin/hour (K/h)
			else if(datapointType == "DPST-9-3") { if(parameter->unit.empty()) parameter->unit = "K/h"; }
			//Lux (lux);
			else if(datapointType == "DPST-9-4")
			{
				if(parameter->unit.empty()) parameter->unit = "lux";
				logical->minimumValue = 0;
			}
			//Speed (m/s)
			else if(datapointType == "DPST-9-5")
			{
				if(parameter->unit.empty()) parameter->unit = "m/s";
				logical->minimumValue = 0;
			}
			//Pressure (Pa)
			else if(datapointType == "DPST-9-6")
			{
				if(parameter->unit.empty()) parameter->unit = "Pa";
				logical->minimumValue = 0;
			}
			//Humidity (%)
			else if(datapointType == "DPST-9-7")
			{
				if(parameter->unit.empty()) parameter->unit = "%";
				logical->minimumValue = 0;
			}
			//Parts/million (ppm) [air quality]
			else if(datapointType == "DPST-9-8")
			{
				if(parameter->unit.empty()) parameter->unit = "ppm";
				logical->minimumValue = 0;
			}
			//Air flow (m³/h)
			else if(datapointType == "DPST-9-9") { if(parameter->unit.empty()) parameter->unit = "m³/h"; }
			//Time (s)
			else if(datapointType == "DPST-9-10") { if(parameter->unit.empty()) parameter->unit = "s"; }
			//Time (ms)
			else if(datapointType == "DPST-9-11") { if(parameter->unit.empty()) parameter->unit = "ms"; }
			//Voltage (mV)
			else if(datapointType == "DPST-9-20") { if(parameter->unit.empty()) parameter->unit = "mV"; }
			//Current (mA)
			else if(datapointType == "DPST-9-21") { if(parameter->unit.empty()) parameter->unit = "mA"; }
			//Power density (W/m²)
			else if(datapointType == "DPST-9-22") { if(parameter->unit.empty()) parameter->unit = "W/m²"; }
			//Kelvin/percent (K/%)
			else if(datapointType == "DPST-9-23") { if(parameter->unit.empty()) parameter->unit = "K/%"; }
			//Power (kW)
			else if(datapointType == "DPST-9-24") { if(parameter->unit.empty()) parameter->unit = "kW"; }
			//Volume flow (l/h)
			else if(datapointType == "DPST-9-25") { if(parameter->unit.empty()) parameter->unit = "l/h"; }
			//Rain amount (l/m²)
			else if(datapointType == "DPST-9-26")
			{
				if(parameter->unit.empty()) parameter->unit = "l/m²";
				logical->minimumValue = -671088.64;
				logical->maximumValue = 670760.96;
			}
			//Temperature (°F)
			else if(datapointType == "DPST-9-27")
			{
				if(parameter->unit.empty()) parameter->unit = "°F";
				logical->minimumValue = -459.6;
				logical->maximumValue = 670760.96;
			}
			//Wind speed (km/h)
			else if(datapointType == "DPST-9-28")
			{
				if(parameter->unit.empty()) parameter->unit = "km/h";
				logical->minimumValue = 0;
				logical->maximumValue = 670760.96;
			}
			else cast->type = "DPT-9";
		}
		//3-byte time
		else if(datapointType == "DPT-10" || datapointType.compare(0, 8, "DPST-10-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 16777215;
			cast->type = "DPT-10";

			if(datapointType == "DPST-10-1")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalEnumeration weekDays(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".DAY", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 3, weekDays));
				weekDays->minimumValue = 0;
				weekDays->maximumValue = 7;
				weekDays->values.emplace_back("No day", 0);
				weekDays->values.emplace_back("Monday", 1);
				weekDays->values.emplace_back("Tuesday", 2);
				weekDays->values.emplace_back("Wednesday", 3);
				weekDays->values.emplace_back("Thursday", 4);
				weekDays->values.emplace_back("Friday", 5);
				weekDays->values.emplace_back("Saturday", 6);
				weekDays->values.emplace_back("Sunday", 7);

				PLogicalInteger hours(new LogicalInteger(_bl));
				hours->minimumValue = 0;
				hours->maximumValue = 23;
				additionalParameters.push_back(createParameter(function, baseName + ".HOURS", "DPT-5", "h", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 3, 5, hours));

				PLogicalInteger minutes(new LogicalInteger(_bl));
				hours->minimumValue = 0;
				hours->maximumValue = 59;
				additionalParameters.push_back(createParameter(function, baseName + ".MINUTES", "DPT-5", "min", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 10, 6, minutes));

				PLogicalInteger seconds(new LogicalInteger(_bl));
				hours->minimumValue = 0;
				hours->maximumValue = 59;
				additionalParameters.push_back(createParameter(function, baseName + ".SECONDS", "DPT-5", "s", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 18, 6, minutes));
			}
		}
		//3-byte date
		else if(datapointType == "DPT-11" || datapointType.compare(0, 7, "DPST-11") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 16777215;
			cast->type = "DPT-11";

			if(datapointType == "DPST-11-1")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger day(new LogicalInteger(_bl));
				day->minimumValue = 1;
				day->maximumValue = 31;
				additionalParameters.push_back(createParameter(function, baseName + ".DAY", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 3, 5, day));

				PLogicalInteger month(new LogicalInteger(_bl));
				month->minimumValue = 1;
				month->maximumValue = 12;
				additionalParameters.push_back(createParameter(function, baseName + ".MONTH", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 12, 4, month));

				PLogicalInteger year(new LogicalInteger(_bl));
				year->minimumValue = 0;
				year->maximumValue = 99;
				additionalParameters.push_back(createParameter(function, baseName + ".YEAR", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 17, 7, year));
			}
		}
		//4-byte unsigned value
		else if(datapointType == "DPT-12" || datapointType.compare(0, 8, "DPST-12-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = -2147483648;
			logical->maximumValue = 2147483647;
			//Counter pulses (unsigned)
			if(datapointType == "DPST-12-1") parameter->unit == "counter pulses";
			else cast->type = "DPT-12";
		}
		//4-byte signed value
		else if(datapointType == "DPT-13" || datapointType.compare(0, 8, "DPST-13-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = -2147483648;
			logical->maximumValue = 2147483647;
			//Counter pulses (signed)
			if(datapointType == "DPST-13-1") parameter->unit == "counter pulses";
			//Flow rate (m³/h)
			else if(datapointType == "DPST-13-2") parameter->unit == "m³/h";
			//Active energy (Wh)
			else if(datapointType == "DPST-13-10") parameter->unit == "Wh";
			//Apparent energy (VAh)
			else if(datapointType == "DPST-13-11") parameter->unit == "VAh";
			//Reactive energy (VARh)
			else if(datapointType == "DPST-13-12") parameter->unit == "VARh";
			//Active energy
			else if(datapointType == "DPST-13-13") parameter->unit == "kWh";
			//Apparent energy
			else if(datapointType == "DPST-13-14") parameter->unit == "kVAh";
			//Reactive energy
			else if(datapointType == "DPST-13-15") parameter->unit == "kVARh";
			//Time lag (s)
			else if(datapointType == "DPST-13-100") parameter->unit == "s";
			else cast->type = "DPT-12";
		}
		//4-byte float value
		else if(datapointType == "DPT-14" || datapointType.compare(0, 8, "DPST-14-") == 0)
		{
			PLogicalDecimal logical(new LogicalDecimal(_bl));
			parameter->logical = logical;
			//Acceleration
			if(datapointType == "DPST-14-0") parameter->unit == "m/s²";
			//Angular acceleration
			else if(datapointType == "DPST-14-1") parameter->unit == "rad/s²";
			//Activation energy
			else if(datapointType == "DPST-14-2") parameter->unit == "J/mol";
			//Radioactive activity
			else if(datapointType == "DPST-14-3") parameter->unit == "1/s";
			//Amount of substance
			else if(datapointType == "DPST-14-4") parameter->unit == "mol";
			//Angle (radiant)
			else if(datapointType == "DPST-14-6") parameter->unit == "rad";
			//Angle (degree)
			else if(datapointType == "DPST-14-7") parameter->unit == "°";
			//Angular momentum
			else if(datapointType == "DPST-14-8") parameter->unit == "Js";
			//Angular velocity
			else if(datapointType == "DPST-14-9") parameter->unit == "rad/s";
			//Area
			else if(datapointType == "DPST-14-10") parameter->unit == "m²";
			//Capacitance
			else if(datapointType == "DPST-14-11") parameter->unit == "F";
			//Flux density
			else if(datapointType == "DPST-14-12") parameter->unit == "C/m²";
			//Charge density
			else if(datapointType == "DPST-14-13") parameter->unit == "C/m³";
			//Compressibility
			else if(datapointType == "DPST-14-14") parameter->unit == "m²/N";
			//Conductance
			else if(datapointType == "DPST-14-15") parameter->unit == "S";
			//Conductivity
			else if(datapointType == "DPST-14-16") parameter->unit == "S/m";
			//Density
			else if(datapointType == "DPST-14-17") parameter->unit == "kg/m³";
			//Electric charge
			else if(datapointType == "DPST-14-18") parameter->unit == "C";
			//Electric current
			else if(datapointType == "DPST-14-19") parameter->unit == "A";
			//Electric current density
			else if(datapointType == "DPST-14-20") parameter->unit == "A/m²";
			//Electric dipole moment
			else if(datapointType == "DPST-14-21") parameter->unit == "Cm";
			//Electric displacement
			else if(datapointType == "DPST-14-22") parameter->unit == "C/m²";
			//Electric field strength
			else if(datapointType == "DPST-14-23") parameter->unit == "V/m";
			//Electric flux
			else if(datapointType == "DPST-14-24") parameter->unit == "C";
			//Electric flux density
			else if(datapointType == "DPST-14-25") parameter->unit == "C/m²";
			//Electric polarization
			else if(datapointType == "DPST-14-26") parameter->unit == "C/m²";
			//Electric potential
			else if(datapointType == "DPST-14-27") parameter->unit == "V";
			//Electric potential difference
			else if(datapointType == "DPST-14-28") parameter->unit == "V";
			//Electromagnetic moment
			else if(datapointType == "DPST-14-29") parameter->unit == "Am²";
			//Electromotive force
			else if(datapointType == "DPST-14-30") parameter->unit == "V";
			//Energy
			else if(datapointType == "DPST-14-31") parameter->unit == "J";
			//Force
			else if(datapointType == "DPST-14-32") parameter->unit == "N";
			//Frequency
			else if(datapointType == "DPST-14-33") parameter->unit == "Hz";
			//Angular frequency
			else if(datapointType == "DPST-14-34") parameter->unit == "rad/s";
			//Heat capacity
			else if(datapointType == "DPST-14-35") parameter->unit == "J/K";
			//Heat flow rate
			else if(datapointType == "DPST-14-36") parameter->unit == "W";
			//Heat quantity
			else if(datapointType == "DPST-14-37") parameter->unit == "J";
			//Impedance
			else if(datapointType == "DPST-14-38") parameter->unit == "Ω";
			//Length
			else if(datapointType == "DPST-14-39") parameter->unit == "m";
			//Light quantity
			else if(datapointType == "DPST-14-40") parameter->unit == "J";
			//Luminance
			else if(datapointType == "DPST-14-41") parameter->unit == "cd/m²";
			//Luminous flux
			else if(datapointType == "DPST-14-42") parameter->unit == "lm";
			//Luminous intensity
			else if(datapointType == "DPST-14-43") parameter->unit == "cd";
			//Magnetic field strength
			else if(datapointType == "DPST-14-44") parameter->unit == "A/m";
			//Magnetic flux
			else if(datapointType == "DPST-14-45") parameter->unit == "Wb";
			//Magnetic flux density
			else if(datapointType == "DPST-14-46") parameter->unit == "T";
			//Magnetic moment
			else if(datapointType == "DPST-14-47") parameter->unit == "Am²";
			//Magnetic polarization
			else if(datapointType == "DPST-14-48") parameter->unit == "T";
			//Magnetization
			else if(datapointType == "DPST-14-49") parameter->unit == "A/m";
			//Magnetomotive force
			else if(datapointType == "DPST-14-50") parameter->unit == "A";
			//Mass
			else if(datapointType == "DPST-14-51") parameter->unit == "kg";
			//Mass flux
			else if(datapointType == "DPST-14-52") parameter->unit == "kg/s";
			//Momentum
			else if(datapointType == "DPST-14-53") parameter->unit == "N/s";
			//Phase angle (radiant)
			else if(datapointType == "DPST-14-54") parameter->unit == "rad";
			//Phase angle (degrees)
			else if(datapointType == "DPST-14-55") parameter->unit == "°";
			//Power
			else if(datapointType == "DPST-14-56") parameter->unit == "W";
			//Power factor
			else if(datapointType == "DPST-14-57") parameter->unit == "cos Φ";
			//Pressure
			else if(datapointType == "DPST-14-58") parameter->unit == "Pa";
			//Reactance
			else if(datapointType == "DPST-14-59") parameter->unit == "Ω";
			//Resistance
			else if(datapointType == "DPST-14-60") parameter->unit == "Ω";
			//Resistivity
			else if(datapointType == "DPST-14-61") parameter->unit == "Ωm";
			//Self inductance
			else if(datapointType == "DPST-14-62") parameter->unit == "H";
			//Solid angle
			else if(datapointType == "DPST-14-63") parameter->unit == "sr";
			//Sound intensity
			else if(datapointType == "DPST-14-64") parameter->unit == "W/m²";
			//Speed
			else if(datapointType == "DPST-14-65") parameter->unit == "m/s";
			//Stress
			else if(datapointType == "DPST-14-66") parameter->unit == "Pa";
			//Surface tension
			else if(datapointType == "DPST-14-67") parameter->unit == "N/m";
			//Temperature
			else if(datapointType == "DPST-14-68") parameter->unit == "°C";
			//Temperature absolute
			else if(datapointType == "DPST-14-69") parameter->unit == "°C";
			//Temperature difference
			else if(datapointType == "DPST-14-70") parameter->unit == "K";
			//Thermal capacity
			else if(datapointType == "DPST-14-71") parameter->unit == "J/K";
			//Thermal conductivity
			else if(datapointType == "DPST-14-72") parameter->unit == "W/mK";
			//Thermoelectriy power
			else if(datapointType == "DPST-14-73") parameter->unit == "V/K";
			//Time (s)
			else if(datapointType == "DPST-14-74") parameter->unit == "s";
			//Torque
			else if(datapointType == "DPST-14-75") parameter->unit == "Nm";
			//Volume
			else if(datapointType == "DPST-14-76") parameter->unit == "m³";
			//Volume flux
			else if(datapointType == "DPST-14-77") parameter->unit == "m³/s";
			//Weight
			else if(datapointType == "DPST-14-78") parameter->unit == "N";
			//Work
			else if(datapointType == "DPST-14-79") parameter->unit == "J";
			else cast->type = "DPT-14";
		}
		//Entrance access (32bit)
		else if(datapointType == "DPT-15" || datapointType.compare(0, 8, "DPST-15-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			cast->type = "DPT-15";

			if(datapointType == "DPST-15-0")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger field1(new LogicalInteger(_bl));
				field1->minimumValue = 0;
				field1->maximumValue = 9;
				additionalParameters.push_back(createParameter(function, baseName + ".DATA1", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 4, field1));

				PLogicalInteger field2(new LogicalInteger(_bl));
				field2->minimumValue = 0;
				field2->maximumValue = 9;
				additionalParameters.push_back(createParameter(function, baseName + ".DATA2", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 4, field2));

				PLogicalInteger field3(new LogicalInteger(_bl));
				field3->minimumValue = 0;
				field3->maximumValue = 9;
				additionalParameters.push_back(createParameter(function, baseName + ".DATA3", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 4, field3));

				PLogicalInteger field4(new LogicalInteger(_bl));
				field4->minimumValue = 0;
				field4->maximumValue = 9;
				additionalParameters.push_back(createParameter(function, baseName + ".DATA4", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 12, 4, field4));

				PLogicalInteger field5(new LogicalInteger(_bl));
				field5->minimumValue = 0;
				field5->maximumValue = 9;
				additionalParameters.push_back(createParameter(function, baseName + ".DATA5", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 4, field5));

				PLogicalInteger field6(new LogicalInteger(_bl));
				field6->minimumValue = 0;
				field6->maximumValue = 9;
				additionalParameters.push_back(createParameter(function, baseName + ".DATA6", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 20, 4, field6));

				additionalParameters.push_back(createParameter(function, baseName + ".DETECTION_ERROR", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 24, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".PERMISSION", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 25, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".READ_DIRECTION", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 26, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".ENCRYPTION", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 27, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));

				PLogicalInteger codeIndex(new LogicalInteger(_bl));
				codeIndex->minimumValue = 0;
				codeIndex->maximumValue = 9;
				additionalParameters.push_back(createParameter(function, baseName + ".CODE_INDEX", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 28, 4, codeIndex));
			}
		}
		//character string
		else if(datapointType == "DPT-16" || datapointType.compare(0, 8, "DPST-16-") == 0)
		{
			PLogicalString logical(new LogicalString(_bl));
			parameter->logical = logical;
			logical->defaultValue = "-";
			//DPST-16-0 => ASCII (0 - 127), DPST-16-1 => ISO-8859-1 (0 - 255)
			if(datapointType == "DPST-16-0" || datapointType == "DPST-16-1") cast->type = datapointType;
			else cast->type = "DPT-16";
		}
		//Scene number
		else if(datapointType == "DPT-17" || datapointType.compare(0, 8, "DPST-17-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 63;
			cast->type = "DPT-17";
		}
		//Scene control
		else if(datapointType == "DPT-18" || datapointType.compare(0, 8, "DPST-18-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 255;
			cast->type = "DPT-18";
			if(datapointType == "DPST-18-1")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".LEARN", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));

				PLogicalInteger hours(new LogicalInteger(_bl));
				hours->minimumValue = 0;
				hours->maximumValue = 63;
				additionalParameters.push_back(createParameter(function, baseName + ".SCENE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 2, 6, hours));
			}
		}
		//Date time
		else if(datapointType == "DPT-19" || datapointType.compare(0, 8, "DPST-19-") == 0)
		{
#ifdef CCU2
			parameter->casts.clear();
			return;
#endif
			PLogicalInteger64 logical(new LogicalInteger64(_bl));
			parameter->logical = logical;
			cast->type = "DPT-19";
			if(datapointType == "DPST-19-1")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger year(new LogicalInteger(_bl));
				year->minimumValue = 0;
				year->maximumValue = 255;
				additionalParameters.push_back(createParameter(function, baseName + ".YEAR", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 8, year));

				PLogicalInteger month(new LogicalInteger(_bl));
				month->minimumValue = 1;
				month->maximumValue = 12;
				additionalParameters.push_back(createParameter(function, baseName + ".MONTH", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 12, 4, month));

				PLogicalInteger dayOfMonth(new LogicalInteger(_bl));
				dayOfMonth->minimumValue = 0;
				dayOfMonth->maximumValue = 31;
				additionalParameters.push_back(createParameter(function, baseName + ".DAY_OF_MONTH", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 19, 5, dayOfMonth));

				PLogicalInteger dayOfWeek(new LogicalInteger(_bl));
				dayOfWeek->minimumValue = 0;
				dayOfWeek->maximumValue = 7;
				additionalParameters.push_back(createParameter(function, baseName + ".DAY_OF_WEEK", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 24, 3, dayOfWeek));

				PLogicalInteger hours(new LogicalInteger(_bl));
				hours->minimumValue = 0;
				hours->maximumValue = 24;
				additionalParameters.push_back(createParameter(function, baseName + ".HOURS", "DPT-5", "h", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 27, 5, hours));

				PLogicalInteger minutes(new LogicalInteger(_bl));
				minutes->minimumValue = 0;
				minutes->maximumValue = 59;
				additionalParameters.push_back(createParameter(function, baseName + ".MINUTES", "DPT-5", "min", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 34, 6, minutes));

				PLogicalInteger seconds(new LogicalInteger(_bl));
				seconds->minimumValue = 0;
				seconds->maximumValue = 59;
				additionalParameters.push_back(createParameter(function, baseName + ".SECONDS", "DPT-5", "s", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 42, 6, seconds));

				additionalParameters.push_back(createParameter(function, baseName + ".FAULT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 48, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".WORKING_DAY", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 49, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".WD_FIELD_INVALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 50, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".YEAR_FIELD_INVALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 51, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".MONTH_AND_DOM_FIELDS_INVALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 52, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".DOW_FIELD_INVALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 53, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".TIME_FIELDS_INVALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 54, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".SUMMERTIME", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 55, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".EXT_SYNC_SIGNAL", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 56, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
		}
		//1-byte
		else if(datapointType == "DPT-20" || datapointType.compare(0, 8, "DPST-20-") == 0)
		{
			PLogicalEnumeration logical(new LogicalEnumeration(_bl));
			parameter->logical = logical;
			cast->type = datapointType;
			//SCLO mode
			if(datapointType == "DPST-20-1")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("Autonomous", 0);
				logical->values.emplace_back("Slave", 1);
				logical->values.emplace_back("Master", 2);
			}
			//Building mode
			else if(datapointType == "DPST-20-2")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("Building in use", 0);
				logical->values.emplace_back("Building not used", 1);
				logical->values.emplace_back("Building protection", 2);
			}
			//Occupied
			else if(datapointType == "DPST-20-3")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("Occupied", 0);
				logical->values.emplace_back("Stand by", 1);
				logical->values.emplace_back("Not occupied", 2);
			}
			//Priority
			else if(datapointType == "DPST-20-4")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.emplace_back("High", 0);
				logical->values.emplace_back("Medium", 1);
				logical->values.emplace_back("Low", 2);
				logical->values.emplace_back("Void", 3);
			}
			//Light application mode
			else if(datapointType == "DPST-20-5")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("Normal", 0);
				logical->values.emplace_back("Presence simulation", 1);
				logical->values.emplace_back("Night round", 2);
			}
			//Light application area
			else if(datapointType == "DPST-20-6")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 50;
				logical->values.emplace_back("No fault", 0);
				logical->values.emplace_back("System an functions of common interest", 1);
				logical->values.emplace_back("HVAC general FBs", 10);
				logical->values.emplace_back("HVAC hot water heating", 11);
				logical->values.emplace_back("HVAC direct electrical heating", 12);
				logical->values.emplace_back("HVAC terminal units", 13);
				logical->values.emplace_back("HVAC VAC", 14);
				logical->values.emplace_back("Lighting", 20);
				logical->values.emplace_back("Security", 30);
				logical->values.emplace_back("Load management", 40);
				logical->values.emplace_back("Shutters and blinds", 50);
			}
			//Alarm class
			else if(datapointType == "DPST-20-7")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 3;
				logical->values.emplace_back("Simple alarm", 1);
				logical->values.emplace_back("Basic alarm", 2);
				logical->values.emplace_back("Extended alarm", 3);
			}
			//PSU mode
			else if(datapointType == "DPST-20-8")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("Disabled (PSU/DPSU fixed off)", 0);
				logical->values.emplace_back("Enabled (PSU/DPSU fixed on)", 1);
				logical->values.emplace_back("Auto (PSU/DPSU automatic on/off)", 2);
			}
			//System error class
			else if(datapointType == "DPST-20-11")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 18;
				logical->values.emplace_back("No fault", 0);
				logical->values.emplace_back("General device fault (e.g. RAM, EEPROM, UI, watchdog, ...)", 1);
				logical->values.emplace_back("Communication fault", 2);
				logical->values.emplace_back("Configuration fault", 3);
				logical->values.emplace_back("Hardware fault", 4);
				logical->values.emplace_back("Software fault", 5);
				logical->values.emplace_back("Insufficient non volatile memory", 6);
				logical->values.emplace_back("Insufficient volatile memory", 7);
				logical->values.emplace_back("Memory allocation command with size 0 received", 8);
				logical->values.emplace_back("CRC error", 9);
				logical->values.emplace_back("Watchdog reset detected", 10);
				logical->values.emplace_back("Invalid opcode detected", 11);
				logical->values.emplace_back("General protection fault", 12);
				logical->values.emplace_back("Maximum table length exceeded", 13);
				logical->values.emplace_back("Undefined load command received", 14);
				logical->values.emplace_back("Group address table is not sorted", 15);
				logical->values.emplace_back("Invalid connection number (TSAP)", 16);
				logical->values.emplace_back("Invalid group object number (ASAP)", 17);
				logical->values.emplace_back("Group object type exceeds (PID_MAX_APDU_LENGTH - 2)", 18);
			}
			//HVAC error class
			else if(datapointType == "DPST-20-12")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 4;
				logical->values.emplace_back("No fault", 0);
				logical->values.emplace_back("Sensor fault", 1);
				logical->values.emplace_back("Process fault / controller fault", 2);
				logical->values.emplace_back("Actuator fault", 3);
				logical->values.emplace_back("Other fault", 4);
			}
			//Time delay
			else if(datapointType == "DPST-20-13")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 25;
				logical->values.emplace_back("Not active", 0);
				logical->values.emplace_back("1 s", 1);
				logical->values.emplace_back("2 s", 2);
				logical->values.emplace_back("3 s", 3);
				logical->values.emplace_back("5 s", 4);
				logical->values.emplace_back("10 s", 5);
				logical->values.emplace_back("15 s", 6);
				logical->values.emplace_back("20 s", 7);
				logical->values.emplace_back("30 s", 8);
				logical->values.emplace_back("45 s", 9);
				logical->values.emplace_back("1 min", 10);
				logical->values.emplace_back("1.25 min", 11);
				logical->values.emplace_back("1.5 min", 12);
				logical->values.emplace_back("2 min", 13);
				logical->values.emplace_back("2.5 min", 14);
				logical->values.emplace_back("3 min", 15);
				logical->values.emplace_back("5 min", 16);
				logical->values.emplace_back("15 min", 17);
				logical->values.emplace_back("20 min", 18);
				logical->values.emplace_back("30 min", 19);
				logical->values.emplace_back("1 h", 20);
				logical->values.emplace_back("2 h", 21);
				logical->values.emplace_back("3 h", 22);
				logical->values.emplace_back("5 h", 23);
				logical->values.emplace_back("12 h", 24);
				logical->values.emplace_back("24 h", 25);
			}
			//Wind force scale (0..12)
			else if(datapointType == "DPST-20-14")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 12;
				logical->values.emplace_back("Calm (no wind)", 0);
				logical->values.emplace_back("Light air", 1);
				logical->values.emplace_back("Light breeze", 2);
				logical->values.emplace_back("Gentle breeze", 3);
				logical->values.emplace_back("Moderate breeze", 4);
				logical->values.emplace_back("Fresh breeze", 5);
				logical->values.emplace_back("Strong breeze", 6);
				logical->values.emplace_back("Near gale / moderate gale", 7);
				logical->values.emplace_back("Fresh gale", 8);
				logical->values.emplace_back("Strong gale", 9);
				logical->values.emplace_back("Whole gale / storm", 10);
				logical->values.emplace_back("Violent storm", 11);
				logical->values.emplace_back("Hurricane", 12);
			}
			//Sensor mode
			else if(datapointType == "DPST-20-17")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 4;
				logical->values.emplace_back("Inactive", 0);
				logical->values.emplace_back("Digital input not inverted", 1);
				logical->values.emplace_back("Digital input inverted", 2);
				logical->values.emplace_back("Analog input -&gt; 0% to 100%", 3);
				logical->values.emplace_back("Temperature sensor input", 4);
			}
			//Actuator connect type
			else if(datapointType == "DPST-20-20")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 2;
				logical->values.emplace_back("Sensor connection", 1);
				logical->values.emplace_back("Controller connection", 2);
			}
			//Fuel type
			else if(datapointType == "DPST-20-100")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.emplace_back("Auto", 0);
				logical->values.emplace_back("Oil", 1);
				logical->values.emplace_back("Gas", 2);
				logical->values.emplace_back("Solid state fuel", 3);
			}
			//Burner type
			else if(datapointType == "DPST-20-101")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.emplace_back("Reserved", 0);
				logical->values.emplace_back("1 stage", 1);
				logical->values.emplace_back("2 stage", 2);
				logical->values.emplace_back("Modulating", 3);
			}
			//HVAC mode
			else if(datapointType == "DPST-20-102")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 4;
				logical->values.emplace_back("Auto", 0);
				logical->values.emplace_back("Comfort", 1);
				logical->values.emplace_back("Standby", 2);
				logical->values.emplace_back("Economy", 3);
				logical->values.emplace_back("Building protection", 4);
			}
			//DHW mode
			else if(datapointType == "DPST-20-103")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 4;
				logical->values.emplace_back("Auto", 0);
				logical->values.emplace_back("Legio protect", 1);
				logical->values.emplace_back("Normal", 2);
				logical->values.emplace_back("Reduced", 3);
				logical->values.emplace_back("Off / frost protect", 4);
			}
			//Load priority
			else if(datapointType == "DPST-20-104")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("None", 0);
				logical->values.emplace_back("Shift load priority", 1);
				logical->values.emplace_back("Absolut load priority", 2);
			}
			//HVAC control mode
			else if(datapointType == "DPST-20-105")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 20;
				logical->values.emplace_back("Auto", 0);
				logical->values.emplace_back("Heat", 1);
				logical->values.emplace_back("Morning warmup", 2);
				logical->values.emplace_back("Cool", 3);
				logical->values.emplace_back("Night purge", 4);
				logical->values.emplace_back("Precool", 5);
				logical->values.emplace_back("Off", 6);
				logical->values.emplace_back("Test", 7);
				logical->values.emplace_back("Emergency heat", 8);
				logical->values.emplace_back("Fan only", 9);
				logical->values.emplace_back("Free cool", 10);
				logical->values.emplace_back("Ice", 11);
				logical->values.emplace_back("Maximum heating mode", 12);
				logical->values.emplace_back("Economic heat/cool mode", 13);
				logical->values.emplace_back("Dehumidification", 14);
				logical->values.emplace_back("Calibration mode", 15);
				logical->values.emplace_back("Emergency cool mode", 16);
				logical->values.emplace_back("Emergency steam mode", 17);
				logical->values.emplace_back("NoDem", 20);
			}
			//HVAC emergency mode
			else if(datapointType == "DPST-20-106")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 5;
				logical->values.emplace_back("Normal", 0);
				logical->values.emplace_back("EmergPressure", 1);
				logical->values.emplace_back("EmergDepressure", 2);
				logical->values.emplace_back("EmergPurge", 3);
				logical->values.emplace_back("EmergShutdown", 4);
				logical->values.emplace_back("EmergFire", 5);
			}
			//Changeover mode
			else if(datapointType == "DPST-20-107")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("Auto", 0);
				logical->values.emplace_back("Cooling only", 1);
				logical->values.emplace_back("Heating only", 2);
			}
			//Valve mode
			else if(datapointType == "DPST-20-108")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 5;
				logical->values.emplace_back("Heat stage A for normal heating", 1);
				logical->values.emplace_back("Heat stage B for heating with two stages (A + B)", 2);
				logical->values.emplace_back("Cool stage A for normal cooling", 3);
				logical->values.emplace_back("Cool stage B for cooling with two stages (A + B)", 4);
				logical->values.emplace_back("Heat/Cool for changeover applications", 5);
			}
			//Damper mode
			else if(datapointType == "DPST-20-109")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 4;
				logical->values.emplace_back("Fresh air, e.g. for fancoils", 1);
				logical->values.emplace_back("Supply air, e.g. for VAV", 2);
				logical->values.emplace_back("Extract air, e.g. for VAV", 3);
				logical->values.emplace_back("Extract air, e.g. for VAV", 4);
			}
			//Heater mode
			else if(datapointType == "DPST-20-110")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 3;
				logical->values.emplace_back("Heat stage A on/off", 1);
				logical->values.emplace_back("Heat stage A proportional", 2);
				logical->values.emplace_back("Heat stage B proportional", 3);
			}
			//Fan mode
			else if(datapointType == "DPST-20-111")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("Nor running", 0);
				logical->values.emplace_back("Permanently running", 1);
				logical->values.emplace_back("Running in intervals", 2);
			}
			//Master/slave mode
			else if(datapointType == "DPST-20-112")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("Autonomous", 0);
				logical->values.emplace_back("Master", 1);
				logical->values.emplace_back("Slave", 2);
			}
			//Status room setpoint
			else if(datapointType == "DPST-20-113")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("Normal setpoint", 0);
				logical->values.emplace_back("Alternative setpoint", 1);
				logical->values.emplace_back("Building protection setpoint", 2);
			}
			//Metering device type
			else if(datapointType == "DPST-20-114")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 255;
				logical->values.emplace_back("Other device", 0);
				logical->values.emplace_back("Oil meter", 1);
				logical->values.emplace_back("Electricity meter", 2);
				logical->values.emplace_back("Gas device", 3);
				logical->values.emplace_back("Heat meter", 4);
				logical->values.emplace_back("Steam meter", 5);
				logical->values.emplace_back("Warm water meter", 6);
				logical->values.emplace_back("Water meter", 7);
				logical->values.emplace_back("Heat cost allocator", 8);
				logical->values.emplace_back("Reserved", 9);
				logical->values.emplace_back("Cooling load meter (outlet)", 10);
				logical->values.emplace_back("Cooling load meter (inlet)", 11);
				logical->values.emplace_back("Heat (inlet)", 12);
				logical->values.emplace_back("Heat and cool", 13);
				logical->values.emplace_back("Breaker (electricity)", 32);
				logical->values.emplace_back("Valve (gas or water)", 33);
				logical->values.emplace_back("Waste water meter", 40);
				logical->values.emplace_back("Garbage", 41);
				logical->values.emplace_back("Void device type", 255);
			}
			//ADA type
			else if(datapointType == "DPST-20-120")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 2;
				logical->values.emplace_back("Air damper", 1);
				logical->values.emplace_back("VAV", 2);
			}
			//Backup mode
			else if(datapointType == "DPST-20-121")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 1;
				logical->values.emplace_back("Backup value", 0);
				logical->values.emplace_back("Keep last state", 1);
			}
			//Start synchronization type
			else if(datapointType == "DPST-20-122")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("Position unchanged", 0);
				logical->values.emplace_back("Single close", 1);
				logical->values.emplace_back("Single open", 2);
			}
			//Behavior lock/unlock
			else if(datapointType == "DPST-20-600")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 6;
				logical->values.emplace_back("Off", 0);
				logical->values.emplace_back("On", 1);
				logical->values.emplace_back("No change", 2);
				logical->values.emplace_back("Value according to additional parameter", 3);
				logical->values.emplace_back("Memory function value", 4);
				logical->values.emplace_back("Updated value", 5);
				logical->values.emplace_back("Value before locking", 6);
			}
			//Behavior bus power up/down
			else if(datapointType == "DPST-20-601")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 4;
				logical->values.emplace_back("Off", 0);
				logical->values.emplace_back("On", 1);
				logical->values.emplace_back("No change", 2);
				logical->values.emplace_back("Value according to additional parameter", 3);
				logical->values.emplace_back("Last (value before bus power down)", 4);
			}
			//Dali fade time
			else if(datapointType == "DPST-20-602")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 15;
				logical->values.emplace_back("0 s (disabled)", 0);
				logical->values.emplace_back("0.7 s", 1);
				logical->values.emplace_back("1.0 s", 2);
				logical->values.emplace_back("1.4 s", 3);
				logical->values.emplace_back("2.0 s", 4);
				logical->values.emplace_back("2.8 s", 5);
				logical->values.emplace_back("4.0 s", 6);
				logical->values.emplace_back("5.7 s", 7);
				logical->values.emplace_back("8.0 s", 8);
				logical->values.emplace_back("11.3 s", 9);
				logical->values.emplace_back("16.0 s", 10);
				logical->values.emplace_back("22.6 s", 11);
				logical->values.emplace_back("32.0 s", 12);
				logical->values.emplace_back("45.3 s", 13);
				logical->values.emplace_back("64.0 s", 14);
				logical->values.emplace_back("90.5 s", 15);
			}
			//Blink mode
			else if(datapointType == "DPST-20-603")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("BlinkingDisabled", 0);
				logical->values.emplace_back("WithoutAcknowledge", 1);
				logical->values.emplace_back("BlinkingWithAcknowledge", 2);
			}
			//Light control mode
			else if(datapointType == "DPST-20-604")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 1;
				logical->values.emplace_back("Automatic light control", 0);
				logical->values.emplace_back("Manual light control", 1);
			}
			//PB switch mode
			else if(datapointType == "DPST-20-605")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 2;
				logical->values.emplace_back("One PB/binary input mode", 1);
				logical->values.emplace_back("Two PBs/binary inputs mode", 2);
			}
			//PB action mode
			else if(datapointType == "DPST-20-606")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.emplace_back("Inactive (no message sent)", 0);
				logical->values.emplace_back("SwitchOff message sent", 1);
				logical->values.emplace_back("SwitchOn message sent", 2);
				logical->values.emplace_back("Inverse value of InfoOnOff is sent", 3);
			}
			//PB dimm mode
			else if(datapointType == "DPST-20-607")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 4;
				logical->values.emplace_back("One PB/binary input; SwitchOnOff inverts on each transmission", 1);
				logical->values.emplace_back("One PB/binary input, On / DimUp message sent", 2);
				logical->values.emplace_back("One PB/binary input, Off / DimDown message sent", 3);
				logical->values.emplace_back("Two PBs/binary inputs mode", 4);
			}
			//Switch on mode
			else if(datapointType == "DPST-20-608")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("Last actual value", 0);
				logical->values.emplace_back("Value according to additional parameter", 1);
				logical->values.emplace_back("Last received absolute set value", 2);
			}
			//Load type
			else if(datapointType == "DPST-20-609")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("Automatic", 0);
				logical->values.emplace_back("Leading edge (inductive load)", 1);
				logical->values.emplace_back("Trailing edge (capacitive load)", 2);
			}
			//Load type detection
			else if(datapointType == "DPST-20-610")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.emplace_back("Undefined", 0);
				logical->values.emplace_back("Leading edge (inductive load)", 1);
				logical->values.emplace_back("Trailing edge (capacitive load)", 2);
				logical->values.emplace_back("Detection not possible or error", 3);
			}
			//SAB except behavior
			else if(datapointType == "DPST-20-801")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 4;
				logical->values.emplace_back("Up", 0);
				logical->values.emplace_back("Down", 1);
				logical->values.emplace_back("No change", 2);
				logical->values.emplace_back("Value according to additional parameter", 3);
				logical->values.emplace_back("Stop", 4);
			}
			//SAB behavior on lock/unlock
			else if(datapointType == "DPST-20-802")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 6;
				logical->values.emplace_back("Up", 0);
				logical->values.emplace_back("Down", 1);
				logical->values.emplace_back("No change", 2);
				logical->values.emplace_back("Value according to additional parameter", 3);
				logical->values.emplace_back("Stop", 4);
				logical->values.emplace_back("Updated value", 5);
				logical->values.emplace_back("Value before locking", 6);
			}
			//SSSB mode
			else if(datapointType == "DPST-20-803")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 4;
				logical->values.emplace_back("One push button/binary input", 1);
				logical->values.emplace_back("One push button/binary input, MoveUp / StepUp message sent", 2);
				logical->values.emplace_back("One push button/binary input, MoveDown / StepDown message sent", 3);
				logical->values.emplace_back("Two push buttons/binary inputs mode", 4);
			}
			//Blinds control mode
			else if(datapointType == "DPST-20-804")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 1;
				logical->values.emplace_back("Automatic control", 0);
				logical->values.emplace_back("Manual control", 1);
			}
			//Communication mode
			else if(datapointType == "DPST-20-1000")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 255;
				logical->values.emplace_back("Data link layer", 0);
				logical->values.emplace_back("Data link layer bus monitor", 1);
				logical->values.emplace_back("Data link layer raw frames", 2);
				logical->values.emplace_back("cEMI transport layer", 6);
				logical->values.emplace_back("No layer", 255);
			}
			else if(datapointType == "DPST-20-1001")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 7;
				logical->values.emplace_back("PL medium domain address", 1);
				logical->values.emplace_back("RF control octet and serial number or DoA", 2);
				logical->values.emplace_back("Bus monitor error flags", 3);
				logical->values.emplace_back("Relative timestamp", 4);
				logical->values.emplace_back("Time delay", 5);
				logical->values.emplace_back("Extended relative timestamp", 6);
				logical->values.emplace_back("BiBat information", 7);
			}
			else if(datapointType == "DPST-20-1002")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("Asynchronous", 0);
				logical->values.emplace_back("Asynchronous + BiBat master", 1);
				logical->values.emplace_back("Asynchronous + BiBat slave", 2);
			}
			else if(datapointType == "DPST-20-1003")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.emplace_back("No filtering, all supported received frames shall be passed to the cEMI client using L_Data.ind", 0);
				logical->values.emplace_back("Filtering by domain address", 1);
				logical->values.emplace_back("Filtering by KNX serial number table", 2);
				logical->values.emplace_back("Filtering by domain address and by serial number", 3);
			}
			else
			{
				auto logicalInteger = std::make_shared<LogicalInteger>(_bl);
				parameter->logical = logicalInteger;
                logicalInteger->minimumValue = 0;
                logicalInteger->maximumValue = 255;
				cast->type = "DPT-20";
			}
		}
		//8-bit set
		else if(datapointType == "DPT-21" || datapointType.compare(0, 8, "DPST-21-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 255;
			cast->type = "DPT-21";

			//General status
			if(datapointType == "DPST-21-1")
			{
				logical->maximumValue = 31;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".NO_ALARM_ACK", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 3, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".DP_IN_ALARM", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".DP_VALUE_OVERRIDDEN", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".DP_VALUE_CORRUPTED", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".DP_VALUE_OUT_OF_SERVICE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//Device control
			else if(datapointType == "DPST-21-2")
			{
				logical->maximumValue = 7;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".VERIFY_MODE_ON", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OWN_ADDRESS_DATAGRAM", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".USER_APPLICATION_STOPPED", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//Forcing signal
			else if(datapointType == "DPST-21-100")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".ROOM_H_MAX", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".ROOM_H_CONF", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 1, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".DHW_LEGIO", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 2, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".DHW_NORM", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 3, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OVERRUN", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OVERSUPPLY", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".PROTECTION", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".FORCE_REQUEST", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//Forcing signal cool
			else if(datapointType == "DPST-21-101")
			{
				logical->maximumValue = 1;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".FORCE_REQUEST", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//Room heating controller status
			else if(datapointType == "DPST-21-102")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".SUMMER_MODE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_STOP_OPTIM", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 1, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_START_OPTIM", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 2, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_MORNING_BOOST", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 3, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".TEMP_RETURN_LIMIT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".TEMP_FLOW_LIMIT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_ECO", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".FAULT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//Solar DHW controller status
			else if(datapointType == "DPST-21-103")
			{
				logical->maximumValue = 7;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".SOLAR_LOAD_SUFFICIENT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".SDHW_LOAD_ACTIVE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".FAULT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//Fuel type set
			else if(datapointType == "DPST-21-104")
			{
				logical->maximumValue = 7;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".SOLID_STATE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".GAS", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OIL", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//Room cooling controller status
			else if(datapointType == "DPST-21-105")
			{
				logical->maximumValue = 1;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".FAULT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//Ventilation controller status
			else if(datapointType == "DPST-21-106")
			{
				logical->maximumValue = 15;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".COOL", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".HEAT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".FAN_ACTIVE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".FAULT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//Lighting actuator error information
			else if(datapointType == "DPST-21-601")
			{
				logical->maximumValue = 127;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".OVERHEAT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 1, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".LAMP_FAILURE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 2, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".DEFECTIVE_LOAD", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 3, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".UNDERLOAD", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OVERCURRENT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".UNDERVOLTAGE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".LOAD_DETECTION_ERROR", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//RF communication mode info
			else if(datapointType == "DPST-21-1000")
			{
				logical->maximumValue = 7;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".BI_BAT_SLAVE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".BI_BAT_MASTER", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".ASYNCHRONOUS", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//cEMI server supported RF filtering modes
			else if(datapointType == "DPST-21-1001")
			{
				logical->maximumValue = 7;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".DOA", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".KNX_SN", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".DOA_AND_KNX_SN", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//Channel activation for 8 channels
			else if(datapointType == "DPST-21-1010")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".STATE_1", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_2", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 1, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_3", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 2, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_4", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 3, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_5", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_6", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_7", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_8", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
		}
		//16-bit set
		else if(datapointType == "DPT-22" || datapointType.compare(0, 8, "DPST-22-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 65535;
			cast->type = "DPT-22";

			//DHW controller status
			if(datapointType == "DPST-22-100")
			{
				logical->maximumValue = 255;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".TEMP_OPTIM_SHIFT_ACTIVE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".SOLAR_ENERGY_SUPPORT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 9, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".SOLAR_ENERGY_ONLY", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 10, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OTHER_ENERGY_SOURCE_ACTIVE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 11, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".DHW_PUSH_ACTIVE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 12, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".LEGIO_PROT_ACTIVE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 13, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".DHW_LOAD_ACTIVE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 14, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".FAULT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 15, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//RHCC status
			else if(datapointType == "DPST-22-101")
			{
				logical->maximumValue = 32767;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".OVERHEAT_ALARM", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 1, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".FROST_ALARM", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 2, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".DEW_POINT_STATUS", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 3, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".COOLING_DISABLED", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_PRE_COOL", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_ECO_C", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".HEAT_COOL_MODE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".HEATING_DISABLED", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_STOP_OPTIM", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 9, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_START_OPTIM", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 10, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_MORNING_BOOST_H", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 11, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".TEMP_FLOW_RETURN_LIMIT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 12, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".TEMP_FLOW_LIMIT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 13, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_ECO_H", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 14, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".FAULT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 15, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//Media
			else if(datapointType == "DPST-22-1000")
			{
				logical->maximumValue = 63;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".KNX_IP", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 10, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".RF", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 11, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".PL110", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 13, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".TP1", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 14, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
			//Channel activation for 16 channels
			else if(datapointType == "DPST-22-1010")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".STATE_1", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_2", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 1, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_3", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 2, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_4", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 3, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_5", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_6", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_7", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_8", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_9", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_10", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 9, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_11", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 10, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_12", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 11, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_13", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 12, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_14", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 13, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_15", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 14, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_16", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 15, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
		}
		else if(datapointType == "DPT-23" || datapointType.compare(0, 8, "DPST-23-") == 0)
		{
			PLogicalEnumeration logical(new LogicalEnumeration(_bl));
			parameter->logical = logical;
			cast->type = datapointType;
			//On/off action
			if(datapointType == "DPST-23-1")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.emplace_back("Off", 0);
				logical->values.emplace_back("On", 1);
				logical->values.emplace_back("Off/on", 2);
				logical->values.emplace_back("On/off", 3);
			}
			//Alarm reaction
			else if(datapointType == "DPST-23-2")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.emplace_back("No alarm is used", 0);
				logical->values.emplace_back("Alarm position is UP", 1);
				logical->values.emplace_back("Alarm position is DOWN", 2);
			}
			//Up/down action
			else if(datapointType == "DPST-23-3")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.emplace_back("Up", 0);
				logical->values.emplace_back("Down", 1);
				logical->values.emplace_back("UpDown", 2);
				logical->values.emplace_back("DownUp", 3);
			}
			//HVAC push button action
			else if(datapointType == "DPST-23-102")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.emplace_back("Comfort/Economy", 0);
				logical->values.emplace_back("Comfort/Nothing", 1);
				logical->values.emplace_back("Economy/Nothing", 2);
				logical->values.emplace_back("Building prot/Auto", 3);
			}
			else
			{
                auto logicalInteger = std::make_shared<LogicalInteger>(_bl);
				parameter->logical = logicalInteger;
                logicalInteger->minimumValue = 0;
                logicalInteger->maximumValue = 3;
				cast->type = "DPT-23";
			}
		}
		//2-nibble set
		else if(datapointType == "DPT-25" || datapointType.compare(0, 8, "DPST-25-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 255;
			cast->type = "DPT-25";

			if(datapointType == "DPST-25-1000")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger nibble1(new LogicalInteger(_bl));
				nibble1->minimumValue = 0;
				nibble1->maximumValue = 3;
				additionalParameters.push_back(createParameter(function, baseName + ".BUSY", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 4, nibble1));

				PLogicalInteger nibble2(new LogicalInteger(_bl));
				nibble2->minimumValue = 0;
				nibble2->maximumValue = 3;
				additionalParameters.push_back(createParameter(function, baseName + ".NAK", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 4, nibble2));
			}
		}
		//8-bit set
		else if(datapointType == "DPT-26" || datapointType.compare(0, 8, "DPST-26-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 255;
			cast->type = "DPT-26";

			if(datapointType == "DPST-26-1")
			{
				logical->maximumValue = 127;

				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".INACTIVE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 1, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));

				PLogicalInteger hours(new LogicalInteger(_bl));
				hours->minimumValue = 0;
				hours->maximumValue = 63;
				additionalParameters.push_back(createParameter(function, baseName + ".SCENE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 2, 6, hours));
			}
		}
		//32-bit set
		else if(datapointType == "DPT-27" || datapointType.compare(0, 8, "DPST-27-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			cast->type = "DPT-27";

			//Bit-combined info on/off
			if(datapointType == "DPST-27-1")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_16_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_15_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 1, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_14_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 2, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_13_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 3, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_12_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_11_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_10_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_9_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_8_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_7_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 9, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_6_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 10, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_5_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 11, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_4_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 12, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_3_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 13, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_2_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 14, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_1_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 15, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_16", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_15", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 17, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_14", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 18, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_13", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 19, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_12", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 20, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_11", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 21, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_10", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 22, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_9", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 23, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_8", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 24, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_7", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 25, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_6", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 26, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_5", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 27, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_4", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 28, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_3", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 29, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_2", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 30, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_1", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 31, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
		}
		//Electrical energy
		else if(datapointType == "DPT-29" || datapointType.compare(0, 8, "DPST-29-") == 0)
		{
#ifdef CCU2
			parameter->casts.clear();
			return;
#endif
			PLogicalInteger64 logical(new LogicalInteger64(_bl));
			parameter->logical = logical;
			cast->type = datapointType;
			//Active energy
			if(datapointType == "DPST-29-10") { if(parameter->unit.empty()) parameter->unit = "Wh"; }
			//Apparent energy
			else if(datapointType == "DPST-29-11") { if(parameter->unit.empty()) parameter->unit = "VAh"; }
			//Reactive energy
			else if(datapointType == "DPST-29-12") { if(parameter->unit.empty()) parameter->unit = "VARh"; }
			else cast->type = "DPT-29";
		}
		//24 channels activation
		else if(datapointType == "DPT-30" || datapointType.compare(0, 8, "DPST-30-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 16777215;
			cast->type = "DPT-30";

			//24 channels activation states
			if(datapointType == "DPST-30-1010")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".STATE_1", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_2", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 1, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_3", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 2, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_4", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 3, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_5", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_6", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_7", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_8", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_9", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_10", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 9, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_11", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 10, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_12", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 11, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_13", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 12, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_14", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 13, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_15", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 14, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_16", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 15, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_17", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_18", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 17, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_19", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 18, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_20", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 19, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_21", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 20, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_22", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 21, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_23", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 22, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".STATE_24", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 23, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
		}
		//16-bit unsigned value and 8-bit enumeration
		if(datapointType == "DPT-206" || datapointType.compare(0, 9, "DPST-206-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 16777215;
			cast->type = "DPT-206";

			//Time delay & HVAC mode
			if(datapointType == "DPST-206-100")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger delay(new LogicalInteger(_bl));
				delay->minimumValue = 0;
				delay->maximumValue = 65535;
				additionalParameters.push_back(createParameter(function, baseName + ".DELAY", "DPT-7", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 16, delay));

				PLogicalEnumeration hvacMode(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".HVAC_MODE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 8, hvacMode));
				hvacMode->minimumValue = 0;
				hvacMode->maximumValue = 4;
				hvacMode->values.emplace_back("Undefined", 0);
				hvacMode->values.emplace_back("Comfort", 1);
				hvacMode->values.emplace_back("Stand by", 2);
				hvacMode->values.emplace_back("Economy", 3);
				hvacMode->values.emplace_back("Building protection", 4);
			}
			//Time delay & DHW mode
			else if(datapointType == "DPST-206-102")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger delay(new LogicalInteger(_bl));
				delay->minimumValue = 0;
				delay->maximumValue = 65535;
				additionalParameters.push_back(createParameter(function, baseName + ".DELAY", "DPT-7", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 16, delay));

				PLogicalEnumeration dhwMode(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".DHW_MODE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 8, dhwMode));
				dhwMode->minimumValue = 0;
				dhwMode->maximumValue = 4;
				dhwMode->values.emplace_back("Undefined", 0);
				dhwMode->values.emplace_back("Legio protect", 1);
				dhwMode->values.emplace_back("Normal", 2);
				dhwMode->values.emplace_back("Reduced", 3);
				dhwMode->values.emplace_back("Off / frost protection", 4);
			}
			//Time delay & occupancy mode
			else if(datapointType == "DPST-206-104")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger delay(new LogicalInteger(_bl));
				delay->minimumValue = 0;
				delay->maximumValue = 65535;
				additionalParameters.push_back(createParameter(function, baseName + ".DELAY", "DPT-7", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 16, delay));

				PLogicalEnumeration occupancyMode(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".OCCUPANCY_MODE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 8, occupancyMode));
				occupancyMode->minimumValue = 0;
				occupancyMode->maximumValue = 2;
				occupancyMode->values.emplace_back("Occupied", 0);
				occupancyMode->values.emplace_back("Stand by", 1);
				occupancyMode->values.emplace_back("Not occupied", 2);
			}
			//Time delay & building mode
			else if(datapointType == "DPST-206-105")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger delay(new LogicalInteger(_bl));
				delay->minimumValue = 0;
				delay->maximumValue = 65535;
				additionalParameters.push_back(createParameter(function, baseName + ".DELAY", "DPT-7", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 16, delay));

				PLogicalEnumeration buildingMode(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".BUILDING_MODE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 8, buildingMode));
				buildingMode->minimumValue = 0;
				buildingMode->maximumValue = 2;
				buildingMode->values.emplace_back("In use", 0);
				buildingMode->values.emplace_back("Not in use", 1);
				buildingMode->values.emplace_back("Protected", 2);
			}
		}
		//Datapoint type version
		else if(datapointType == "DPT-217" || datapointType.compare(0, 9, "DPST-217-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 65535;
			cast->type = "DPT-217";

			if(datapointType == "DPST-217-1")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger magic(new LogicalInteger(_bl));
				magic->minimumValue = 0;
				magic->maximumValue = 31;
				additionalParameters.push_back(createParameter(function, baseName + ".MAGIC_NUMBER", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 5, magic));

				PLogicalInteger version(new LogicalInteger(_bl));
				version->minimumValue = 0;
				version->maximumValue = 31;
				additionalParameters.push_back(createParameter(function, baseName + ".VERSION_NUMBER", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 5, version));

				PLogicalInteger revision(new LogicalInteger(_bl));
				revision->minimumValue = 0;
				revision->maximumValue = 63;
				additionalParameters.push_back(createParameter(function, baseName + ".REVISION_NUMBER", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 10, 6, revision));
			}
		}
		//Alarm info
		else if(datapointType == "DPT-219" || datapointType.compare(0, 9, "DPST-219-") == 0)
		{
#ifdef CCU2
			parameter->casts.clear();
			return;
#endif
			PLogicalInteger64 logical(new LogicalInteger64(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 281474976710655;
			cast->type = "DPT-219";

			if(datapointType == "DPST-219-1")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger logNumber(new LogicalInteger(_bl));
				logNumber->minimumValue = 0;
				logNumber->maximumValue = 255;
				additionalParameters.push_back(createParameter(function, baseName + ".LOG_NUMBER", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 8, logNumber));

				PLogicalEnumeration alarmPriority(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".PRIORITY", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 8, alarmPriority));
				alarmPriority->minimumValue = 0;
				alarmPriority->maximumValue = 3;
				alarmPriority->values.emplace_back("High", 0);
				alarmPriority->values.emplace_back("Medium", 1);
				alarmPriority->values.emplace_back("Low", 2);
				alarmPriority->values.emplace_back("None", 3);

				PLogicalEnumeration applicationArea(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".APPLICATION_AREA", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 8, applicationArea));
				applicationArea->minimumValue = 0;
				applicationArea->maximumValue = 50;
				applicationArea->values.emplace_back("No fault", 0);
				applicationArea->values.emplace_back("Common interest", 1);
				applicationArea->values.emplace_back("HVAC general FB's", 10);
				applicationArea->values.emplace_back("HVAC hot water heating", 11);
				applicationArea->values.emplace_back("HVAC electrical heating", 12);
				applicationArea->values.emplace_back("HVAC terminal units", 13);
				applicationArea->values.emplace_back("HVAC VAC", 14);
				applicationArea->values.emplace_back("Lighting", 20);
				applicationArea->values.emplace_back("Security", 30);
				applicationArea->values.emplace_back("Shutters and blinds", 50);

				PLogicalEnumeration errorClass(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".ERROR_CLASS", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 24, 8, errorClass));
				errorClass->minimumValue = 0;
				errorClass->maximumValue = 5;
				errorClass->values.emplace_back("No fault", 0);
				errorClass->values.emplace_back("General device fault", 1);
				errorClass->values.emplace_back("Communication fault", 2);
				errorClass->values.emplace_back("Configuration fault", 3);
				errorClass->values.emplace_back("HW fault", 4);
				errorClass->values.emplace_back("SW fault", 5);

				additionalParameters.push_back(createParameter(function, baseName + ".ERROR_CODE_SUP", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 36, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".ALARM_TEXT_SUP", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 37, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".TIME_STAMP_SUP", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 38, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".ACK_SUP", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 39, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".LOCKED", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 45, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".ALARM_UN_ACK", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 46, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".IN_ALARM", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 47, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
		}
		//3x 2-byte float value
		else if(datapointType == "DPT-222" || datapointType.compare(0, 9, "DPST-222-") == 0)
		{
#ifdef CCU2
			parameter->casts.clear();
			return;
#endif
			PLogicalInteger64 logical(new LogicalInteger64(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 281474976710655;
			cast->type = "DPT-222";

			//Room temperature setpoint
			if(datapointType == "DPST-222-100")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalDecimal comfort(new LogicalDecimal(_bl));
				comfort->minimumValue = -273.0;
				comfort->maximumValue = 670760.0;
				additionalParameters.push_back(createParameter(function, baseName + ".COMFORT_TEMPERATURE", "DPT-9", "°C", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 16, comfort));

				PLogicalDecimal standBy(new LogicalDecimal(_bl));
				standBy->minimumValue = -273.0;
				standBy->maximumValue = 670760.0;
				additionalParameters.push_back(createParameter(function, baseName + ".STANDBY_TEMPERATURE", "DPT-9", "°C", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 16, standBy));

				PLogicalDecimal eco(new LogicalDecimal(_bl));
				eco->minimumValue = -273.0;
				eco->maximumValue = 670760.0;
				additionalParameters.push_back(createParameter(function, baseName + ".ECO_TEMPERATURE", "DPT-9", "°C", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 32, 16, eco));
			}
			//Room temperature setpoint shift
			else if(datapointType == "DPST-222-101")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalDecimal comfort(new LogicalDecimal(_bl));
				comfort->minimumValue = -670760.0;
				comfort->maximumValue = 670760.0;
				additionalParameters.push_back(createParameter(function, baseName + ".COMFORT_TEMPERATURE_SHIFT", "DPT-9", "°C", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 16, comfort));

				PLogicalDecimal standBy(new LogicalDecimal(_bl));
				standBy->minimumValue = -670760.0;
				standBy->maximumValue = 670760.0;
				additionalParameters.push_back(createParameter(function, baseName + ".STANDBY_TEMPERATURE_SHIFT", "DPT-9", "°C", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 16, standBy));

				PLogicalDecimal eco(new LogicalDecimal(_bl));
				eco->minimumValue = -670760.0;
				eco->maximumValue = 670760.0;
				additionalParameters.push_back(createParameter(function, baseName + ".ECO_TEMPERATURE_SHIFT", "DPT-9", "°C", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 32, 16, eco));
			}
		}
		//4-1-1 byte combined information
		else if(datapointType == "DPT-229" || datapointType.compare(0, 9, "DPST-229-") == 0)
		{
#ifdef CCU2
			parameter->casts.clear();
			return;
#endif
			PLogicalInteger64 logical(new LogicalInteger64(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 281474976710655;
			cast->type = "DPT-229";

			//Metering value (value, encoding, cmd)
			if(datapointType == "DPST-229-1")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger counter(new LogicalInteger(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".COUNTER", "DPT-12", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 32, counter));

				PLogicalEnumeration valueInformation(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".APPLICATION_AREA", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 32, 8, valueInformation));
				valueInformation->minimumValue = 0;
				valueInformation->maximumValue = 177;
				valueInformation->values.emplace_back("Energy, 0.001 Wh", 0);
				valueInformation->values.emplace_back("Energy, 0.01 Wh", 1);
				valueInformation->values.emplace_back("Energy, 0.1 Wh", 2);
				valueInformation->values.emplace_back("Energy, 1 Wh", 3);
				valueInformation->values.emplace_back("Energy, 10 Wh", 4);
				valueInformation->values.emplace_back("Energy, 100 Wh", 5);
				valueInformation->values.emplace_back("Energy, 1000 Wh", 6);
				valueInformation->values.emplace_back("Energy, 10000 Wh", 7);
				valueInformation->values.emplace_back("Energy, 0.001kJ", 8);
				valueInformation->values.emplace_back("Energy, 0.01 kJ", 9);
				valueInformation->values.emplace_back("Energy, 0.1 kJ", 10);
				valueInformation->values.emplace_back("Energy, 1 kJ", 11);
				valueInformation->values.emplace_back("Energy, 10 kJ", 12);
				valueInformation->values.emplace_back("Energy, 100 kJ", 13);
				valueInformation->values.emplace_back("Energy, 1000 kJ", 14);
				valueInformation->values.emplace_back("Energy, 10000 kJ", 15);
				valueInformation->values.emplace_back("Volume, 0.001 l", 16);
				valueInformation->values.emplace_back("Volume, 0.01 l", 17);
				valueInformation->values.emplace_back("Volume, 0.1 l", 18);
				valueInformation->values.emplace_back("Volume, 1 l", 19);
				valueInformation->values.emplace_back("Volume, 10 l", 20);
				valueInformation->values.emplace_back("Volume, 100 l", 21);
				valueInformation->values.emplace_back("Volume, 1000 l", 22);
				valueInformation->values.emplace_back("Volume, 10000 l", 23);
				valueInformation->values.emplace_back("Mass, 0.001 kg", 24);
				valueInformation->values.emplace_back("Mass, 0.01 kg", 25);
				valueInformation->values.emplace_back("Mass, 0.1 kg", 26);
				valueInformation->values.emplace_back("Mass, 1 kg", 27);
				valueInformation->values.emplace_back("Mass, 10 kg", 28);
				valueInformation->values.emplace_back("Mass, 100 kg", 29);
				valueInformation->values.emplace_back("Mass, 1000 kg", 30);
				valueInformation->values.emplace_back("Mass, 10000 kg", 31);

				valueInformation->values.emplace_back("Power, 0.001 W", 40);
				valueInformation->values.emplace_back("Power, 0.01 W", 41);
				valueInformation->values.emplace_back("Power, 0.1 W", 42);
				valueInformation->values.emplace_back("Power, 1 W", 43);
				valueInformation->values.emplace_back("Power, 10 W", 44);
				valueInformation->values.emplace_back("Power, 100 W", 45);
				valueInformation->values.emplace_back("Power, 1000 W", 46);
				valueInformation->values.emplace_back("Power, 10000 W", 47);
				valueInformation->values.emplace_back("Power, 0.001 kJ/h", 48);
				valueInformation->values.emplace_back("Power, 0.01 kJ/h", 49);
				valueInformation->values.emplace_back("Power, 0.1 kJ/h", 50);
				valueInformation->values.emplace_back("Power, 1 kJ/h", 51);
				valueInformation->values.emplace_back("Power, 10 kJ/h", 52);
				valueInformation->values.emplace_back("Power, 100 kJ/h", 53);
				valueInformation->values.emplace_back("Power, 1000 kJ/h", 54);
				valueInformation->values.emplace_back("Power, 10000 kJ/h", 55);
				valueInformation->values.emplace_back("Volume flow, 0.001 l/h", 56);
				valueInformation->values.emplace_back("Volume flow, 0.01 l/h", 57);
				valueInformation->values.emplace_back("Volume flow, 0.1 l/h", 58);
				valueInformation->values.emplace_back("Volume flow, 1 l/h", 59);
				valueInformation->values.emplace_back("Volume flow, 10 l/h", 60);
				valueInformation->values.emplace_back("Volume flow, 100 l/h", 61);
				valueInformation->values.emplace_back("Volume flow, 1000 l/h", 62);
				valueInformation->values.emplace_back("Volume flow, 10000 l/h", 63);
				valueInformation->values.emplace_back("Volume flow, 0.0001 l/min", 64);
				valueInformation->values.emplace_back("Volume flow, 0.001 l/min", 65);
				valueInformation->values.emplace_back("Volume flow, 0.01 l/min", 66);
				valueInformation->values.emplace_back("Volume flow, 0.1 l/min", 67);
				valueInformation->values.emplace_back("Volume flow, 1 l/min", 68);
				valueInformation->values.emplace_back("Volume flow, 10 l/min", 69);
				valueInformation->values.emplace_back("Volume flow, 100 l/min", 70);
				valueInformation->values.emplace_back("Volume flow, 1000 l/min", 71);
				valueInformation->values.emplace_back("Volume flow, 0.001 ml/s", 72);
				valueInformation->values.emplace_back("Volume flow, 0.01 ml/s", 73);
				valueInformation->values.emplace_back("Volume flow, 0.1 ml/s", 74);
				valueInformation->values.emplace_back("Volume flow, 1 ml/s", 75);
				valueInformation->values.emplace_back("Volume flow, 10 ml/s", 76);
				valueInformation->values.emplace_back("Volume flow, 100 ml/s", 77);
				valueInformation->values.emplace_back("Volume flow, 1000 ml/s", 78);
				valueInformation->values.emplace_back("Volume flow, 10000 ml/s", 79);
				valueInformation->values.emplace_back("Mass flow, 0.001 kg/h", 80);
				valueInformation->values.emplace_back("Mass flow, 0.01 kg/h", 81);
				valueInformation->values.emplace_back("Mass flow, 0.1 kg/h", 82);
				valueInformation->values.emplace_back("Mass flow, 1 kg/h", 83);
				valueInformation->values.emplace_back("Mass flow, 10 kg/h", 84);
				valueInformation->values.emplace_back("Mass flow, 100 kg/h", 85);
				valueInformation->values.emplace_back("Mass flow, 1000 kg/h", 86);
				valueInformation->values.emplace_back("Mass flow, 10000 kg/h", 87);

				valueInformation->values.emplace_back("Units for HCA", 110);

				valueInformation->values.emplace_back("Energy, 0.1 MWh", 128);
				valueInformation->values.emplace_back("Energy, 1 MWh", 129);

				valueInformation->values.emplace_back("Energy, 0.1 GJ", 136);
				valueInformation->values.emplace_back("Energy, 1 GJ", 137);

				valueInformation->values.emplace_back("Power, 0.1 MW", 168);
				valueInformation->values.emplace_back("Power, 1 MW", 169);

				valueInformation->values.emplace_back("Power, 0.1 GJ/h", 176);
				valueInformation->values.emplace_back("Power, 1 GJ/h", 177);

				additionalParameters.push_back(createParameter(function, baseName + ".ALARM_UN_ACK", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 43, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".IN_ALARM", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 44, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OVERRIDDEN", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 45, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".FAULT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 46, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OUT_OF_SERVICE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 47, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
		}
		//MBus address
		else if(datapointType == "DPT-230" || datapointType.compare(0, 9, "DPST-230-") == 0)
		{
#ifdef CCU2
			parameter->casts.clear();
			return;
#endif
			PLogicalInteger64 logical(new LogicalInteger64(_bl));
			parameter->logical = logical;
			cast->type = "DPT-230";

			if(datapointType == "DPST-230-1000")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger manufacturerId(new LogicalInteger(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".MANUFACTURER_ID", "DPT-7", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 16, manufacturerId));

				PLogicalInteger identNumber(new LogicalInteger(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".IDENT_NUMBER", "DPT-12", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 32, identNumber));

				PLogicalInteger version(new LogicalInteger(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".IDENT_NUMBER", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 48, 8, version));

				PLogicalEnumeration medium(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".MEDIUM", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 56, 8, medium));
				medium->minimumValue = 0;
				medium->maximumValue = 55;
				medium->values.emplace_back("Other", 0);
				medium->values.emplace_back("Oil meter", 1);
				medium->values.emplace_back("Electricity meter", 2);
				medium->values.emplace_back("Gas meter", 3);
				medium->values.emplace_back("Heat meter", 4);
				medium->values.emplace_back("Steam meter", 5);
				medium->values.emplace_back("Warm water meter", 6);
				medium->values.emplace_back("Water meter", 7);
				medium->values.emplace_back("Heat cost allocator", 8);
				medium->values.emplace_back("Compressed air", 9);
				medium->values.emplace_back("Cooling load meter (inlet)", 10);
				medium->values.emplace_back("Cooling load meter (outlet)", 11);
				medium->values.emplace_back("Heat (inlet)", 12);
				medium->values.emplace_back("Heat and cool", 13);
				medium->values.emplace_back("Bus/system", 14);
				medium->values.emplace_back("Unknown device type", 15);

				medium->values.emplace_back("Breaker (electricity)", 32);
				medium->values.emplace_back("Valve (gas or water)", 33);

				medium->values.emplace_back("Waste water meter", 40);
				medium->values.emplace_back("Garbage", 41);

				medium->values.emplace_back("Radio converter", 55);
			}
		}
		//3-byte color RGB
		else if(datapointType == "DPT-232" || datapointType.compare(0, 9, "DPST-232-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 16777215;
			cast->type = "DPT-232";

			if(datapointType == "DPST-232-600")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger red(new LogicalInteger(_bl));
				red->minimumValue = 0;
				red->maximumValue = 255;
				additionalParameters.push_back(createParameter(function, baseName + ".RED", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 8, red));

				PLogicalInteger green(new LogicalInteger(_bl));
				green->minimumValue = 0;
				green->maximumValue = 255;
				additionalParameters.push_back(createParameter(function, baseName + ".GREEN", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 8, green));

				PLogicalInteger blue(new LogicalInteger(_bl));
				blue->minimumValue = 0;
				blue->maximumValue = 255;
				additionalParameters.push_back(createParameter(function, baseName + ".BLUE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 8, blue));
			}
		}
		//ISO 639-1 language code
		else if(datapointType == "DPT-234" || datapointType.compare(0, 9, "DPST-234-") == 0)
		{
			PLogicalString logical(new LogicalString(_bl));
			parameter->logical = logical;
			cast->type = "DPT-234";
		}
		//Configuration / diagnostics
		else if(datapointType == "DPT-237" || datapointType.compare(0, 9, "DPST-237-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 2047;
			cast->type = "DPT-237";

			//DPT DALI control gear diagnostic
			if(datapointType == "DPST-237-600")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".CONVERTER_ERROR", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".BALLAST_FAILURE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".LAMP_FAILURE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".READ", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".GROUP_ADDRESS", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 9, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));

				PLogicalInteger red(new LogicalInteger(_bl));
				red->minimumValue = 0;
				red->maximumValue = 63;
				additionalParameters.push_back(createParameter(function, baseName + ".DALI_ADDRESS", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 10, 6, red));
			}
		}
		//Configuration / diagnostics
		else if(datapointType == "DPT-238" || datapointType.compare(0, 9, "DPST-238-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 255;
			cast->type = "DPT-238";

			//DPT DALI diagnostics
			if(datapointType == "DPST-238-600")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".BALLAST_FAILURE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".LAMP_FAILURE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 1, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));

				PLogicalInteger red(new LogicalInteger(_bl));
				red->minimumValue = 0;
				red->maximumValue = 63;
				additionalParameters.push_back(createParameter(function, baseName + ".DEVICE_ADDRESS", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 2, 6, red));
			}
		}
		//Combined position
		else if(datapointType == "DPT-240" || datapointType.compare(0, 9, "DPST-240-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 16777215;
			cast->type = "DPT-240";

			if(datapointType == "DPST-240-800")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger height(new LogicalInteger(_bl));
				height->minimumValue = 0;
				height->maximumValue = 100;
				additionalParameters.push_back(createParameter(function, baseName + ".HEIGHT_POSITION", "DPST-5-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 8, height));

				PLogicalInteger slats(new LogicalInteger(_bl));
				slats->minimumValue = 0;
				slats->maximumValue = 100;
				additionalParameters.push_back(createParameter(function, baseName + ".SLATS_POSITION", "DPST-5-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 8, slats));

				additionalParameters.push_back(createParameter(function, baseName + ".HEIGHT_POS_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 22, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".SLATS_POS_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 23, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
		}
		//Sun blind and shutter actuator status
		else if(datapointType == "DPT-241" || datapointType.compare(0, 9, "DPST-241-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			cast->type = "DPT-241";

			if(datapointType == "DPST-241-800")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger height(new LogicalInteger(_bl));
				height->minimumValue = 0;
				height->maximumValue = 100;
				additionalParameters.push_back(createParameter(function, baseName + ".HEIGHT_POSITION", "DPST-5-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 8, height));

				PLogicalInteger slats(new LogicalInteger(_bl));
				slats->minimumValue = 0;
				slats->maximumValue = 100;
				additionalParameters.push_back(createParameter(function, baseName + ".SLATS_POSITION", "DPST-5-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 8, slats));

				additionalParameters.push_back(createParameter(function, baseName + ".UPPER_END_REACHED", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".LOWER_END_REACHED", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 17, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".LOWER_PREDEF_REACHED", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 18, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".TARGET_REACHED", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 19, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".CANT_REACH_TARGET_POS", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 20, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".CANT_REACH_SLATS_POS", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 21, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".IN_ALARM", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 22, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".UP_DOWN_FORCED_INPUT", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 23, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".LOCKED", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 24, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".OVERRIDDEN", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 25, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".FAILURE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 26, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".HEIGHT_POS_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 30, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".SLATS_POS_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 13, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
		}
		//DALI converter status
		else if(datapointType == "DPT-244" || datapointType.compare(0, 9, "DPST-244-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			cast->type = "DPT-244";

			if(datapointType == "DPST-244-600")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalEnumeration converterMode(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".CONVERTER_MODE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 4, converterMode));
				converterMode->minimumValue = 0;
				converterMode->maximumValue = 9;
				converterMode->values.emplace_back("Unknown", 0);
				converterMode->values.emplace_back("Normal mode active, all OK", 1);
				converterMode->values.emplace_back("Inhibit mode active", 2);
				converterMode->values.emplace_back("Hardwired inhibit mode active", 3);
				converterMode->values.emplace_back("Rest mode active", 4);
				converterMode->values.emplace_back("Emergency mode active", 5);
				converterMode->values.emplace_back("Extended emergency mode active", 6);
				converterMode->values.emplace_back("FT in progress", 7);
				converterMode->values.emplace_back("DT in progress", 8);
				converterMode->values.emplace_back("PDT in progress", 9);

				additionalParameters.push_back(createParameter(function, baseName + ".HARDWIRED_SWITCH_ACTIVE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".HARDWIRED_INHIBIT_ACTIVE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));

				PLogicalEnumeration functionTestPending(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".FUNCTION_TEST_PENDING", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 2, functionTestPending));
				functionTestPending->minimumValue = 0;
				functionTestPending->maximumValue = 2;
				functionTestPending->values.emplace_back("Unknown", 0);
				functionTestPending->values.emplace_back("No test pending", 1);
				functionTestPending->values.emplace_back("Test pending", 2);

				PLogicalEnumeration durationTestPending(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".DURATION_TEST_PENDING", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 10, 2, durationTestPending));
				durationTestPending->minimumValue = 0;
				durationTestPending->maximumValue = 2;
				durationTestPending->values.emplace_back("Unknown", 0);
				durationTestPending->values.emplace_back("No test pending", 1);
				durationTestPending->values.emplace_back("Test pending", 2);

				PLogicalEnumeration partialDurationTestPending(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".PARTIAL_DURATION_TEST_PENDING", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 12, 2, partialDurationTestPending));
				partialDurationTestPending->minimumValue = 0;
				partialDurationTestPending->maximumValue = 2;
				partialDurationTestPending->values.emplace_back("Unknown", 0);
				partialDurationTestPending->values.emplace_back("No test pending", 1);
				partialDurationTestPending->values.emplace_back("Test pending", 2);

				PLogicalEnumeration converterFailure(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".CONVERTER_FAILURE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 14, 2, converterFailure));
				converterFailure->minimumValue = 0;
				converterFailure->maximumValue = 2;
				converterFailure->values.emplace_back("Unknown", 0);
				converterFailure->values.emplace_back("No failure detected", 1);
				converterFailure->values.emplace_back("Failure detected", 2);
			}
		}
		//DALI converter test result
		else if(datapointType == "DPT-245" || datapointType.compare(0, 9, "DPST-245-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			cast->type = "DPT-245";

			if(datapointType == "DPST-245-600")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalEnumeration ltrf(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".LTRF", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 4, ltrf));
				ltrf->minimumValue = 0;
				ltrf->maximumValue = 5;
				ltrf->values.emplace_back("Unknown", 0);
				ltrf->values.emplace_back("Passed in time", 1);
				ltrf->values.emplace_back("Passed max delay exceeded", 2);
				ltrf->values.emplace_back("Failed, test executed in time", 3);
				ltrf->values.emplace_back("Failed, max delay exceeded", 4);
				ltrf->values.emplace_back("Test manually stopped", 5);

				PLogicalEnumeration ltrd(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".LTRF", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 4, ltrd));
				ltrd->minimumValue = 0;
				ltrd->maximumValue = 5;
				ltrd->values.emplace_back("Unknown", 0);
				ltrd->values.emplace_back("Passed in time", 1);
				ltrd->values.emplace_back("Passed max delay exceeded", 2);
				ltrd->values.emplace_back("Failed, test executed in time", 3);
				ltrd->values.emplace_back("Failed, max delay exceeded", 4);
				ltrd->values.emplace_back("Test manually stopped", 5);

				PLogicalEnumeration ltrp(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".LTRF", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 4, ltrp));
				ltrp->minimumValue = 0;
				ltrp->maximumValue = 5;
				ltrp->values.emplace_back("Unknown", 0);
				ltrp->values.emplace_back("Passed in time", 1);
				ltrp->values.emplace_back("Passed max delay exceeded", 2);
				ltrp->values.emplace_back("Failed, test executed in time", 3);
				ltrp->values.emplace_back("Failed, max delay exceeded", 4);
				ltrp->values.emplace_back("Test manually stopped", 5);

				PLogicalEnumeration sf(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".LTRF", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 2, sf));
				sf->minimumValue = 0;
				sf->maximumValue = 2;
				sf->values.emplace_back("Unknown", 0);
				sf->values.emplace_back("Started automatically", 1);
				sf->values.emplace_back("Started by gateway", 2);

				PLogicalEnumeration sd(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".LTRF", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 18, 2, sd));
				sd->minimumValue = 0;
				sd->maximumValue = 2;
				sd->values.emplace_back("Unknown", 0);
				sd->values.emplace_back("Started automatically", 1);
				sd->values.emplace_back("Started by gateway", 2);

				PLogicalEnumeration sp(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".LTRF", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 20, 2, sp));
				sp->minimumValue = 0;
				sp->maximumValue = 2;
				sp->values.emplace_back("Unknown", 0);
				sp->values.emplace_back("Started automatically", 1);
				sp->values.emplace_back("Started by gateway", 2);

				PLogicalInteger ldtr(new LogicalInteger(_bl));
				ldtr->minimumValue = 0;
				ldtr->maximumValue = 510;
				additionalParameters.push_back(createParameter(function, baseName + ".LDTR", "DPT-7", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 24, 16, ldtr));

				PLogicalInteger lpdtr(new LogicalInteger(_bl));
				lpdtr->minimumValue = 0;
				lpdtr->maximumValue = 255;
				additionalParameters.push_back(createParameter(function, baseName + ".LPDTR", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 40, 8, lpdtr));
			}
		}
		//Brightness color temperature transition
		else if(datapointType == "DPT-249" || datapointType.compare(0, 9, "DPST-249-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			cast->type = "DPT-249";

			if(datapointType == "DPST-249-600")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger duration(new LogicalInteger(_bl));
				duration->minimumValue = 0;
				duration->maximumValue = 65535;
				additionalParameters.push_back(createParameter(function, baseName + ".DURATION", "DPST-7-4", "100 ms", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 0, 16, duration));

				PLogicalInteger temperature(new LogicalInteger(_bl));
				temperature->minimumValue = 0;
				temperature->maximumValue = 65535;
				additionalParameters.push_back(createParameter(function, baseName + ".TEMPERATURE", "DPT-7", "K", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 16, temperature));

				PLogicalInteger brightness(new LogicalInteger(_bl));
				brightness->minimumValue = 0;
				brightness->maximumValue = 100;
				additionalParameters.push_back(createParameter(function, baseName + ".BRIGHTNESS", "DPST-5-1", "%", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 32, 8, brightness));

				additionalParameters.push_back(createParameter(function, baseName + ".DURATION_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 45, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".TEMPERATURE_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 46, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".BRIGHTNESS_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 47, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
		}
		//Brightness color temperature control
		else if(datapointType == "DPT-250" || datapointType.compare(0, 9, "DPST-250-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			cast->type = "DPT-250";

			if(datapointType == "DPST-250-600")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				additionalParameters.push_back(createParameter(function, baseName + ".CCT_INCREASE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));

				PLogicalInteger cctStep(new LogicalInteger(_bl));
				cctStep->minimumValue = 1;
				cctStep->maximumValue = 7;
				additionalParameters.push_back(createParameter(function, baseName + ".CCT_STEP", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 5, 3, cctStep));

				additionalParameters.push_back(createParameter(function, baseName + ".CB_INCREASE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 12, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));

				PLogicalInteger cbStep(new LogicalInteger(_bl));
				cbStep->minimumValue = 1;
				cbStep->maximumValue = 7;
				additionalParameters.push_back(createParameter(function, baseName + ".CB_STEP", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 13, 3, cbStep));

				additionalParameters.push_back(createParameter(function, baseName + ".CCT_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 22, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".CB_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 23, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
			}
		}
		//RGBW
		else if(datapointType == "DPT-251" || datapointType.compare(0, 9, "DPST-251-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			cast->type = "DPT-251";

			if(datapointType == "DPST-251-600")
			{
				std::string baseName = parameter->id;
				parameter->id = baseName + ".RAW";
				if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, (uint16_t)parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(_bl)));

				PLogicalInteger red(new LogicalInteger(_bl));
				red->minimumValue = 0;
				red->maximumValue = 255;
				additionalParameters.push_back(createParameter(function, baseName + ".RED", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 8, 8, red));

				PLogicalInteger green(new LogicalInteger(_bl));
				green->minimumValue = 0;
				green->maximumValue = 255;
				additionalParameters.push_back(createParameter(function, baseName + ".RED", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 16, 8, green));

				PLogicalInteger blue(new LogicalInteger(_bl));
				blue->minimumValue = 0;
				blue->maximumValue = 255;
				additionalParameters.push_back(createParameter(function, baseName + ".RED", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 24, 8, blue));

				PLogicalInteger white(new LogicalInteger(_bl));
				white->minimumValue = 0;
				white->maximumValue = 255;
				additionalParameters.push_back(createParameter(function, baseName + ".RED", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 32, 8, white));

				additionalParameters.push_back(createParameter(function, baseName + ".RED_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 44, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".GREEN_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 45, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".BLUE_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 46, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));
				additionalParameters.push_back(createParameter(function, baseName + ".WHITE_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, 47, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl)));

			}
		}

		for(auto& additionalParameter : additionalParameters)
        {
			if(!additionalParameter) continue;
			function->variables->parametersOrdered.push_back(additionalParameter);
			function->variables->parameters[additionalParameter->id] = additionalParameter;
		}
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

}
