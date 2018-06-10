/* Copyright 2013-2017 Homegear UG (haftungsbeschränkt) */

#ifndef SEARCH_H_
#define SEARCH_H_

#include "../config.h"
#include <homegear-base/BaseLib.h>

#include <sys/stat.h>
#include <zip.h>

using namespace BaseLib;
using namespace BaseLib::DeviceDescription;

namespace MyFamily
{
class Search
{
public:
	struct PeerInfo
	{
		std::string serialNumber;
		int32_t address = -1;
		int32_t type = -1;
		std::string name;
		std::string room;
	};

	explicit Search(BaseLib::SharedObjects* baseLib);
	virtual ~Search() = default;

	std::vector<PeerInfo> search(std::unordered_set<uint32_t>& usedTypeNumbers, std::unordered_map<std::string, uint32_t>& typeNumberIdMap);
	PeerInfo updateDevice(std::unordered_set<uint32_t>& usedTypeNumbers, std::unordered_map<std::string, uint32_t>& typeNumberIdMap, BaseLib::PVariable deviceInfo);
protected:
	struct GroupVariableXmlData
	{
		uint16_t address = 0;
		std::string mainGroupName;
		std::string middleGroupName;
		std::string groupVariableName;
		std::string datapointType;
		int32_t index = -1;
		bool writeFlag = true;
		bool readFlag = true;
        bool transmitFlag = true;
		bool autocreated = false;
		BaseLib::PVariable description;
	};

	struct GroupVariableInfo
	{
		int32_t index = -1;
		bool writeFlag = true;
		bool readFlag = true;
        bool transmitFlag = true;
	};

	struct DeviceXmlData
	{
		std::string id;
		std::string name;
		std::string room;
		int32_t address;
		BaseLib::PVariable description;
		std::unordered_map<std::string, std::list<GroupVariableInfo>> variableInfo;
		std::unordered_map<uint32_t, std::shared_ptr<GroupVariableXmlData>> variables;
	};

	struct XmlData
	{
		bool jsonExists = false;
		std::set<std::shared_ptr<Search::GroupVariableXmlData>> groupVariableXmlData;
		std::set<std::shared_ptr<Search::DeviceXmlData>> deviceXmlData;
	};

	std::string _xmlPath;
	BaseLib::SharedObjects* _bl = nullptr;

	void createDirectories();
	void createXmlMaintenanceChannel(PHomegearDevice& device);
	void parseDatapointType(PFunction& function, std::string& datapointType, PParameter& parameter);
	PParameter createParameter(PFunction& function, std::string name, std::string metadata, std::string unit, IPhysical::OperationType::Enum operationType, bool readable, bool writeable, uint16_t address, int32_t size = -1, std::shared_ptr<ILogical> logical = std::shared_ptr<ILogical>(), bool noCast = false);
	std::vector<std::shared_ptr<std::vector<char>>> extractKnxProjectFiles();
	void assignRoomsToDevices(xml_node<>* currentNode, std::string currentRoom, std::unordered_map<std::string, std::shared_ptr<DeviceXmlData>>& devices);
	XmlData extractXmlData(std::vector<std::shared_ptr<std::vector<char>>>& knxProjectFiles);
	void addDeviceToPeerInfo(PHomegearDevice& device, int32_t address, std::string name, std::string room, std::vector<PeerInfo>& peerInfo, std::map<int32_t, std::string>& usedTypes);
};

}

#endif
