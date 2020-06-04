/* Copyright 2013-2019 Homegear GmbH */

#include "Search.h"
#include "GD.h"
#include "Cemi.h"
#include "DatapointTypeParsers/DpstParser.h"
#include "KnxCentral.h"

#include <sys/stat.h>
#include <zip.h>

namespace Knx
{

Search::Search(BaseLib::SharedObjects* baseLib) : _bl(baseLib)
{
}

uint64_t Search::getRoomIdByName(std::string& name)
{
    auto central = std::dynamic_pointer_cast<KnxCentral>(GD::family->getCentral());
    return central->getRoomIdByName(name);
}

void Search::addDeviceToPeerInfo(const DeviceXmlData& deviceXml, const PHomegearDevice& device, std::vector<PeerInfo>& peerInfo, std::map<int32_t, std::string>& usedTypes)
{
    try
    {
        std::string filename = _xmlPath + BaseLib::HelperFunctions::stringReplace(device->supportedDevices.at(0)->id, "/", "_") + ".xml";
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
        info.address = deviceXml.address;
        info.name = deviceXml.name;
        info.roomId = deviceXml.roomId;
        info.variableRoomIds = deviceXml.variableRoomIds;
        peerInfo.push_back(info);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Search::addDeviceToPeerInfo(PHomegearDevice& device, int32_t address, std::string name, uint64_t roomId, std::vector<PeerInfo>& peerInfo, std::map<int32_t, std::string>& usedTypes)
{
	try
	{
		std::string filename = _xmlPath + BaseLib::HelperFunctions::stringReplace(device->supportedDevices.at(0)->id, "/", "_") + ".xml";
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
		info.roomId = roomId;
		peerInfo.push_back(info);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

std::shared_ptr<HomegearDevice> Search::createHomegearDevice(Search::DeviceXmlData& deviceInfo, std::unordered_set<uint32_t>& usedTypeNumbers, std::unordered_map<std::string, uint32_t>& idTypeNumberMap)
{
    try
    {
        if(deviceInfo.address == -1) return PHomegearDevice();
        std::shared_ptr<HomegearDevice> device = std::make_shared<HomegearDevice>(_bl);
        device->version = 1;
        device->interface = deviceInfo.interface;
        PSupportedDevice supportedDevice = std::make_shared<SupportedDevice>(_bl);
        bool newDevice = true;
        auto typeNumberIterator = idTypeNumberMap.find(deviceInfo.id); //Backwards compatability
        if(typeNumberIterator != idTypeNumberMap.end() && typeNumberIterator->second > 0)
        {
            supportedDevice->typeNumber = typeNumberIterator->second;
            newDevice = false;
            std::string deviceId = deviceInfo.id;
            BaseLib::Io::deleteFile(_xmlPath + BaseLib::HelperFunctions::stringReplace(deviceId, "/", "_") + ".xml");
        }
        else
        {
            typeNumberIterator = idTypeNumberMap.find((device->interface.empty() ? "" : device->interface + "-") + Cemi::getFormattedPhysicalAddress(deviceInfo.address));
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

        supportedDevice->id = (device->interface.empty() ? "" : device->interface + "-") + Cemi::getFormattedPhysicalAddress(deviceInfo.address);
        supportedDevice->description = deviceInfo.name;
        device->supportedDevices.push_back(supportedDevice);

        createXmlMaintenanceChannel(device);

        for(const auto& groupVariable : deviceInfo.variables)
        {
            int32_t channel = 1;
            std::string variableName;
            std::string unit;
            std::unordered_map<uint64_t, BaseLib::Role> roles;

            if(!deviceInfo.description || deviceInfo.description->structValue->empty())
            {
                //No JSON in description
                if(groupVariable.second->homegearInfo)
                {
                    //Get variable data from Homegear info

                    auto& infoStruct = groupVariable.second->homegearInfo->structValue;

                    auto infoIterator = infoStruct->find("name");
                    if(infoIterator != infoStruct->end() && !infoIterator->second->stringValue.empty()) variableName = infoIterator->second->stringValue;
                    else variableName = groupVariable.second->groupVariableName;

                    infoIterator = infoStruct->find("channel");
                    if(infoIterator != infoStruct->end() && infoIterator->second->integerValue > 0)
                    {
                        channel = infoIterator->second->integerValue;
                    }

                    infoIterator = infoStruct->find("room");
                    if(infoIterator != infoStruct->end() && (uint64_t)infoIterator->second->integerValue64 > 0)
                    {
                        if(GD::bl->db->roomExists((uint64_t)infoIterator->second->integerValue64)) deviceInfo.variableRoomIds[channel][variableName] = (uint64_t)infoIterator->second->integerValue64;
                    }

                    infoIterator = infoStruct->find("role");
                    if(infoIterator != infoStruct->end())
                    {
                        auto roleIterator = infoIterator->second->structValue->find("id");
                        if(roleIterator != infoIterator->second->structValue->end())
                        {
                            Role role;
                            role.id = (uint64_t)roleIterator->second->integerValue64;

                            if(role.id != 0)
                            {
                                roleIterator = infoIterator->second->structValue->find("invert");
                                if(roleIterator != infoIterator->second->structValue->end()) role.invert = roleIterator->second->booleanValue;

                                if(groupVariable.second->readFlag && groupVariable.second->writeFlag) role.direction = RoleDirection::both;
                                else if(groupVariable.second->readFlag) role.direction = RoleDirection::input;
                                else if(groupVariable.second->writeFlag) role.direction = RoleDirection::output;
                                roles.emplace(role.id, role);
                            }
                        }
                    }
                }
                else
                {
                    //Use group variable name and parse channel, unit and roles from group variable
                    auto homegearInfoStartPos = groupVariable.second->groupVariableName.find('{');
                    auto homegearInfoEndPos = groupVariable.second->groupVariableName.find('}');
                    if(homegearInfoStartPos == std::string::npos || homegearInfoEndPos == std::string::npos)
                    {
                        variableName = groupVariable.second->groupVariableName;
                    }
                    else
                    {
                        variableName = groupVariable.second->groupVariableName.substr(0, homegearInfoStartPos) + groupVariable.second->groupVariableName.substr(homegearInfoEndPos + 1);
                        auto homegearInfo = groupVariable.second->groupVariableName.substr(homegearInfoStartPos + 1, homegearInfoEndPos - homegearInfoStartPos - 1);
                        auto homegearInfoParts = BaseLib::HelperFunctions::splitAll(homegearInfo, ';');
                        //1. channel, 2. room ID, 3. unit, 4. comma-seperated roles
                        if(!homegearInfoParts.empty())
                        {
                            channel = BaseLib::Math::getUnsignedNumber(homegearInfoParts.at(0));
                            if(channel == 0) channel = 1;
                        }
                        if(homegearInfoParts.size() >= 2)
                        {
                            unit = homegearInfoParts.at(1);
                        }
                        if(homegearInfoParts.size() >= 3)
                        {
                            uint64_t roomId = 0;
                            if(BaseLib::Math::isNumber(homegearInfoParts.at(2))) roomId = BaseLib::Math::getUnsignedNumber64(homegearInfoParts.at(2));
                            else roomId = getRoomIdByName(homegearInfoParts.at(2));
                            if(GD::bl->db->roomExists(roomId)) deviceInfo.variableRoomIds[channel][variableName] = roomId;
                        }
                        if(homegearInfoParts.size() >= 4)
                        {
                            auto roleParts = BaseLib::HelperFunctions::splitAll(homegearInfoParts.at(3), ',');
                            for(auto& rolePart : roleParts)
                            {
                                Role role;

                                if(rolePart.empty()) continue;
                                if(rolePart.back() == 'i') role.invert = true;
                                role.id = BaseLib::Math::getUnsignedNumber64(rolePart);
                                if(role.id == 0) continue;

                                if(groupVariable.second->readFlag && groupVariable.second->writeFlag) role.direction = RoleDirection::both;
                                else if(groupVariable.second->readFlag) role.direction = RoleDirection::input;
                                else if(groupVariable.second->writeFlag) role.direction = RoleDirection::output;
                                roles.emplace(role.id, role);
                            }
                        }
                    }
                }
            }
            else
            {
                //JSON exists in description
                auto variableIterator = deviceInfo.description->structValue->find(std::to_string(groupVariable.first));
                if(variableIterator == deviceInfo.description->structValue->end()) continue;

                auto structIterator = variableIterator->second->structValue->find("channel");
                if(structIterator != variableIterator->second->structValue->end()) channel = structIterator->second->integerValue;

                structIterator = variableIterator->second->structValue->find("variable");
                if(structIterator != variableIterator->second->structValue->end()) variableName = BaseLib::HelperFunctions::stringReplace(structIterator->second->stringValue, ".", "_");;

                structIterator = variableIterator->second->structValue->find("unit");
                if(structIterator != variableIterator->second->structValue->end()) unit = structIterator->second->stringValue;

                structIterator = variableIterator->second->structValue->find("roles");
                if(structIterator != variableIterator->second->structValue->end())
                {
                    for(auto& element : *structIterator->second->arrayValue)
                    {
                        auto roleIdIterator = element->structValue->find("id");
                        if(roleIdIterator == element->structValue->end())
                        {
                            GD::out.printWarning(std::string("Warning: Role of device ") + Cemi::getFormattedPhysicalAddress(deviceInfo.address) + " has no key \"id\".");
                            continue;
                        }
                        auto roleDirectionIterator = element->structValue->find("direction");
                        auto roleInvertIterator = element->structValue->find("invert");

                        Role role;
                        role.id = roleIdIterator->second->integerValue64;
                        if(roleDirectionIterator != element->structValue->end()) role.direction = (RoleDirection)roleDirectionIterator->second->integerValue;
                        if(roleInvertIterator != element->structValue->end()) role.invert = roleInvertIterator->second->booleanValue;
                        roles.emplace(role.id, role);
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

            PParameter parameter = DpstParserBase::createParameter(function, variableName.empty() ? "VALUE" : variableName, groupVariable.second->datapointType, unit, IPhysical::OperationType::command, groupVariable.second->readFlag, groupVariable.second->writeFlag, roles, groupVariable.second->address);
            if(!parameter) continue;
            parameter->transmitted = groupVariable.second->transmitFlag;

            parseDatapointType(function, groupVariable.second->datapointType, parameter);

            if(!parameter->casts.empty())
            {
                int32_t index = 1;
                std::map<std::string, PParameter>::iterator variablesIterator;
                do
                {
                    variablesIterator = function->variables->parameters.find(parameter->id);
                    if(variablesIterator != function->variables->parameters.end())
                    {
                        parameter->id = variableName + " " + std::to_string(index++);
                    }
                } while(variablesIterator != function->variables->parameters.end());

                function->variables->parametersOrdered.push_back(parameter);
                function->variables->parameters[parameter->id] = parameter;
            }
        }

        if(device->functions.size() == 1) GD::out.printWarning(std::string("Warning: Device ") + Cemi::getFormattedPhysicalAddress(deviceInfo.address) + " has no channels.");

        return device;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return PHomegearDevice();
}

std::vector<Search::PeerInfo> Search::search(std::unordered_set<uint32_t>& usedTypeNumbers, std::unordered_map<std::string, uint32_t>& idTypeNumberMap)
{
	std::vector<Search::PeerInfo> peerInfo;
	try
	{
	    auto projectFilenames = getKnxProjectFilenames();
        if(projectFilenames.empty())
        {
            GD::out.printError("Error: Could not find any KNX project in " + GD::bl->settings.deviceDescriptionPath() + std::to_string(MY_FAMILY_ID) + "/.");
            return peerInfo;
        }

        XmlData xmlData{};
        for(auto& projectFilename : projectFilenames)
        {
            auto knxProjectData = extractKnxProject(projectFilename);
            if(!knxProjectData)
            {
                GD::out.printWarning("Warning: Aborting processing of project file " + projectFilename);
                continue;
            }

            extractXmlData(xmlData, knxProjectData);
        }

        if(xmlData.groupVariableXmlData.empty() && xmlData.deviceXmlData.empty())
        {
            GD::out.printError("Error: Could not search for KNX devices. No group addresses were found in KNX project file.");
            return peerInfo;
        }

		createDirectories();

		//{{{ Group variables
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
                std::unordered_map<uint64_t, BaseLib::Role> roles;

				if(!variableXml->description) continue;

                auto structIterator = variableXml->description->structValue->find("id");
                if(structIterator == variableXml->description->structValue->end()) continue;

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

                structIterator = variableXml->description->structValue->find("variable");
                if(structIterator != variableXml->description->structValue->end()) variableName = _bl->hf.stringReplace(structIterator->second->stringValue, ".", "_");;

                structIterator = variableXml->description->structValue->find("unit");
                if(structIterator != variableXml->description->structValue->end()) unit = structIterator->second->stringValue;

                structIterator = variableXml->description->structValue->find("readable");
                if(structIterator != variableXml->description->structValue->end()) readable = structIterator->second->booleanValue;

                structIterator = variableXml->description->structValue->find("writeable");
                if(structIterator != variableXml->description->structValue->end()) writeable = structIterator->second->booleanValue;

                structIterator = variableXml->description->structValue->find("roles");
                if(structIterator != variableXml->description->structValue->end())
                {
                    for(auto& element : *structIterator->second->arrayValue)
                    {
                        auto roleIdIterator = element->structValue->find("id");
                        if(roleIdIterator == element->structValue->end())
                        {
                            GD::out.printWarning("Warning: Role has no key \"id\".");
                            continue;
                        }
                        auto roleDirectionIterator = element->structValue->find("direction");
                        auto roleInvertIterator = element->structValue->find("invert");

                        Role role;
                        role.id = roleIdIterator->second->integerValue64;
                        if(roleDirectionIterator != element->structValue->end()) role.direction = (RoleDirection)roleDirectionIterator->second->integerValue;
                        if(roleInvertIterator != element->structValue->end()) role.invert = roleInvertIterator->second->booleanValue;
                        roles.emplace(role.id, std::move(role));
                    }
                }

                std::shared_ptr<HomegearDevice> device;
                auto deviceIterator = rpcDevicesJson.find(id);
                if(deviceIterator == rpcDevicesJson.end())
                {
                    device = std::make_shared<HomegearDevice>(_bl);
                    device->version = 1;
                    PSupportedDevice supportedDevice(new SupportedDevice(_bl));
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

                PParameter parameter = DpstParserBase::createParameter(function, variableName.empty() ? "VALUE" : variableName, variableXml->datapointType, unit, IPhysical::OperationType::command, readable, writeable, roles, variableXml->address);
                if(!parameter) continue;

                parseDatapointType(function, variableXml->datapointType, parameter);

                if(!parameter->casts.empty())
                {
                    function->variables->parametersOrdered.push_back(parameter);
                    function->variables->parameters[parameter->id] = parameter;
                }
			}
		//}}}

        std::map<int32_t, std::string> usedTypeIds;

		///{{{ Devices
        {
            auto jsonOnly = GD::family->getFamilySetting("importJsonOnly");
            for(auto& deviceXml : xmlData.deviceXmlData)
            {
                if(deviceXml->address == -1)
                {
                    GD::out.printInfo("Info: Ignoring device with ID \"" + deviceXml->id + "\", because it has no physical address.");
                    continue;
                }
                if(jsonOnly && (bool)jsonOnly->integerValue && (!deviceXml->description || deviceXml->description->structValue->empty()))
                {
                    GD::out.printInfo("Info: Ignoring device with ID \"" + deviceXml->id + "\", (" + Cemi::getFormattedPhysicalAddress(deviceXml->address) + ") because it has no JSON description.");
                    continue;
                }
                auto device = createHomegearDevice(*deviceXml, usedTypeNumbers, idTypeNumberMap);
                if(device)
                {
                    auto typeId = (*device->supportedDevices.begin())->id;
                    addDeviceToPeerInfo(*deviceXml, device, peerInfo, usedTypeIds);
                }
            }
        }
		//}}}

		for(auto& i : rpcDevicesJson)
		{
			addDeviceToPeerInfo(i.second, -1, "", 0, peerInfo, usedTypeIds);
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
        deviceXml.roomId = getRoomIdByName(structIterator->second->stringValue);

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
                deviceXml.description = BaseLib::Rpc::JsonDecoder::decode(jsonString);
            }
            catch(const std::exception& ex)
            {
                _bl->out.printError("Error decoding JSON in KNX device information: " + std::string(ex.what()));
                return PeerInfo();
            }
        }

        if(!deviceXml.description) deviceXml.description = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

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

            auto variableIterator = variableElement.second->structValue->find("name");
            if(variableIterator == variableElement.second->structValue->end() || variableIterator->second->stringValue.empty())
            {
                GD::out.printError("Error: Group variable with index " + variableElement.first + " has no field \"name\" or the name is empty.");
                return PeerInfo();
            }
            variable->groupVariableName = variableIterator->second->stringValue;

            variableIterator = variableElement.second->structValue->find("address");
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

            //Contains "name", "channel", "room" and "roles"
            variableIterator = variableElement.second->structValue->find("homegearInfo");
            if(variableIterator != variableElement.second->structValue->end()) variable->homegearInfo = variableIterator->second;

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

        addDeviceToPeerInfo(deviceXml, device, peerInfo, usedTypeIds);

        if(!peerInfo.empty()) return *peerInfo.begin();
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
			if(chown(path1.c_str(), localUserId, localGroupId) == -1) GD::out.printWarning("Could not set owner on " + path1);
			if(chmod(path1.c_str(), GD::bl->settings.dataPathPermissions()) == -1) GD::out.printWarning("Could not set permissions on " + path1);
		}
		if(!BaseLib::Io::directoryExists(path2)) BaseLib::Io::createDirectory(path2, GD::bl->settings.dataPathPermissions());
		if(localUserId != 0 || localGroupId != 0)
		{
			if(chown(path2.c_str(), localUserId, localGroupId) == -1) GD::out.printWarning("Could not set owner on " + path2);
			if(chmod(path2.c_str(), GD::bl->settings.dataPathPermissions()) == -1) GD::out.printWarning("Could not set permissions on " + path2);
		}
		if(!BaseLib::Io::directoryExists(_xmlPath)) BaseLib::Io::createDirectory(_xmlPath, GD::bl->settings.dataPathPermissions());
		if(localUserId != 0 || localGroupId != 0)
		{
			if(chown(_xmlPath.c_str(), localUserId, localGroupId) == -1) GD::out.printWarning("Could not set owner on " + _xmlPath);
			if(chmod(_xmlPath.c_str(), GD::bl->settings.dataPathPermissions()) == -1) GD::out.printWarning("Could not set permissions on " + _xmlPath);
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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

		PParameter parameter(new Parameter(_bl, function->variables));
		parameter->id = "UNREACH";
		function->variables->parametersOrdered.push_back(parameter);
		function->variables->parameters[parameter->id] = parameter;
		parameter->writeable = false;
		parameter->service = true;
		parameter->logical = std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(_bl);;
		parameter->physical = std::make_shared<BaseLib::DeviceDescription::PhysicalInteger>(_bl);
		parameter->physical->groupId = parameter->id;
		parameter->physical->operationType = IPhysical::OperationType::internal;

		parameter.reset(new Parameter(_bl, function->variables));
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

std::vector<std::string> Search::getKnxProjectFilenames()
{
    std::string projectPath = _bl->settings.deviceDescriptionPath() + std::to_string(GD::family->getFamily()) + '/';
    std::vector<std::string> files = _bl->io.getFiles(projectPath);
    std::vector<std::string> projectFilenames;
    projectFilenames.reserve(files.size());
    for(auto& file : files)
    {
        if(file.size() > 8 && file.compare(file.size() - 8, 8, ".knxproj") == 0)
        {
            projectFilenames.push_back(projectPath + file);
        }
    }
    return projectFilenames;
}

Search::PProjectData Search::extractKnxProject(const std::string& projectFilename)
{
	try
	{
        BaseLib::Rpc::RpcDecoder rpcDecoder;

        auto currentProjectData = std::make_shared<ProjectData>();
        currentProjectData->filename = BaseLib::HelperFunctions::splitLast(projectFilename, '/').second;

        int32_t error = 0;
        zip_error_t zipError;
        zip* projectArchive = zip_open(projectFilename.c_str(), 0, &error);
        if(!projectArchive)
        {
            if(error == ZIP_ER_OPEN) GD::out.printError("Error: Could not open project archive. Please check file permissions and make sure, Homegear as read access.");
            else if(error == ZIP_ER_NOZIP) GD::out.printError("Error: Could not open project archive. The file is no valid KNX project (not a valid ZIP archive).");
            else GD::out.printError("Error: Could not open project archive. Error code: " + std::to_string(error));
            return PProjectData();
        }

        zip_int64_t filesInArchive = zip_get_num_entries(projectArchive, 0);
        for(zip_uint64_t i = 0; i < (zip_uint64_t)filesInArchive; ++i)
        {
            auto content = std::make_shared<std::vector<char>>();

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
            bool isProjectXml = false;
            bool isHomegearInfo = false;
            bool isGroupVariableInfo = false;
            bool isProjectZip = false;
            if((filename.compare(0, 2, "P-") == 0) || (filename.compare(0, 2, "p-") == 0))
            {
                auto parts = BaseLib::HelperFunctions::splitFirst(filename, '/');
                if(parts.second.empty()) parts = BaseLib::HelperFunctions::splitFirst(filename, '.');
                currentProjectData->projectId = BaseLib::HelperFunctions::toUpper(parts.first);

                isProjectXml = (filename.compare(filename.size() - 6, 6, "/0.xml") == 0);
                if(filename.size() >= 17) isHomegearInfo = (filename.compare(filename.size() - 17, 17, "/homegearInfo.dat") == 0);
                if(filename.size() >= 30) isGroupVariableInfo = (filename.compare(filename.size() - 30, 30, "/homegearGroupVariableInfo.dat") == 0);
                isProjectZip = (filename.compare(filename.size() - 4, 4, ".zip") == 0);
            }
            if(isProjectZip)
            {
                std::vector<char> projectZip;
                projectZip.resize(st.size);
                zip_file* projectZipFile = zip_fopen(projectArchive, filename.c_str(), 0);
                if(zip_fread(projectZipFile, projectZip.data(), st.size) != (signed)st.size)
                {
                    GD::out.printError("Error: Could not read project zip file in archive. Error code: " + std::to_string(error));
                    continue;
                }
                zip_fclose(projectZipFile);

                auto projectZipSource = zip_source_buffer_create(projectZip.data(), projectZip.size(), 0, &zipError);
                if(!projectZipSource)
                {
                    GD::out.printError("Error: Could not create buffer for project zip file. Error: " + std::string(zipError.str));
                    continue;
                }

                auto projectZipArchive = zip_open_from_source(projectZipSource, 0, &zipError);
                if(!projectZipArchive)
                {
                    GD::out.printError("Error: Could not open project zip file. Error: " + std::string(zipError.str));
                    continue;
                }

                zip_int64_t filesInProjectArchive = zip_get_num_entries(projectZipArchive, 0);
                for(zip_uint64_t j = 0; j < (zip_uint64_t)filesInProjectArchive; ++j)
                {
                    struct zip_stat projectSt{};
                    zip_stat_init(&projectSt);
                    if(zip_stat_index(projectZipArchive, j, 0, &projectSt) == -1)
                    {
                        GD::out.printError("Error: Could not get information for project file at index " + std::to_string(j));
                        continue;
                    }
                    if(!(projectSt.valid & ZIP_STAT_NAME) || !(projectSt.valid & ZIP_STAT_SIZE))
                    {
                        GD::out.printError("Error: Could not get name or size for project file at index " + std::to_string(j));
                        continue;
                    }

                    std::string projectZipFilename(projectSt.name);

                    auto password = GD::family->getFamilySetting("knxProjectPassword");
                    if(password && !password->stringValue.empty()) zip_set_default_password(projectZipArchive, password->stringValue.c_str());

                    content->resize(projectSt.size + 1);
                    zip_file* projectFile = zip_fopen(projectZipArchive, projectZipFilename.c_str(), 0);
                    if(!projectFile)
                    {
                        GD::out.printError("Error: Could not open project file in ZIP archive. Wrong password?");
                        continue;
                    }
                    if(zip_fread(projectFile, content->data(), projectSt.size) != (signed)projectSt.size)
                    {
                        GD::out.printError("Error: Could not read project file in ZIP archive.");
                        continue;
                    }
                    content->back() = '\0';
                    zip_fclose(projectFile);

                    if(projectZipFilename == "0.xml") currentProjectData->projectXml = content;
                    else if(projectZipFilename.size() >= 17 && projectZipFilename.compare(projectZipFilename.size() - 17, 17, "/homegearInfo.dat") == 0)
                    {
                        GD::out.printInfo("Info: Project contains generic Homegear-specific data.");
                        currentProjectData->homegearInfo = rpcDecoder.decodeResponse(*content);
                    }
                    else if(projectZipFilename.size() >= 30 && projectZipFilename.compare(projectZipFilename.size() - 30, 30, "/homegearGroupVariableInfo.dat") == 0)
                    {
                        GD::out.printInfo("Info: Project contains Homegear-specific group variable data.");
                        currentProjectData->groupVariableInfo = rpcDecoder.decodeResponse(*content);
                    }
                    else currentProjectData->xmlFiles.emplace(BaseLib::HelperFunctions::toLower(filename), content);
                }

                zip_close(projectZipArchive);
            }
            else
            {
                content->resize(st.size + 1);
                zip_file* projectFile = zip_fopen(projectArchive, filename.c_str(), 0);
                if(!projectFile)
                {
                    GD::out.printError("Error: Could not open project file in archive.");
                    continue;
                }
                if(zip_fread(projectFile, content->data(), st.size) != (signed)st.size)
                {
                    GD::out.printError("Error: Could not read project file in archive.");
                    continue;
                }
                content->back() = '\0';
                zip_fclose(projectFile);

                if(isProjectXml) currentProjectData->projectXml = content;
                else if(isHomegearInfo)
                {
                    GD::out.printInfo("Project contains generic Homegear-specific data.");
                    currentProjectData->homegearInfo = rpcDecoder.decodeResponse(*content);
                }
                else if(isGroupVariableInfo)
                {
                    GD::out.printInfo("Project contains Homegear-specific group variable info.");
                    currentProjectData->groupVariableInfo = rpcDecoder.decodeResponse(*content);
                }
                else currentProjectData->xmlFiles.emplace(BaseLib::HelperFunctions::toLower(filename), content);
            }
        }

        zip_close(projectArchive);

        //{{{ Get project name
        auto projectFileName = currentProjectData->projectId + "/project.xml";
        BaseLib::HelperFunctions::toLower(projectFileName);
        auto projectFileIterator = currentProjectData->xmlFiles.find(projectFileName);
        if(projectFileIterator != currentProjectData->xmlFiles.end())
        {
            xml_document doc;
            try
            {
                char* startPos = (!projectFileIterator->second || projectFileIterator->second->empty() ? nullptr : (char*)memchr(projectFileIterator->second->data(), '<', 10));
                if(!startPos)
                {
                    _bl->out.printError("Error: No '<' found in \"project.xml\".");
                    doc.clear();
                    return PProjectData();
                }
                doc.parse<parse_no_entity_translation>(startPos);
                xml_node* rootNode = doc.first_node("KNX");
                if(!rootNode)
                {
                    _bl->out.printError(R"(Error: "project.xml" does not start with "KNX".)");
                    doc.clear();
                    return PProjectData();
                }
                xml_node* projectNode = rootNode->first_node("Project");
                if(projectNode)
                {
                    xml_node* projectInformationNode = projectNode->first_node("ProjectInformation");
                    if(projectInformationNode)
                    {
                        auto nameAttribute = projectInformationNode->first_attribute("Name");
                        if(nameAttribute) currentProjectData->projectName = std::string(nameAttribute->value(), nameAttribute->value_size());
                        auto styleAttribute = projectInformationNode->first_attribute("GroupAddressStyle");
                        if(styleAttribute)
                        {
                            std::string groupAddressStyle = std::string(styleAttribute->value(), styleAttribute->value_size());
                            if(groupAddressStyle != "ThreeLevel")
                            {
                                _bl->out.printError("Error: Group address style is not set to \"three level\".");
                                doc.clear();
                                return PProjectData();
                            }
                        }
                    }
                }
            }
            catch(const std::exception& ex)
            {
                GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
                doc.clear();
                return PProjectData();
            }
            doc.clear();
        }
        else _bl->out.printWarning("Warning: No file \"project.xml\" found.");
        //}}}

        return currentProjectData;
    }
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return PProjectData();
}

void Search::assignRoomsToDevices(xml_node* currentNode, std::string currentRoom, std::unordered_map<std::string, std::shared_ptr<DeviceXmlData>>& devices)
{
	try
	{
		for(xml_node* buildingPartNode = currentNode->first_node("BuildingPart"); buildingPartNode; buildingPartNode = buildingPartNode->next_sibling("BuildingPart"))
		{
			std::string room;
			xml_attribute* attribute = buildingPartNode->first_attribute("Type");
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
			for(xml_node* deviceNode = currentNode->first_node("DeviceInstanceRef"); deviceNode; deviceNode = deviceNode->next_sibling("DeviceInstanceRef"))
			{
				xml_attribute* attribute = deviceNode->first_attribute("RefId");
				if(!attribute) continue;
				std::string deviceId = std::string(attribute->value());
				if(deviceId.empty()) continue;
				auto devicesIterator = devices.find(deviceId);
				if(devicesIterator == devices.end()) continue;
				devicesIterator->second->roomId = getRoomIdByName(currentRoom);
			}
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

std::unordered_map<std::string, Search::PManufacturerData> Search::extractManufacturerXmlData(const Search::PProjectData& projectData)
{
    try
    {
        std::unordered_map<std::string, PManufacturerData> result;
        for(auto& file : projectData->xmlFiles)
        {
            if(file.first.size() < 13 || file.first.compare(0, 2, "m-") != 0 || file.first.compare(file.first.size() - 13, 13, "/hardware.xml") != 0) continue;
            auto pair = BaseLib::HelperFunctions::splitFirst(file.first, '/');
            auto manufacturerId = BaseLib::HelperFunctions::toUpper(pair.first);

            auto manufacturerData = std::make_shared<ManufacturerData>();

            std::unordered_set<std::string> applicationProgramRefs;

            { //Get all ApplicationProgramRef
                xml_document doc;
                try
                {
                    char* startPos = (char*)memchr(file.second->data(), '<', 10);
                    if(!startPos)
                    {
                        _bl->out.printError("Error: No '<' found in \"" + file.first + "\".");
                        doc.clear();
                        continue;
                    }
                    doc.parse<parse_no_entity_translation | parse_validate_closing_tags>(startPos);
                    xml_node* rootNode = doc.first_node("KNX");
                    if(!rootNode)
                    {
                        _bl->out.printError("Error: \"" + file.first + R"(" does not start with "KNX".)");
                        doc.clear();
                        continue;
                    }
                    xml_node* manufacturerDataNode = rootNode->first_node("ManufacturerData");
                    if(manufacturerDataNode)
                    {
                        xml_node* manufacturerNode = manufacturerDataNode->first_node("Manufacturer");
                        if(manufacturerNode)
                        {
                            xml_node* hardwareGroupNode = manufacturerNode->first_node("Hardware");
                            if(hardwareGroupNode)
                            {
                                for(xml_node* hardwareNode = hardwareGroupNode->first_node("Hardware"); hardwareNode; hardwareNode = hardwareNode->next_sibling("Hardware"))
                                {
                                    xml_node* hardware2ProgramsNode = hardwareNode->first_node("Hardware2Programs");
                                    if(hardware2ProgramsNode)
                                    {
                                        for(xml_node* hardware2ProgramNode = hardware2ProgramsNode->first_node("Hardware2Program"); hardware2ProgramNode; hardware2ProgramNode = hardware2ProgramNode->next_sibling("Hardware2Program"))
                                        {
                                            auto idAttribute = hardware2ProgramNode->first_attribute("Id");
                                            if(!idAttribute) continue;

                                            std::string hardware2ProgramId(idAttribute->value(), idAttribute->value_size());
                                            if(hardware2ProgramId.empty()) continue;

                                            auto& hardware2ProgramRefs = manufacturerData->hardware2programRefs[hardware2ProgramId];
                                            hardware2ProgramRefs.reserve(10);

                                            for(xml_node* applicationProgramRefNode = hardware2ProgramNode->first_node("ApplicationProgramRef"); applicationProgramRefNode; applicationProgramRefNode = applicationProgramRefNode->next_sibling("ApplicationProgramRef"))
                                            {
                                                auto attribute = applicationProgramRefNode->first_attribute("RefId");
                                                if(attribute)
                                                {
                                                    auto value = std::string(attribute->value(), attribute->value_size());
                                                    if(!value.empty())
                                                    {
                                                        hardware2ProgramRefs.emplace_back(value);
                                                        applicationProgramRefs.emplace(value);
                                                    }
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
                    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
                }
                doc.clear();
            }

            for(auto& applicationProgramRef : applicationProgramRefs)
            {
                //Read all ApplicationProgramRef files to extract device data
                auto filename = manufacturerId + '/' + applicationProgramRef + ".xml";
                BaseLib::HelperFunctions::toLower(filename);
                auto fileEntry = projectData->xmlFiles.find(filename);
                if(fileEntry == projectData->xmlFiles.end())
                {
                    GD::out.printWarning("Warning: File \"" + filename + "\" not found.");
                    continue;
                }

                xml_document doc;
                try
                {
                    char* startPos = (char*)memchr(fileEntry->second->data(), '<', 10);
                    if(!startPos)
                    {
                        _bl->out.printError("Error: No '<' found in \"" + fileEntry->first + "\".");
                        doc.clear();
                        continue;
                    }
                    doc.parse<parse_no_entity_translation | parse_validate_closing_tags>(startPos);
                    xml_node* rootNode = doc.first_node("KNX");
                    if(!rootNode)
                    {
                        _bl->out.printError("Error: \"" + file.first + "\" does not start with \"KNX\".");
                        doc.clear();
                        continue;
                    }

                    xml_node* manufacturerDataNode = rootNode->first_node("ManufacturerData");
                    if(manufacturerDataNode)
                    {
                        xml_node* manufacturerNode = manufacturerDataNode->first_node("Manufacturer");
                        if(manufacturerNode)
                        {
                            xml_node* applicationProgramsNode = manufacturerNode->first_node("ApplicationPrograms");
                            if(applicationProgramsNode)
                            {
                                for(xml_node* applicationProgramNode = applicationProgramsNode->first_node("ApplicationProgram"); applicationProgramNode; applicationProgramNode = applicationProgramNode->next_sibling("ApplicationProgram"))
                                {
                                    auto programTypeAttribute = applicationProgramNode->first_attribute("ProgramType");
                                    if(!programTypeAttribute || std::string(programTypeAttribute->value(), programTypeAttribute->value_size()) != "ApplicationProgram") continue;

                                    std::string applicationProgramId;

                                    { //Get ID
                                        auto idAttribute = applicationProgramNode->first_attribute("Id");
                                        if(!idAttribute) continue;

                                        applicationProgramId = std::string(idAttribute->value(), idAttribute->value_size());
                                        if(applicationProgramId.empty()) continue;
                                    }

                                    auto staticNode = applicationProgramNode->first_node("Static");
                                    if(staticNode)
                                    {
                                        //{{{ Get all ComObject
                                        std::unordered_map<std::string, PComObjectData> allComObjects;

                                        auto comObjectTableNode = staticNode->first_node("ComObjectTable");
                                        if(comObjectTableNode)
                                        {
                                            for(xml_node* comObjectNode = comObjectTableNode->first_node("ComObject"); comObjectNode; comObjectNode = comObjectNode->next_sibling("ComObject"))
                                            {
                                                auto comObjectData = std::make_shared<ComObjectData>();

                                                auto idAttribute = comObjectNode->first_attribute("Id");
                                                if(!idAttribute) continue;

                                                std::string id(idAttribute->value(), idAttribute->value_size());
                                                if(id.empty()) continue;

                                                auto nameAttribute = comObjectNode->first_attribute("Name");
                                                if(!nameAttribute) continue;
                                                comObjectData->name = std::string(nameAttribute->value(), nameAttribute->value_size());

                                                auto functionTextAttribute = comObjectNode->first_attribute("FunctionText");
                                                if(functionTextAttribute)
                                                {
                                                    comObjectData->functionText = std::string(functionTextAttribute->value(), functionTextAttribute->value_size());
                                                }

                                                std::array<std::pair<std::string, bool*>, 6> flagsToRead{
                                                        std::make_pair("CommunicationFlag", &comObjectData->communicationFlag),
                                                        std::make_pair("ReadFlag", &comObjectData->readFlag),
                                                        std::make_pair("ReadOnInitFlag", &comObjectData->readOnInitFlag),
                                                        std::make_pair("TransmitFlag", &comObjectData->transmitFlag),
                                                        std::make_pair("UpdateFlag", &comObjectData->updateFlag),
                                                        std::make_pair("WriteFlag", &comObjectData->writeFlag)
                                                };

                                                for(auto& flagToRead : flagsToRead)
                                                {
                                                    auto flagAttribute = comObjectNode->first_attribute(flagToRead.first.c_str());
                                                    if(!flagAttribute) continue;

                                                    auto value = std::string(flagAttribute->value(), flagAttribute->value_size());
                                                    if(value != "Enabled" && value != "Disabled") GD::out.printWarning("Warning: Unknown value for " + flagToRead.first + ": " + value);
                                                    *flagToRead.second = (value != "Disabled");
                                                }

                                                allComObjects.emplace(id, std::move(comObjectData));
                                            }
                                        }
                                        //}}}

                                        auto currentProductData = std::make_shared<ManufacturerProductData>();

                                        //{{{ Get all ComObjectRefs and fill comObjectData
                                        auto comObjectRefsNode = staticNode->first_node("ComObjectRefs");
                                        if(comObjectRefsNode)
                                        {
                                            for(xml_node* comObjectRefNode = comObjectRefsNode->first_node("ComObjectRef"); comObjectRefNode; comObjectRefNode = comObjectRefNode->next_sibling("ComObjectRef"))
                                            {
                                                auto idAttribute = comObjectRefNode->first_attribute("Id");
                                                if(!idAttribute) continue;

                                                std::string id(idAttribute->value(), idAttribute->value_size());
                                                if(id.empty()) continue;

                                                auto refIdAttribute = comObjectRefNode->first_attribute("RefId");
                                                if(!refIdAttribute) continue;

                                                std::string refId(refIdAttribute->value(), refIdAttribute->value_size());
                                                if(refId.empty()) continue;

                                                auto comObjectIterator = allComObjects.find(refId);
                                                if(comObjectIterator != allComObjects.end())
                                                {
                                                    PComObjectData comObjectData = std::make_shared<ComObjectData>();
                                                    *comObjectData = *comObjectIterator->second;

                                                    //{{{ Read attributes at this point again. Attributes here overwrite the attributes in ComObject.
                                                    auto functionTextAttribute = comObjectRefNode->first_attribute("FunctionText");
                                                    if(functionTextAttribute)
                                                    {
                                                        comObjectData->functionText = std::string(functionTextAttribute->value(), functionTextAttribute->value_size());
                                                    }

                                                    std::array<std::pair<std::string, bool*>, 6> flagsToRead{
                                                            std::make_pair("CommunicationFlag", &comObjectData->communicationFlag),
                                                            std::make_pair("ReadFlag", &comObjectData->readFlag),
                                                            std::make_pair("ReadOnInitFlag", &comObjectData->readOnInitFlag),
                                                            std::make_pair("TransmitFlag", &comObjectData->transmitFlag),
                                                            std::make_pair("UpdateFlag", &comObjectData->updateFlag),
                                                            std::make_pair("WriteFlag", &comObjectData->writeFlag)
                                                    };

                                                    for(auto& flagToRead : flagsToRead)
                                                    {
                                                        auto flagAttribute = comObjectRefNode->first_attribute(flagToRead.first.c_str());
                                                        if(!flagAttribute) continue;

                                                        auto value = std::string(flagAttribute->value(), flagAttribute->value_size());
                                                        if(value != "Enabled" && value != "Disabled") GD::out.printWarning("Warning: Unknown value for " + flagToRead.first + ": " + value);
                                                        *flagToRead.second = (value != "Disabled");
                                                    }
                                                    //}}}

                                                    currentProductData->comObjectData.emplace(id, comObjectData);
                                                }
                                            }
                                        }
                                        manufacturerData->productData.emplace(applicationProgramId, std::move(currentProductData));
                                        //}}}
                                    }
                                }
                            }
                        }
                    }
                }
                catch(const std::exception& ex)
                {
                    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
                }
                doc.clear();
            }

            result.emplace(manufacturerId, manufacturerData);
        }

        return result;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return std::unordered_map<std::string, Search::PManufacturerData>();
}

void Search::extractXmlData(XmlData& xmlData, const PProjectData& projectData)
{
    std::unordered_map<std::string, Search::PManufacturerData> manufacturerData = extractManufacturerXmlData(projectData);

    xml_document doc;
    try
    {
        char* startPos = (!projectData->projectXml || projectData->projectXml->empty()) ? nullptr : (char*)memchr(projectData->projectXml->data(), '<', 10);
        if(!startPos)
        {
            _bl->out.printError("Error: No '<' found in KNX project XML.");
            doc.clear();
            return;
        }
        doc.parse<parse_no_entity_translation | parse_validate_closing_tags>(startPos);
        xml_node* rootNode = doc.first_node("KNX");
        if(!rootNode)
        {
            _bl->out.printError("Error: KNX project XML does not start with \"KNX\".");
            doc.clear();
            return;
        }

        std::unordered_map<std::string, std::shared_ptr<DeviceXmlData>> deviceById;
        std::unordered_map<std::string, std::set<std::shared_ptr<DeviceXmlData>>> deviceByGroupVariable;
        for(xml_node* projectNode = rootNode->first_node("Project"); projectNode; projectNode = projectNode->next_sibling("Project"))
        {
            for(xml_node* installationsNode = projectNode->first_node("Installations"); installationsNode; installationsNode = installationsNode->next_sibling("Installations"))
            {
                for(xml_node* installationNode = installationsNode->first_node("Installation"); installationNode; installationNode = installationNode->next_sibling("Installation"))
                {
                    for(xml_node* topologyNode = installationNode->first_node("Topology"); topologyNode; topologyNode = topologyNode->next_sibling("Topology"))
                    {
                        int32_t currentArea = -1;
                        int32_t currentLine = -1;
                        int32_t currentAddress = -1;
                        for(xml_node* areaNode = topologyNode->first_node("Area"); areaNode; areaNode = areaNode->next_sibling("Area"))
                        {
                            std::string attributeValue;
                            xml_attribute* attribute = areaNode->first_attribute("Address");
                            if(!attribute) continue;
                            attributeValue = std::string(attribute->value());
                            currentArea = Math::getNumber(attributeValue) & 0x0F;

                            for(xml_node* lineNode = areaNode->first_node("Line"); lineNode; lineNode = lineNode->next_sibling("Line"))
                            {
                                attribute = lineNode->first_attribute("Address");
                                if(!attribute) continue;
                                attributeValue = std::string(attribute->value());
                                currentLine = Math::getNumber(attributeValue) & 0x0F;

                                for(xml_node* deviceNode = lineNode->first_node("DeviceInstance"); deviceNode; deviceNode = deviceNode->next_sibling("DeviceInstance"))
                                {
                                    std::shared_ptr<DeviceXmlData> device = std::make_shared<DeviceXmlData>();

                                    //{{{ Assign interface
                                    if(projectData->homegearInfo)
                                    {
                                        auto interfaceIterator = projectData->homegearInfo->structValue->find("interface");
                                        if(interfaceIterator != projectData->homegearInfo->structValue->end())
                                        {
                                            device->interface = interfaceIterator->second->stringValue;
                                        }
                                    }

                                    //Interface name from filename overwrites interface name from config
                                    auto pos = projectData->filename.find("##");
                                    if(pos != std::string::npos)
                                    {
                                        auto pos2 = projectData->filename.find("##", pos + 1);
                                        if(pos2 != std::string::npos)
                                        {
                                            auto metadataString = projectData->filename.substr(pos + 2, pos2 - pos - 2);
                                            auto metadataParts = BaseLib::HelperFunctions::splitFirst(metadataString, '=');
                                            if(metadataParts.first == "if") device->interface = metadataParts.second;
                                        }
                                    }
                                    //}}}

                                    attribute = deviceNode->first_attribute("Address");
                                    if(!attribute) continue;
                                    attributeValue = std::string(attribute->value());
                                    currentAddress = Math::getNumber(attributeValue) & 0xFF;
                                    device->address = (currentArea << 12) | (currentLine << 8) | currentAddress;

                                    GD::out.printDebug("Debug: Found device " + Cemi::getFormattedPhysicalAddress(device->address));
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
                                        GD::out.printDebug("Debug: Found JSON for device " + Cemi::getFormattedPhysicalAddress(device->address));
                                        attributeValue = attributeValue.substr(jsonStartPos + 2);
                                        std::string jsonString;
                                        BaseLib::Html::unescapeHtmlEntities(attributeValue, jsonString);
                                        try
                                        {
                                            BaseLib::PVariable json = BaseLib::Rpc::JsonDecoder::decode(jsonString);
                                            device->description = json;
                                        }
                                        catch(const std::exception& ex)
                                        {
                                            _bl->out.printError("Error decoding JSON of device \"" + device->name + "\" with ID \"" + device->id + "\": " + ex.what());
                                            continue;
                                        }
                                        GD::out.printDebug("Debug: Successfully parsed JSON of device " + Cemi::getFormattedPhysicalAddress(device->address));
                                    }

                                    //{{{ Get product data and application program reference ID
                                    std::vector<PManufacturerProductData> productData;
                                    std::string applicationProgramRefId;

                                    {
                                        std::string hardware2ProgramRefId;
                                        attribute = deviceNode->first_attribute("Hardware2ProgramRefId");
                                        if(attribute) hardware2ProgramRefId = std::string(attribute->value(), attribute->value_size());
                                        else GD::out.printWarning("Warning: Hardware2ProgramRefId not found.");

                                        std::string manufacturerId = BaseLib::HelperFunctions::splitFirst(hardware2ProgramRefId, '_').first;
                                        auto manufacturerDataIterator = manufacturerData.find(manufacturerId);
                                        if(manufacturerDataIterator == manufacturerData.end())
                                        {
                                            GD::out.printError("Error: Manufacturer " + manufacturerId + " not found in XML file.");
                                            continue;
                                        }

                                        auto applicationProgramRefIdsIterator = manufacturerDataIterator->second->hardware2programRefs.find(hardware2ProgramRefId);
                                        if(applicationProgramRefIdsIterator == manufacturerDataIterator->second->hardware2programRefs.end() || applicationProgramRefIdsIterator->second.empty())
                                        {
                                            GD::out.printError("Error: No application program found for device " + device->id);
                                            continue;
                                        }

                                        productData.reserve(applicationProgramRefIdsIterator->second.size());

                                        for(auto& applicationProgramRefId : applicationProgramRefIdsIterator->second)
                                        {
                                            auto productDataIterator = manufacturerDataIterator->second->productData.find(applicationProgramRefId);
                                            if(productDataIterator != manufacturerDataIterator->second->productData.end())
                                            {
                                                productData.emplace_back(productDataIterator->second);
                                            }
                                        }

                                        if(productData.empty())
                                        {
                                            GD::out.printError("Error: No application program dound for device (2) " + device->id);
                                            continue;
                                        }
                                    }
                                    //}}}

                                    for(xml_node* comInstanceRefsNode = deviceNode->first_node("ComObjectInstanceRefs"); comInstanceRefsNode; comInstanceRefsNode = comInstanceRefsNode->next_sibling("ComObjectInstanceRefs"))
                                    {
                                        for(xml_node* comInstanceRefNode = comInstanceRefsNode->first_node("ComObjectInstanceRef"); comInstanceRefNode; comInstanceRefNode = comInstanceRefNode->next_sibling("ComObjectInstanceRef"))
                                        {
                                            GroupVariableInfo variableInfo;

                                            attribute = comInstanceRefNode->first_attribute("DatapointType");
                                            if(attribute) variableInfo.datapointType = std::string(attribute->value());

                                            std::string fullReferenceId;
                                            attribute = comInstanceRefNode->first_attribute("RefId");
                                            if(attribute)
                                            {
                                                std::string referenceId = std::string(attribute->value());
                                                std::vector<std::string> parts = BaseLib::HelperFunctions::splitAll(referenceId, '_');
                                                if(parts.size() >= 3)
                                                {
                                                    fullReferenceId = referenceId;
                                                    auto indexPair = BaseLib::HelperFunctions::splitLast(parts.at(2), '-');
                                                    variableInfo.index = BaseLib::Math::getNumber(indexPair.second, false);
                                                }
                                                else if(parts.size() == 2) //>= ETS5.7
                                                {
                                                    fullReferenceId = applicationProgramRefId + '_' + referenceId;
                                                    auto indexPair = BaseLib::HelperFunctions::splitLast(parts.at(0), '-');
                                                    variableInfo.index = BaseLib::Math::getNumber(indexPair.second, false);
                                                }
                                            }

                                            //{{{ Get default flags from application program.
                                            for(auto& productDataEntry : productData)
                                            {
                                                auto productDataIterator = productDataEntry->comObjectData.find(fullReferenceId);
                                                if(productDataIterator != productDataEntry->comObjectData.end())
                                                {
                                                    variableInfo.name = productDataIterator->second->name;
                                                    variableInfo.functionText = productDataIterator->second->functionText;
                                                    variableInfo.writeFlag = productDataIterator->second->writeFlag;
                                                    variableInfo.readFlag = productDataIterator->second->readFlag;
                                                    variableInfo.transmitFlag = productDataIterator->second->transmitFlag;
                                                    break;
                                                }
                                            }
                                            //}}}

                                            //{{{ Overwrite default flags. In ETS versions before 5.7 the flags seem always to be set here in deviceNode (needs to be reverified). In ETS >= 5.7 the flags are specified in ComObjectInstanceRef, if they are different from the default.
                                            attribute = deviceNode->first_attribute("WriteFlag");
                                            if(attribute)
                                            {
                                                attributeValue = std::string(attribute->value());
                                                variableInfo.writeFlag = attributeValue != "Disabled";
                                            }
                                            attribute = comInstanceRefNode->first_attribute("WriteFlag");
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
                                            attribute = comInstanceRefNode->first_attribute("ReadFlag");
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
                                            attribute = comInstanceRefNode->first_attribute("TransmitFlag");
                                            if(attribute)
                                            {
                                                attributeValue = std::string(attribute->value());
                                                variableInfo.transmitFlag = attributeValue != "Disabled";
                                            }
                                            //}}}

                                            if(variableInfo.index == -1) continue;

                                            attribute = comInstanceRefNode->first_attribute("Links");
                                            if(attribute) //>= ETS5.7
                                            {
                                                attributeValue = std::string(attribute->value());
                                                std::vector<std::string> groupAddresses = BaseLib::HelperFunctions::splitAll(attributeValue, ' ');
                                                for(int32_t i = 0; i < (signed)groupAddresses.size(); i++)
                                                {
                                                    auto& groupAddress = groupAddresses[i];
                                                    if(groupAddress.empty()) continue;
                                                    GD::out.printDebug("Debug: Device " + Cemi::getFormattedPhysicalAddress(device->address) + " has group address " + groupAddress);
                                                    deviceByGroupVariable[groupAddress].emplace(device);

                                                    if(i > 0)
                                                    {
                                                        //Only first entry is writeable
                                                        GroupVariableInfo receiveVariableInfo = variableInfo;
                                                        receiveVariableInfo.writeFlag = false;
                                                        device->variableInfo[groupAddress].push_back(receiveVariableInfo);
                                                    }
                                                    else
                                                    {
                                                        //Needs to be a list, because the same group variable might be assigned to one device more than once
                                                        device->variableInfo[groupAddress].push_back(variableInfo);
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                for(xml_node* connectorsNode = comInstanceRefNode->first_node("Connectors"); connectorsNode; connectorsNode = connectorsNode->next_sibling("Connectors"))
                                                {
                                                    for(xml_node* sendNode = connectorsNode->first_node("Send"); sendNode; sendNode = sendNode->next_sibling("Send"))
                                                    {
                                                        attribute = sendNode->first_attribute("GroupAddressRefId");
                                                        if(!attribute) continue;
                                                        attributeValue = std::string(attribute->value());
                                                        if(attributeValue.empty()) continue;
                                                        deviceByGroupVariable[attributeValue].emplace(device);
                                                        //Needs to be a list, because the same group variable might be assigned to one device more than once
                                                        device->variableInfo[attributeValue].push_back(variableInfo);
                                                    }

                                                    for(xml_node* receiveNode = connectorsNode->first_node("Receive"); receiveNode; receiveNode = receiveNode->next_sibling("Receive"))
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

        for(xml_node* projectNode = rootNode->first_node("Project"); projectNode; projectNode = projectNode->next_sibling("Project"))
        {
            for(xml_node* installationsNode = projectNode->first_node("Installations"); installationsNode; installationsNode = installationsNode->next_sibling("Installations"))
            {
                for(xml_node* installationNode = installationsNode->first_node("Installation"); installationNode; installationNode = installationNode->next_sibling("Installation"))
                {
                    for(xml_node* buildingsNode = installationNode->first_node("Buildings"); buildingsNode; buildingsNode = buildingsNode->next_sibling("Buildings"))
                    {
                        assignRoomsToDevices(buildingsNode, "", deviceById);
                    }
                }
            }
        }

        for(xml_node* projectNode = rootNode->first_node("Project"); projectNode; projectNode = projectNode->next_sibling("Project"))
        {
            for(xml_node* installationsNode = projectNode->first_node("Installations"); installationsNode; installationsNode = installationsNode->next_sibling("Installations"))
            {
                xml_node* installationNode = installationsNode->first_node("Installation");
                if(!installationNode) GD::out.printWarning("Warning: No element \"Installation\" found.");
                for(xml_node* installationNode = installationsNode->first_node("Installation"); installationNode; installationNode = installationNode->next_sibling("Installation"))
                {
                    xml_node* groupAdressesNode = installationNode->first_node("GroupAddresses");
                    if(!groupAdressesNode)
                    {
                        xml_node* topologyNode = installationNode->first_node("Topology");
                        if(topologyNode) groupAdressesNode = topologyNode->first_node("GroupAddresses");
                    }
                    if(!groupAdressesNode) GD::out.printWarning("Warning: No element \"GroupAddresses\" found.");
                    for(; groupAdressesNode; groupAdressesNode = groupAdressesNode->next_sibling("GroupAddresses"))
                    {
                        xml_node* groupRangesNode = groupAdressesNode->first_node("GroupRanges");
                        if(!groupRangesNode) GD::out.printWarning("Warning: No element \"GroupRanges\" found.");
                        for(xml_node* groupRangesNode = groupAdressesNode->first_node("GroupRanges"); groupRangesNode; groupRangesNode = groupRangesNode->next_sibling("GroupRanges"))
                        {
                            xml_node* mainGroupNode = groupRangesNode->first_node("GroupRange");
                            if(!mainGroupNode) GD::out.printDebug("Debug: No element \"GroupRange\" (1) found.");
                            for(xml_node* mainGroupNode = groupRangesNode->first_node("GroupRange"); mainGroupNode; mainGroupNode = mainGroupNode->next_sibling("GroupRange"))
                            {
                                xml_attribute* attribute = mainGroupNode->first_attribute("Name");
                                if(!attribute)
                                {
                                    GD::out.printWarning("Warning: Main GroupRange has no name.");
                                    continue;
                                }
                                std::string mainGroupName(attribute->value());

                                xml_node* middleGroupNode = mainGroupNode->first_node("GroupRange");
                                if(!middleGroupNode) GD::out.printDebug("Debug: No element \"GroupRange\" (2) found.");
                                for(xml_node* middleGroupNode = mainGroupNode->first_node("GroupRange"); middleGroupNode; middleGroupNode = middleGroupNode->next_sibling("GroupRange"))
                                {
                                    attribute = middleGroupNode->first_attribute("Name");
                                    if(!attribute)
                                    {
                                        GD::out.printWarning("Warning: Middle GroupRange has no name.");
                                        continue;
                                    }
                                    std::string middleGroupName(attribute->value());

                                    xml_node* groupAddressNode = middleGroupNode->first_node("GroupAddress");
                                    if(!groupAddressNode) GD::out.printDebug("Debug: No element \"GroupRange\" (3) found.");
                                    for(xml_node* groupAddressNode = middleGroupNode->first_node("GroupAddress"); groupAddressNode; groupAddressNode = groupAddressNode->next_sibling("GroupAddress"))
                                    {
                                        std::shared_ptr<GroupVariableXmlData> element = std::make_shared<GroupVariableXmlData>();
                                        element->mainGroupName = mainGroupName;
                                        element->middleGroupName = middleGroupName;

                                        attribute = groupAddressNode->first_attribute("Id");
                                        if(!attribute) continue;
                                        std::string id = std::string(attribute->value());
                                        if(id.empty()) continue;

                                        GD::out.printDebug("Debug: Found for group address " + id);

                                        std::string shortId = BaseLib::HelperFunctions::splitLast(id, '_').second;
                                        if(shortId.empty()) continue;

                                        GD::out.printDebug("Debug: Short ID of group address " + id + ": " + shortId);

                                        attribute = groupAddressNode->first_attribute("Name");
                                        if(attribute) element->groupVariableName = std::string(attribute->value());

                                        attribute = groupAddressNode->first_attribute("Address");
                                        if(!attribute) continue;

                                        std::string attributeValue(attribute->value());
                                        element->address = (uint16_t)BaseLib::Math::getNumber(attributeValue);

                                        GD::out.printDebug("Debug: Address of group address with ID " + id + ": " + Cemi::getFormattedGroupAddress(element->address));

                                        //Try to add datapoint type from group variable. If the attribute doesn't exist, we try to get the
                                        //datapoint type from the device further below. So don't throw an error here.
                                        attribute = groupAddressNode->first_attribute("DatapointType");
                                        if(attribute)
                                        {
                                            element->datapointType = std::string(attribute->value());
                                            GD::out.printDebug("Debug: DPT of group address with ID " + id + ": " + element->datapointType);
                                        }

                                        attribute = groupAddressNode->first_attribute("Description");
                                        if(attribute) attributeValue = std::string(attribute->value()); else attributeValue = "";
                                        std::string::size_type jsonStartPos = attributeValue.find("$${");
                                        if(jsonStartPos != std::string::npos)
                                        {
                                            attributeValue = attributeValue.substr(jsonStartPos + 2);
                                            std::string jsonString;
                                            BaseLib::Html::unescapeHtmlEntities(attributeValue, jsonString);
                                            try
                                            {
                                                BaseLib::PVariable json = BaseLib::Rpc::JsonDecoder::decode(jsonString);
                                                element->description = json;
                                            }
                                            catch(const std::exception& ex)
                                            {
                                                _bl->out.printError("Error decoding JSON of group variable \"" + element->groupVariableName + "\": " + ex.what());
                                                continue;
                                            }
                                        }

                                        //{{{ Find Homegear info
                                        if(projectData->groupVariableInfo)
                                        {
                                            auto infoIterator = projectData->groupVariableInfo->structValue->find(Cemi::getFormattedGroupAddress(element->address));
                                            if(infoIterator != projectData->groupVariableInfo->structValue->end())
                                            {
                                                element->homegearInfo = infoIterator->second;
                                            }
                                        }
                                        //}}

                                        //{{{ Assign group variable to device
                                            auto variableIterator = deviceByGroupVariable.find(id);
                                            if(variableIterator == deviceByGroupVariable.end()) variableIterator = deviceByGroupVariable.find(shortId);
                                            if(variableIterator != deviceByGroupVariable.end())
                                            {
                                                for(auto& device : variableIterator->second)
                                                {
                                                    GD::out.printDebug("Debug: Device " + Cemi::getFormattedPhysicalAddress(device->address) + " found for group address " + id);
                                                    auto infoIterator = device->variableInfo.find(id);
                                                    if(infoIterator == device->variableInfo.end()) infoIterator = device->variableInfo.find(shortId);
                                                    if(infoIterator != device->variableInfo.end())
                                                    {
                                                        for(auto& variableInfoElement : infoIterator->second)
                                                        {
                                                            if(element->datapointType.empty() && !variableInfoElement.datapointType.empty())
                                                            {
                                                                element->datapointType = variableInfoElement.datapointType;
                                                                GD::out.printDebug("Debug: DPT of group address with ID " + id + " (assigned from device): " + element->datapointType);
                                                            }

                                                            if(element->datapointType.empty())
                                                            {
                                                                GD::out.printWarning("Warning: Group variable has no datapoint type: " + std::to_string(element->address >> 11) + "/" + std::to_string((element->address >> 8) & 0x7) + "/" + std::to_string(element->address & 0xFF) + ". The group variable does not work.");
                                                                break;
                                                            }

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

                                                            variableInfo->comObjectName = variableInfoElement.name;
                                                            variableInfo->functionText = variableInfoElement.functionText;
                                                            variableInfo->readFlag = variableInfoElement.readFlag;
                                                            variableInfo->writeFlag = variableInfoElement.writeFlag;
                                                            variableInfo->transmitFlag = variableInfoElement.transmitFlag;
                                                            variableInfo->index = variableInfoElement.index;

                                                            device->variables.emplace(variableInfo->index, variableInfo);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        if(element->datapointType.empty())
                                                        {
                                                            GD::out.printWarning("Warning: Group variable has no datapoint type: " + std::to_string(element->address >> 11) + "/" + std::to_string((element->address >> 8) & 0x7) + "/" + std::to_string(element->address & 0xFF) + ". The group variable does not work.");
                                                            break;
                                                        }

                                                        std::shared_ptr<GroupVariableXmlData> variableInfo = std::make_shared<GroupVariableXmlData>();
                                                        *variableInfo = *element;
                                                        variableInfo->autocreated = true;
                                                        device->variables.emplace(variableInfo->index, variableInfo);
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                GD::out.printDebug("Debug: No device found for group address " + id);
                                            }
                                        //}}}

                                        if(element->datapointType.empty())
                                        {
                                            GD::out.printWarning("Warning: Group variable has no datapoint type: " + std::to_string(element->address >> 11) + "/" + std::to_string((element->address >> 8) & 0x7) + "/" + std::to_string(element->address & 0xFF) + ". The group variable does not work.");
                                            continue;
                                        }

                                        xmlData.groupVariableXmlData.emplace(element);
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
    doc.clear();
}

void Search::parseDatapointType(PFunction& function, std::string& datapointType, PParameter& parameter)
{
    try
    {
        auto result = DpstParser::parse(function, datapointType, parameter);
        if(!result)
        {
            _bl->out.printWarning("Warning: Unknown datapoint type: " + datapointType);
        }
    }
    catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

}
