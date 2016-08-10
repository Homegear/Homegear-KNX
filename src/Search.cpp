/* Copyright 2013-2016 Sathya Laufer
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

#include "Search.h"
#include "GD.h"

namespace MyFamily
{

Search::Search(BaseLib::Obj* baseLib) : _bl(baseLib)
{
}

Search::~Search()
{
}

std::vector<Search::PeerInfo> Search::search()
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

		std::vector<XmlData> xmlData = extractXmlData(knxProjectFiles);
		if(xmlData.empty())
		{
			GD::out.printError("Error: Could not search for KNX devices. No group addresses were found in KNX project file.");
			return peerInfo;
		}

		createDirectories();

		std::map<std::string, PHomegearDevice> rpcDevices;
		for(std::vector<XmlData>::iterator i = xmlData.begin(); i != xmlData.end(); ++i)
		{
			std::string id;
			int32_t type = -1;
			int32_t channel = 1;
			std::string variableName;
			std::string unit;

			if(i->description)
			{
				BaseLib::Struct::iterator structIterator = i->description->structValue->find("id");
				if(structIterator != i->description->structValue->end())
				{
					id = structIterator->second->stringValue;

					structIterator = i->description->structValue->find("type");
					if(structIterator != i->description->structValue->end()) type = structIterator->second->integerValue;
					if(type > 9999999)
					{
						GD::out.printError("Error: Type number too large: " + std::to_string(type));
						continue;
					}

					structIterator = i->description->structValue->find("channel");
					if(structIterator != i->description->structValue->end()) channel = structIterator->second->integerValue;
				}

				structIterator = i->description->structValue->find("variable");
				if(structIterator != i->description->structValue->end()) variableName = _bl->hf.stringReplace(structIterator->second->stringValue, ".", "_");;

				structIterator = i->description->structValue->find("unit");
				if(structIterator != i->description->structValue->end()) unit = structIterator->second->stringValue;
			}

			if(id.empty())
			{
				PHomegearDevice device(new HomegearDevice(_bl, GD::family->getFamily()));
				device->version = 1;
				PSupportedDevice supportedDevice(new SupportedDevice(_bl, device.get()));
				supportedDevice->id = "KNX_" + std::to_string(i->address >> 11) + "_" + std::to_string((i->address >> 8) & 0x7) + "_" + std::to_string(i->address & 0xFF);
				supportedDevice->description = "KNX_" + std::to_string(i->address >> 11) + "/" + std::to_string((i->address >> 8) & 0x7) + "/" + std::to_string(i->address & 0xFF);
				supportedDevice->typeNumber = i->address;
				device->supportedDevices.push_back(supportedDevice);
				rpcDevices[supportedDevice->id] = device;

				createXmlMaintenanceChannel(device);

				PFunction function(new Function(_bl));
				function->channel = 1;
				function->type = "KNX_GROUP_VARIABLE";
				function->variablesId = "knx_values";
				device->functions[function->channel] = function;

				PParameter parameter = createParameter(function, variableName.empty() ? "VALUE" : variableName, i->datapointType, unit, IPhysical::OperationType::command, i->address);
				if(!parameter) continue;

				parseDatapointType(function, i->datapointType, parameter);

				if(!parameter->casts.empty())
				{
					function->variables->parametersOrdered.push_back(parameter);
					function->variables->parameters[parameter->id] = parameter;
				}
			}
			else
			{
				std::shared_ptr<HomegearDevice> device;
				std::map<std::string, PHomegearDevice>::iterator deviceIterator = rpcDevices.find(id);
				if(deviceIterator == rpcDevices.end())
				{
					device.reset(new HomegearDevice(_bl, GD::family->getFamily()));
					device->version = 1;
					PSupportedDevice supportedDevice(new SupportedDevice(_bl, device.get()));
					supportedDevice->id = id;
					supportedDevice->description = supportedDevice->id;
					supportedDevice->typeNumber = type + 65535;
					device->supportedDevices.push_back(supportedDevice);
					rpcDevices[supportedDevice->id] = device;

					createXmlMaintenanceChannel(device);
				}
				else device = deviceIterator->second;

				if(type != -1) device->supportedDevices.at(0)->typeNumber = type + 65535;

				PFunction function;
				Functions::iterator functionIterator = device->functions.find(channel);
				if(functionIterator == device->functions.end())
				{
					function.reset(new Function(_bl));
					function->channel = channel;
					function->type = "KNX_CHANNEL_" + std::to_string(channel);
					function->variablesId = "knx_values_" + std::to_string(channel);
					device->functions[function->channel] = function;
				}
				else function = functionIterator->second;

				PParameter parameter = createParameter(function, variableName.empty() ? "VALUE" : variableName, i->datapointType, unit, IPhysical::OperationType::command, i->address);
				if(!parameter) continue;

				parseDatapointType(function, i->datapointType, parameter);

				if(!parameter->casts.empty())
				{
					function->variables->parametersOrdered.push_back(parameter);
					function->variables->parameters[parameter->id] = parameter;
				}
			}
		}

		for(std::map<std::string, PHomegearDevice>::iterator i = rpcDevices.begin(); i != rpcDevices.end(); ++i)
		{
			std::string filename = _xmlPath + i->second->supportedDevices.at(0)->id + ".xml";
			i->second->save(filename);

			PeerInfo info;
			info.type = i->second->supportedDevices.at(0)->typeNumber;
			std::string paddedType = std::to_string(info.type);
			if(paddedType.size() < 9) paddedType.insert(0, 9 - paddedType.size(), '0');
			info.serialNumber = "KNX" + paddedType;
			peerInfo.push_back(info);
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

		std::string path1 = _bl->settings.dataPath() + "families/";
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
		parameter->logical = PLogicalBoolean(new LogicalBoolean(_bl));;
		parameter->physical = PPhysicalInteger(new PhysicalInteger(_bl));
		parameter->physical->groupId = parameter->id;
		parameter->physical->operationType = IPhysical::OperationType::internal;

		parameter.reset(new Parameter(_bl, function->variables.get()));
		parameter->id = "STICKY_UNREACH";
		function->variables->parametersOrdered.push_back(parameter);
		function->variables->parameters[parameter->id] = parameter;
		parameter->sticky = true;
		parameter->service = true;
		parameter->logical = PLogicalBoolean(new LogicalBoolean(_bl));;
		parameter->physical = PPhysicalInteger(new PhysicalInteger(_bl));
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
		for(std::vector<std::string>::iterator i = files.begin(); i != files.end(); i++)
		{
			if((*i).size() > 8 && (*i).compare((*i).size() - 8, 8, ".knxproj") == 0)
			{
				projectFilenames.push_back(projectPath + *i);
			}
		}
		if(projectFilenames.empty())
		{
			GD::out.printError("Error: Could not find any project archive in " + GD::bl->settings.deviceDescriptionPath() + std::to_string(MY_FAMILY_ID) + "/.");
			return contents;
		}

		for(std::vector<std::string>::iterator i = projectFilenames.begin(); i != projectFilenames.end(); i++)
		{
			std::shared_ptr<std::vector<char>> content(new std::vector<char>());

			int32_t error = 0;
			zip* projectArchive = zip_open((*i).c_str(), 0, &error);
			if(!projectArchive)
			{
				GD::out.printError("Error: Could not open project archive. Error code: " + std::to_string(error));
				continue;
			}

			zip_int64_t filesInArchive = zip_get_num_entries(projectArchive, 0);
			for(int64_t i = 0; i < filesInArchive; ++i)
			{
				struct zip_stat st;
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

std::vector<Search::XmlData> Search::extractXmlData(std::vector<std::shared_ptr<std::vector<char>>>& knxProjectFiles)
{
	std::vector<XmlData> xmlData;
	BaseLib::Rpc::JsonDecoder jsonDecoder(_bl);
	for(std::vector<std::shared_ptr<std::vector<char>>>::iterator i = knxProjectFiles.begin(); i != knxProjectFiles.end(); ++i)
	{
		xml_document<> doc;
		try
		{
			const char* startPos = (char*)memchr(&(*i)->at(0), '<', 10);
			if(!startPos)
			{
				_bl->out.printError("Error: No '<' found in KNX project XML.");
				doc.clear();
				continue;
			}
			doc.parse<parse_no_entity_translation | parse_validate_closing_tags>(&(*i)->at(0));
			xml_node<>* rootNode = doc.first_node("KNX");
			if(!rootNode)
			{
				_bl->out.printError("Error: KNX project XML does not start with \"KNX\".");
				doc.clear();
				continue;
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
											XmlData element;
											element.mainGroupName = mainGroupName;
											element.middleGroupName = middleGroupName;

											attribute = groupAddressNode->first_attribute("Name");
											if(attribute) element.groupVariableName = std::string(attribute->value());

											attribute = groupAddressNode->first_attribute("Address");
											if(!attribute) continue;
											std::string attributeValue(attribute->value());
											element.address = BaseLib::Math::getNumber(attributeValue);

											attribute = groupAddressNode->first_attribute("DatapointType");
											if(!attribute)
											{
												GD::out.printWarning("Warning: Group variable has no datapoint type: " + std::to_string(element.address >> 11) + "/" + std::to_string((element.address >> 8) & 0x7) + "/" + std::to_string(element.address & 0xFF));
												continue;
											}
											element.datapointType = std::string(attribute->value());

											attribute = groupAddressNode->first_attribute("Description");
											if(attribute) attributeValue = std::string(attribute->value()); else attributeValue = "";
											std::string::size_type startPos = attributeValue.find("$${");
											if(startPos != std::string::npos)
											{
												attributeValue = attributeValue.substr(startPos + 2);
												std::string jsonString;
												BaseLib::Html::unescapeHtmlEntities(attributeValue, jsonString);
												try
												{
													BaseLib::PVariable json = jsonDecoder.decode(jsonString);
													element.description = json;
												}
												catch(const Exception& ex)
												{
													_bl->out.printError("Error decoding JSON of group variable \"" + element.groupVariableName + "\": " + ex.what());
												}
											}

											xmlData.push_back(element);
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

PParameter Search::createParameter(PFunction& function, std::string name, std::string metadata, std::string unit, IPhysical::OperationType::Enum operationType, uint16_t address, int32_t size, std::shared_ptr<ILogical> logical, bool noCast)
{
	try
	{
		PParameter parameter(new Parameter(_bl, function->variables.get()));
		parameter->id = name;
		parameter->metadata = metadata;
		parameter->unit = unit;
		if(logical) parameter->logical = logical;
		parameter->physical = PPhysical(new Physical(_bl));
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
			logical->values.push_back(EnumerationValue("NoControlFalse", 0));
			logical->values.push_back(EnumerationValue("NoControlTrue", 1));
			logical->values.push_back(EnumerationValue("ControlFalse", 2));
			logical->values.push_back(EnumerationValue("ControlTrue", 3));

			std::string baseName = parameter->id;
			parameter->id = baseName + ".RAW";
			additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->physical->address, -1, PLogicalAction(new LogicalAction(_bl))));

			additionalParameters.push_back(createParameter(function, baseName + ".CONTROL", "DPT-1", "", IPhysical::OperationType::store, 6, 1, PLogicalBoolean(new LogicalBoolean(_bl))));
			additionalParameters.push_back(createParameter(function, baseName + ".STATE", "DPT-1", "", IPhysical::OperationType::store, 7, 1, PLogicalBoolean(new LogicalBoolean(_bl))));
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
			additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->physical->address, -1, PLogicalAction(new LogicalAction(_bl))));

			additionalParameters.push_back(createParameter(function, baseName + ".CONTROL", "DPT-1", "", IPhysical::OperationType::store, 4, 1, PLogicalBoolean(new LogicalBoolean(_bl))));

			PLogicalInteger stepCode(new LogicalInteger(_bl));
			stepCode->minimumValue = 0;
			stepCode->minimumValue = 7;
			additionalParameters.push_back(createParameter(function, baseName + ".STEP_CODE", "DPT-5", "", IPhysical::OperationType::store, 5, 3, stepCode));
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
				additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->physical->address, -1, PLogicalAction(new LogicalAction(_bl))));

				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_A", "DPT-1", "", IPhysical::OperationType::store, 0, 1, PLogicalBoolean(new LogicalBoolean(_bl))));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_B", "DPT-1", "", IPhysical::OperationType::store, 1, 1, PLogicalBoolean(new LogicalBoolean(_bl))));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_C", "DPT-1", "", IPhysical::OperationType::store, 2, 1, PLogicalBoolean(new LogicalBoolean(_bl))));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_D", "DPT-1", "", IPhysical::OperationType::store, 3, 1, PLogicalBoolean(new LogicalBoolean(_bl))));
				additionalParameters.push_back(createParameter(function, baseName + ".STATUS_E", "DPT-1", "", IPhysical::OperationType::store, 4, 1, PLogicalBoolean(new LogicalBoolean(_bl))));
				PLogicalEnumeration enumeration(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".MODE", "DPT-5", "", IPhysical::OperationType::store, 5, 3, enumeration));
				enumeration->minimumValue = 1;
				enumeration->maximumValue = 4;
				enumeration->values.push_back(EnumerationValue("Mode 0", 1));
				enumeration->values.push_back(EnumerationValue("Mode 1", 2));
				enumeration->values.push_back(EnumerationValue("Mode 2", 4));
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
				additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->physical->address, -1, PLogicalAction(new LogicalAction(_bl))));

				PLogicalEnumeration weekDays(new LogicalEnumeration(_bl));
				additionalParameters.push_back(createParameter(function, baseName + ".DAY", "DPT-5", "", IPhysical::OperationType::store, 0, 3, weekDays));
				weekDays->minimumValue = 0;
				weekDays->maximumValue = 7;
				weekDays->values.push_back(EnumerationValue("No day", 0));
				weekDays->values.push_back(EnumerationValue("Monday", 1));
				weekDays->values.push_back(EnumerationValue("Tuesday", 2));
				weekDays->values.push_back(EnumerationValue("Wednesday", 3));
				weekDays->values.push_back(EnumerationValue("Thursday", 4));
				weekDays->values.push_back(EnumerationValue("Friday", 5));
				weekDays->values.push_back(EnumerationValue("Saturday", 6));
				weekDays->values.push_back(EnumerationValue("Sunday", 7));

				PLogicalInteger hours(new LogicalInteger(_bl));
				hours->minimumValue = 0;
				hours->maximumValue = 23;
				additionalParameters.push_back(createParameter(function, baseName + ".HOURS", "DPT-5", "h", IPhysical::OperationType::store, 3, 5, hours));

				PLogicalInteger minutes(new LogicalInteger(_bl));
				hours->minimumValue = 0;
				hours->maximumValue = 59;
				additionalParameters.push_back(createParameter(function, baseName + ".MINUTES", "DPT-5", "min", IPhysical::OperationType::store, 10, 6, minutes));

				PLogicalInteger seconds(new LogicalInteger(_bl));
				hours->minimumValue = 0;
				hours->maximumValue = 59;
				additionalParameters.push_back(createParameter(function, baseName + ".SECONDS", "DPT-5", "s", IPhysical::OperationType::store, 18, 6, minutes));
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
				logical->values.push_back(EnumerationValue("Autonomous", 0));
				logical->values.push_back(EnumerationValue("Slave", 1));
				logical->values.push_back(EnumerationValue("Master", 2));
			}
			//Building mode
			else if(datapointType == "DPST-20-2")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Building in use", 0));
				logical->values.push_back(EnumerationValue("Building not used", 1));
				logical->values.push_back(EnumerationValue("Building protection", 2));
			}
			//Occupied
			else if(datapointType == "DPST-20-3")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Occupied", 0));
				logical->values.push_back(EnumerationValue("Stand by", 1));
				logical->values.push_back(EnumerationValue("Not occupied", 2));
			}
			//Priority
			else if(datapointType == "DPST-20-4")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.push_back(EnumerationValue("High", 0));
				logical->values.push_back(EnumerationValue("Medium", 1));
				logical->values.push_back(EnumerationValue("Low", 2));
				logical->values.push_back(EnumerationValue("Void", 3));
			}
			//Light application mode
			else if(datapointType == "DPST-20-5")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Normal", 0));
				logical->values.push_back(EnumerationValue("Presence simulation", 1));
				logical->values.push_back(EnumerationValue("Night round", 2));
			}
			//Light application area
			else if(datapointType == "DPST-20-6")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 50;
				logical->values.push_back(EnumerationValue("No fault", 0));
				logical->values.push_back(EnumerationValue("System an functions of common interest", 1));
				logical->values.push_back(EnumerationValue("HVAC general FBs", 10));
				logical->values.push_back(EnumerationValue("HVAC hot water heating", 11));
				logical->values.push_back(EnumerationValue("HVAC direct electrical heating", 12));
				logical->values.push_back(EnumerationValue("HVAC terminal units", 13));
				logical->values.push_back(EnumerationValue("HVAC VAC", 14));
				logical->values.push_back(EnumerationValue("Lighting", 20));
				logical->values.push_back(EnumerationValue("Security", 30));
				logical->values.push_back(EnumerationValue("Load management", 40));
				logical->values.push_back(EnumerationValue("Shutters and blinds", 50));
			}
			//Alarm class
			else if(datapointType == "DPST-20-7")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 3;
				logical->values.push_back(EnumerationValue("Simple alarm", 1));
				logical->values.push_back(EnumerationValue("Basic alarm", 2));
				logical->values.push_back(EnumerationValue("Extended alarm", 3));
			}
			//PSU mode
			else if(datapointType == "DPST-20-8")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Disabled (PSU/DPSU fixed off)", 0));
				logical->values.push_back(EnumerationValue("Enabled (PSU/DPSU fixed on)", 1));
				logical->values.push_back(EnumerationValue("Auto (PSU/DPSU automatic on/off)", 2));
			}
			//System error class
			else if(datapointType == "DPST-20-11")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 18;
				logical->values.push_back(EnumerationValue("No fault", 0));
				logical->values.push_back(EnumerationValue("General device fault (e.g. RAM, EEPROM, UI, watchdog, ...)", 1));
				logical->values.push_back(EnumerationValue("Communication fault", 2));
				logical->values.push_back(EnumerationValue("Configuration fault", 3));
				logical->values.push_back(EnumerationValue("Hardware fault", 4));
				logical->values.push_back(EnumerationValue("Software fault", 5));
				logical->values.push_back(EnumerationValue("Insufficient non volatile memory", 6));
				logical->values.push_back(EnumerationValue("Insufficient volatile memory", 7));
				logical->values.push_back(EnumerationValue("Memory allocation command with size 0 received", 8));
				logical->values.push_back(EnumerationValue("CRC error", 9));
				logical->values.push_back(EnumerationValue("Watchdog reset detected", 10));
				logical->values.push_back(EnumerationValue("Invalid opcode detected", 11));
				logical->values.push_back(EnumerationValue("General protection fault", 12));
				logical->values.push_back(EnumerationValue("Maximum table length exceeded", 13));
				logical->values.push_back(EnumerationValue("Undefined load command received", 14));
				logical->values.push_back(EnumerationValue("Group address table is not sorted", 15));
				logical->values.push_back(EnumerationValue("Invalid connection number (TSAP)", 16));
				logical->values.push_back(EnumerationValue("Invalid group object number (ASAP)", 17));
				logical->values.push_back(EnumerationValue("Group object type exceeds (PID_MAX_APDU_LENGTH - 2)", 18));
			}
			//HVAC error class
			else if(datapointType == "DPST-20-12")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 4;
				logical->values.push_back(EnumerationValue("No fault", 0));
				logical->values.push_back(EnumerationValue("Sensor fault", 1));
				logical->values.push_back(EnumerationValue("Process fault / controller fault", 2));
				logical->values.push_back(EnumerationValue("Actuator fault", 3));
				logical->values.push_back(EnumerationValue("Other fault", 4));
			}
			//Time delay
			else if(datapointType == "DPST-20-13")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 25;
				logical->values.push_back(EnumerationValue("Not active", 0));
				logical->values.push_back(EnumerationValue("1 s", 1));
				logical->values.push_back(EnumerationValue("2 s", 2));
				logical->values.push_back(EnumerationValue("3 s", 3));
				logical->values.push_back(EnumerationValue("5 s", 4));
				logical->values.push_back(EnumerationValue("10 s", 5));
				logical->values.push_back(EnumerationValue("15 s", 6));
				logical->values.push_back(EnumerationValue("20 s", 7));
				logical->values.push_back(EnumerationValue("30 s", 8));
				logical->values.push_back(EnumerationValue("45 s", 9));
				logical->values.push_back(EnumerationValue("1 min", 10));
				logical->values.push_back(EnumerationValue("1.25 min", 11));
				logical->values.push_back(EnumerationValue("1.5 min", 12));
				logical->values.push_back(EnumerationValue("2 min", 13));
				logical->values.push_back(EnumerationValue("2.5 min", 14));
				logical->values.push_back(EnumerationValue("3 min", 15));
				logical->values.push_back(EnumerationValue("5 min", 16));
				logical->values.push_back(EnumerationValue("15 min", 17));
				logical->values.push_back(EnumerationValue("20 min", 18));
				logical->values.push_back(EnumerationValue("30 min", 19));
				logical->values.push_back(EnumerationValue("1 h", 20));
				logical->values.push_back(EnumerationValue("2 h", 21));
				logical->values.push_back(EnumerationValue("3 h", 22));
				logical->values.push_back(EnumerationValue("5 h", 23));
				logical->values.push_back(EnumerationValue("12 h", 24));
				logical->values.push_back(EnumerationValue("24 h", 25));
			}
			//Wind force scale (0..12)
			else if(datapointType == "DPST-20-14")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 12;
				logical->values.push_back(EnumerationValue("Calm (no wind)", 0));
				logical->values.push_back(EnumerationValue("Light air", 1));
				logical->values.push_back(EnumerationValue("Light breeze", 2));
				logical->values.push_back(EnumerationValue("Gentle breeze", 3));
				logical->values.push_back(EnumerationValue("Moderate breeze", 4));
				logical->values.push_back(EnumerationValue("Fresh breeze", 5));
				logical->values.push_back(EnumerationValue("Strong breeze", 6));
				logical->values.push_back(EnumerationValue("Near gale / moderate gale", 7));
				logical->values.push_back(EnumerationValue("Fresh gale", 8));
				logical->values.push_back(EnumerationValue("Strong gale", 9));
				logical->values.push_back(EnumerationValue("Whole gale / storm", 10));
				logical->values.push_back(EnumerationValue("Violent storm", 11));
				logical->values.push_back(EnumerationValue("Hurricane", 12));
			}
			//Sensor mode
			else if(datapointType == "DPST-20-17")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 4;
				logical->values.push_back(EnumerationValue("Inactive", 0));
				logical->values.push_back(EnumerationValue("Digital input not inverted", 1));
				logical->values.push_back(EnumerationValue("Digital input inverted", 2));
				logical->values.push_back(EnumerationValue("Analog input -&gt; 0% to 100%", 3));
				logical->values.push_back(EnumerationValue("Temperature sensor input", 4));
			}
			//Actuator connect type
			else if(datapointType == "DPST-20-20")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Sensor connection", 1));
				logical->values.push_back(EnumerationValue("Controller connection", 2));
			}
			//Fuel type
			else if(datapointType == "DPST-20-100")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.push_back(EnumerationValue("Auto", 0));
				logical->values.push_back(EnumerationValue("Oil", 1));
				logical->values.push_back(EnumerationValue("Gas", 2));
				logical->values.push_back(EnumerationValue("Solid state fuel", 3));
			}
			//Burner type
			else if(datapointType == "DPST-20-101")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.push_back(EnumerationValue("Reserved", 0));
				logical->values.push_back(EnumerationValue("1 stage", 1));
				logical->values.push_back(EnumerationValue("2 stage", 2));
				logical->values.push_back(EnumerationValue("Modulating", 3));
			}
			//HVAC mode
			else if(datapointType == "DPST-20-102")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 4;
				logical->values.push_back(EnumerationValue("Auto", 0));
				logical->values.push_back(EnumerationValue("Comfort", 1));
				logical->values.push_back(EnumerationValue("Standby", 2));
				logical->values.push_back(EnumerationValue("Economy", 3));
				logical->values.push_back(EnumerationValue("Building protection", 4));
			}
			//DHW mode
			else if(datapointType == "DPST-20-103")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 4;
				logical->values.push_back(EnumerationValue("Auto", 0));
				logical->values.push_back(EnumerationValue("Legio protect", 1));
				logical->values.push_back(EnumerationValue("Normal", 2));
				logical->values.push_back(EnumerationValue("Reduced", 3));
				logical->values.push_back(EnumerationValue("Off / frost protect", 4));
			}
			//Load priority
			else if(datapointType == "DPST-20-104")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("None", 0));
				logical->values.push_back(EnumerationValue("Shift load priority", 1));
				logical->values.push_back(EnumerationValue("Absolut load priority", 2));
			}
			//HVAC control mode
			else if(datapointType == "DPST-20-105")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 20;
				logical->values.push_back(EnumerationValue("Auto", 0));
				logical->values.push_back(EnumerationValue("Heat", 1));
				logical->values.push_back(EnumerationValue("Morning warmup", 2));
				logical->values.push_back(EnumerationValue("Cool", 3));
				logical->values.push_back(EnumerationValue("Night purge", 4));
				logical->values.push_back(EnumerationValue("Precool", 5));
				logical->values.push_back(EnumerationValue("Off", 6));
				logical->values.push_back(EnumerationValue("Test", 7));
				logical->values.push_back(EnumerationValue("Emergency heat", 8));
				logical->values.push_back(EnumerationValue("Fan only", 9));
				logical->values.push_back(EnumerationValue("Free cool", 10));
				logical->values.push_back(EnumerationValue("Ice", 11));
				logical->values.push_back(EnumerationValue("Maximum heating mode", 12));
				logical->values.push_back(EnumerationValue("Economic heat/cool mode", 13));
				logical->values.push_back(EnumerationValue("Dehumidification", 14));
				logical->values.push_back(EnumerationValue("Calibration mode", 15));
				logical->values.push_back(EnumerationValue("Emergency cool mode", 16));
				logical->values.push_back(EnumerationValue("Emergency steam mode", 17));
				logical->values.push_back(EnumerationValue("NoDem", 20));
			}
			//HVAC emergency mode
			else if(datapointType == "DPST-20-106")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 5;
				logical->values.push_back(EnumerationValue("Normal", 0));
				logical->values.push_back(EnumerationValue("EmergPressure", 1));
				logical->values.push_back(EnumerationValue("EmergDepressure", 2));
				logical->values.push_back(EnumerationValue("EmergPurge", 3));
				logical->values.push_back(EnumerationValue("EmergShutdown", 4));
				logical->values.push_back(EnumerationValue("EmergFire", 5));
			}
			//Changeover mode
			else if(datapointType == "DPST-20-107")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Auto", 0));
				logical->values.push_back(EnumerationValue("Cooling only", 1));
				logical->values.push_back(EnumerationValue("Heating only", 2));
			}
			//Valve mode
			else if(datapointType == "DPST-20-108")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 5;
				logical->values.push_back(EnumerationValue("Heat stage A for normal heating", 1));
				logical->values.push_back(EnumerationValue("Heat stage B for heating with two stages (A + B)", 2));
				logical->values.push_back(EnumerationValue("Cool stage A for normal cooling", 3));
				logical->values.push_back(EnumerationValue("Cool stage B for cooling with two stages (A + B)", 4));
				logical->values.push_back(EnumerationValue("Heat/Cool for changeover applications", 5));
			}
			//Damper mode
			else if(datapointType == "DPST-20-109")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 4;
				logical->values.push_back(EnumerationValue("Fresh air, e.g. for fancoils", 1));
				logical->values.push_back(EnumerationValue("Supply air, e.g. for VAV", 2));
				logical->values.push_back(EnumerationValue("Extract air, e.g. for VAV", 3));
				logical->values.push_back(EnumerationValue("Extract air, e.g. for VAV", 4));
			}
			//Heater mode
			else if(datapointType == "DPST-20-110")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 3;
				logical->values.push_back(EnumerationValue("Heat stage A on/off", 1));
				logical->values.push_back(EnumerationValue("Heat stage A proportional", 2));
				logical->values.push_back(EnumerationValue("Heat stage B proportional", 3));
			}
			//Fan mode
			else if(datapointType == "DPST-20-111")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Nor running", 0));
				logical->values.push_back(EnumerationValue("Permanently running", 1));
				logical->values.push_back(EnumerationValue("Running in intervals", 2));
			}
			//Master/slave mode
			else if(datapointType == "DPST-20-112")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Autonomous", 0));
				logical->values.push_back(EnumerationValue("Master", 1));
				logical->values.push_back(EnumerationValue("Slave", 2));
			}
			//Status room setpoint
			else if(datapointType == "DPST-20-113")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Normal setpoint", 0));
				logical->values.push_back(EnumerationValue("Alternative setpoint", 1));
				logical->values.push_back(EnumerationValue("Building protection setpoint", 2));
			}
			//Metering device type
			else if(datapointType == "DPST-20-114")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 255;
				logical->values.push_back(EnumerationValue("Other device", 0));
				logical->values.push_back(EnumerationValue("Oil meter", 1));
				logical->values.push_back(EnumerationValue("Electricity meter", 2));
				logical->values.push_back(EnumerationValue("Gas device", 3));
				logical->values.push_back(EnumerationValue("Heat meter", 4));
				logical->values.push_back(EnumerationValue("Steam meter", 5));
				logical->values.push_back(EnumerationValue("Warm water meter", 6));
				logical->values.push_back(EnumerationValue("Water meter", 7));
				logical->values.push_back(EnumerationValue("Heat cost allocator", 8));
				logical->values.push_back(EnumerationValue("Reserved", 9));
				logical->values.push_back(EnumerationValue("Cooling load meter (outlet)", 10));
				logical->values.push_back(EnumerationValue("Cooling load meter (inlet)", 11));
				logical->values.push_back(EnumerationValue("Heat (inlet)", 12));
				logical->values.push_back(EnumerationValue("Heat and cool", 13));
				logical->values.push_back(EnumerationValue("Breaker (electricity)", 32));
				logical->values.push_back(EnumerationValue("Valve (gas or water)", 33));
				logical->values.push_back(EnumerationValue("Waste water meter", 40));
				logical->values.push_back(EnumerationValue("Garbage", 41));
				logical->values.push_back(EnumerationValue("Void device type", 255));
			}
			//ADA type
			else if(datapointType == "DPST-20-120")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Air damper", 1));
				logical->values.push_back(EnumerationValue("VAV", 2));
			}
			//Backup mode
			else if(datapointType == "DPST-20-121")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 1;
				logical->values.push_back(EnumerationValue("Backup value", 0));
				logical->values.push_back(EnumerationValue("Keep last state", 1));
			}
			//Start synchronization type
			else if(datapointType == "DPST-20-122")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Position unchanged", 0));
				logical->values.push_back(EnumerationValue("Single close", 1));
				logical->values.push_back(EnumerationValue("Single open", 2));
			}
			//Behavior lock/unlock
			else if(datapointType == "DPST-20-600")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 6;
				logical->values.push_back(EnumerationValue("Off", 0));
				logical->values.push_back(EnumerationValue("On", 1));
				logical->values.push_back(EnumerationValue("No change", 2));
				logical->values.push_back(EnumerationValue("Value according to additional parameter", 3));
				logical->values.push_back(EnumerationValue("Memory function value", 4));
				logical->values.push_back(EnumerationValue("Updated value", 5));
				logical->values.push_back(EnumerationValue("Value before locking", 6));
			}
			//Behavior bus power up/down
			else if(datapointType == "DPST-20-601")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 4;
				logical->values.push_back(EnumerationValue("Off", 0));
				logical->values.push_back(EnumerationValue("On", 1));
				logical->values.push_back(EnumerationValue("No change", 2));
				logical->values.push_back(EnumerationValue("Value according to additional parameter", 3));
				logical->values.push_back(EnumerationValue("Last (value before bus power down)", 4));
			}
			//Dali fade time
			else if(datapointType == "DPST-20-602")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 15;
				logical->values.push_back(EnumerationValue("0 s (disabled)", 0));
				logical->values.push_back(EnumerationValue("0.7 s", 1));
				logical->values.push_back(EnumerationValue("1.0 s", 2));
				logical->values.push_back(EnumerationValue("1.4 s", 3));
				logical->values.push_back(EnumerationValue("2.0 s", 4));
				logical->values.push_back(EnumerationValue("2.8 s", 5));
				logical->values.push_back(EnumerationValue("4.0 s", 6));
				logical->values.push_back(EnumerationValue("5.7 s", 7));
				logical->values.push_back(EnumerationValue("8.0 s", 8));
				logical->values.push_back(EnumerationValue("11.3 s", 9));
				logical->values.push_back(EnumerationValue("16.0 s", 10));
				logical->values.push_back(EnumerationValue("22.6 s", 11));
				logical->values.push_back(EnumerationValue("32.0 s", 12));
				logical->values.push_back(EnumerationValue("45.3 s", 13));
				logical->values.push_back(EnumerationValue("64.0 s", 14));
				logical->values.push_back(EnumerationValue("90.5 s", 15));
			}
			//Blink mode
			else if(datapointType == "DPST-20-603")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("BlinkingDisabled", 0));
				logical->values.push_back(EnumerationValue("WithoutAcknowledge", 1));
				logical->values.push_back(EnumerationValue("BlinkingWithAcknowledge", 2));
			}
			//Light control mode
			else if(datapointType == "DPST-20-604")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 1;
				logical->values.push_back(EnumerationValue("Automatic light control", 0));
				logical->values.push_back(EnumerationValue("Manual light control", 1));
			}
			//PB switch mode
			else if(datapointType == "DPST-20-605")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("One PB/binary input mode", 1));
				logical->values.push_back(EnumerationValue("Two PBs/binary inputs mode", 2));
			}
			//PB action mode
			else if(datapointType == "DPST-20-606")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.push_back(EnumerationValue("Inactive (no message sent)", 0));
				logical->values.push_back(EnumerationValue("SwitchOff message sent", 1));
				logical->values.push_back(EnumerationValue("SwitchOn message sent", 2));
				logical->values.push_back(EnumerationValue("Inverse value of InfoOnOff is sent", 3));
			}
			//PB dimm mode
			else if(datapointType == "DPST-20-607")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 4;
				logical->values.push_back(EnumerationValue("One PB/binary input; SwitchOnOff inverts on each transmission", 1));
				logical->values.push_back(EnumerationValue("One PB/binary input, On / DimUp message sent", 2));
				logical->values.push_back(EnumerationValue("One PB/binary input, Off / DimDown message sent", 3));
				logical->values.push_back(EnumerationValue("Two PBs/binary inputs mode", 4));
			}
			//Switch on mode
			else if(datapointType == "DPST-20-608")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Last actual value", 0));
				logical->values.push_back(EnumerationValue("Value according to additional parameter", 1));
				logical->values.push_back(EnumerationValue("Last received absolute set value", 2));
			}
			//Load type
			else if(datapointType == "DPST-20-609")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Automatic", 0));
				logical->values.push_back(EnumerationValue("Leading edge (inductive load)", 1));
				logical->values.push_back(EnumerationValue("Trailing edge (capacitive load)", 2));
			}
			//Load type detection
			else if(datapointType == "DPST-20-610")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.push_back(EnumerationValue("Undefined", 0));
				logical->values.push_back(EnumerationValue("Leading edge (inductive load)", 1));
				logical->values.push_back(EnumerationValue("Trailing edge (capacitive load)", 2));
				logical->values.push_back(EnumerationValue("Detection not possible or error", 3));
			}
			//SAB except behavior
			else if(datapointType == "DPST-20-801")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 4;
				logical->values.push_back(EnumerationValue("Up", 0));
				logical->values.push_back(EnumerationValue("Down", 1));
				logical->values.push_back(EnumerationValue("No change", 2));
				logical->values.push_back(EnumerationValue("Value according to additional parameter", 3));
				logical->values.push_back(EnumerationValue("Stop", 4));
			}
			//SAB behavior on lock/unlock
			else if(datapointType == "DPST-20-802")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 6;
				logical->values.push_back(EnumerationValue("Up", 0));
				logical->values.push_back(EnumerationValue("Down", 1));
				logical->values.push_back(EnumerationValue("No change", 2));
				logical->values.push_back(EnumerationValue("Value according to additional parameter", 3));
				logical->values.push_back(EnumerationValue("Stop", 4));
				logical->values.push_back(EnumerationValue("Updated value", 5));
				logical->values.push_back(EnumerationValue("Value before locking", 6));
			}
			//SSSB mode
			else if(datapointType == "DPST-20-803")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 4;
				logical->values.push_back(EnumerationValue("One push button/binary input", 1));
				logical->values.push_back(EnumerationValue("One push button/binary input, MoveUp / StepUp message sent", 2));
				logical->values.push_back(EnumerationValue("One push button/binary input, MoveDown / StepDown message sent", 3));
				logical->values.push_back(EnumerationValue("Two push buttons/binary inputs mode", 4));
			}
			//Blinds control mode
			else if(datapointType == "DPST-20-804")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 1;
				logical->values.push_back(EnumerationValue("Automatic control", 0));
				logical->values.push_back(EnumerationValue("Manual control", 1));
			}
			//Communication mode
			else if(datapointType == "DPST-20-1000")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 255;
				logical->values.push_back(EnumerationValue("Data link layer", 0));
				logical->values.push_back(EnumerationValue("Data link layer bus monitor", 1));
				logical->values.push_back(EnumerationValue("Data link layer raw frames", 2));
				logical->values.push_back(EnumerationValue("cEMI transport layer", 6));
				logical->values.push_back(EnumerationValue("No layer", 255));
			}
			else if(datapointType == "DPST-20-1001")
			{
				logical->minimumValue = 1;
				logical->maximumValue = 7;
				logical->values.push_back(EnumerationValue("PL medium domain address", 1));
				logical->values.push_back(EnumerationValue("RF control octet and serial number or DoA", 2));
				logical->values.push_back(EnumerationValue("Bus monitor error flags", 3));
				logical->values.push_back(EnumerationValue("Relative timestamp", 4));
				logical->values.push_back(EnumerationValue("Time delay", 5));
				logical->values.push_back(EnumerationValue("Extended relative timestamp", 6));
				logical->values.push_back(EnumerationValue("BiBat information", 7));
			}
			else if(datapointType == "DPST-20-1002")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("Asynchronous", 0));
				logical->values.push_back(EnumerationValue("Asynchronous + BiBat master", 1));
				logical->values.push_back(EnumerationValue("Asynchronous + BiBat slave", 2));
			}
			else if(datapointType == "DPST-20-1003")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.push_back(EnumerationValue("No filtering, all supported received frames shall be passed to the cEMI client using L_Data.ind", 0));
				logical->values.push_back(EnumerationValue("Filtering by domain address", 1));
				logical->values.push_back(EnumerationValue("Filtering by KNX serial number table", 2));
				logical->values.push_back(EnumerationValue("Filtering by domain address and by serial number", 3));
			}
			else
			{
				PLogicalInteger logical(new LogicalInteger(_bl));
				parameter->logical = logical;
				logical->minimumValue = 0;
				logical->maximumValue = 255;
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
		}
		//16-bit set
		else if(datapointType == "DPT-22" || datapointType.compare(0, 8, "DPST-22-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 65535;
			cast->type = "DPT-22";
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
				logical->values.push_back(EnumerationValue("Off", 0));
				logical->values.push_back(EnumerationValue("On", 1));
				logical->values.push_back(EnumerationValue("Off/on", 2));
				logical->values.push_back(EnumerationValue("On/off", 3));
			}
			//Alarm reaction
			else if(datapointType == "DPST-23-2")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 2;
				logical->values.push_back(EnumerationValue("No alarm is used", 0));
				logical->values.push_back(EnumerationValue("Alarm position is UP", 1));
				logical->values.push_back(EnumerationValue("Alarm position is DOWN", 2));
			}
			//Up/down action
			else if(datapointType == "DPST-23-3")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.push_back(EnumerationValue("Up", 0));
				logical->values.push_back(EnumerationValue("Down", 1));
				logical->values.push_back(EnumerationValue("UpDown", 2));
				logical->values.push_back(EnumerationValue("DownUp", 3));
			}
			//HVAC push button action
			else if(datapointType == "DPST-23-102")
			{
				logical->minimumValue = 0;
				logical->maximumValue = 3;
				logical->values.push_back(EnumerationValue("Comfort/Economy", 0));
				logical->values.push_back(EnumerationValue("Comfort/Nothing", 1));
				logical->values.push_back(EnumerationValue("Economy/Nothing", 2));
				logical->values.push_back(EnumerationValue("Building prot/Auto", 3));
			}
			else
			{
				PLogicalInteger logical(new LogicalInteger(_bl));
				parameter->logical = logical;
				logical->minimumValue = 0;
				logical->maximumValue = 3;
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
		}
		//8-bit set
		else if(datapointType == "DPT-26" || datapointType.compare(0, 8, "DPST-26-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 255;
			cast->type = "DPT-26";
		}
		//32-bit set
		else if(datapointType == "DPT-27" || datapointType.compare(0, 8, "DPST-27-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			cast->type = "DPT-27";
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
		}
		//16-bit unsigned value and 8-bit enumeration
		if(datapointType == "DPT-206" || datapointType.compare(0, 9, "DPST-206-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 16777215;
			cast->type = "DPT-206";
		}
		//Datapoint type version
		else if(datapointType == "DPT-217" || datapointType.compare(0, 9, "DPST-217-") == 0)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			parameter->logical = logical;
			logical->minimumValue = 0;
			logical->maximumValue = 65535;
			cast->type = "DPT-217";
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
		}

		for(std::vector<PParameter>::iterator i = additionalParameters.begin(); i != additionalParameters.end(); ++i)
		{
			if(!*i) continue;
			function->variables->parametersOrdered.push_back(*i);
			function->variables->parameters[(*i)->id] = *i;
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
