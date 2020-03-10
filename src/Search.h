/* Copyright 2013-2019 Homegear GmbH */

#ifndef SEARCH_H_
#define SEARCH_H_

#include "../config.h"
#include <homegear-base/BaseLib.h>

using namespace BaseLib;
using namespace BaseLib::DeviceDescription;

namespace Knx
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

	std::vector<PeerInfo> search(std::unordered_set<uint32_t>& usedTypeNumbers, std::unordered_map<std::string, uint32_t>& idTypeNumberMap);
	PeerInfo updateDevice(std::unordered_set<uint32_t>& usedTypeNumbers, std::unordered_map<std::string, uint32_t>& idTypeNumberMap, BaseLib::PVariable deviceInfo);
protected:
    struct ProjectData
    {
        std::string filename;
        std::string projectId;
        std::string projectName;

        std::unordered_map<std::string, std::shared_ptr<std::vector<char>>> xmlFiles;
        std::shared_ptr<std::vector<char>> projectXml;
    };
    typedef std::shared_ptr<ProjectData> PProjectData;

    struct ComObjectData
    {
        bool communicationFlag = false;
        bool readFlag = true;
        bool readOnInitFlag = false;
        bool transmitFlag = false;
        bool updateFlag = false;
        bool writeFlag = true;
    };
    typedef std::shared_ptr<ComObjectData> PComObjectData;

    struct ManufacturerProductData
    {
        std::unordered_map<std::string, PComObjectData> comObjectData;
    };
    typedef std::shared_ptr<ManufacturerProductData> PManufacturerProductData;

    struct ManufacturerData
    {
        std::unordered_map<std::string, std::vector<std::string>> hardware2programRefs;
        std::unordered_map<std::string, PManufacturerProductData> productData;
    };
    typedef std::shared_ptr<ManufacturerData> PManufacturerData;

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
        std::string datapointType;
		bool writeFlag = true;
		bool readFlag = true;
        bool transmitFlag = true;
	};

	struct DeviceXmlData
	{
	    std::string interface;
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
	PParameter createParameter(PFunction& function, std::string name, std::string metadata, std::string unit, IPhysical::OperationType::Enum operationType, bool readable, bool writeable, const std::unordered_map<uint64_t, Role>& roles, uint16_t address, int32_t size = -1, std::shared_ptr<ILogical> logical = std::shared_ptr<ILogical>(), bool noCast = false);
	std::vector<PProjectData> extractKnxProjects();
	void assignRoomsToDevices(xml_node<>* currentNode, std::string currentRoom, std::unordered_map<std::string, std::shared_ptr<DeviceXmlData>>& devices);
    std::unordered_map<std::string, PManufacturerData> extractManufacturerXmlData(PProjectData& projectData);
	XmlData extractXmlData(std::vector<PProjectData>& projectData);
    std::shared_ptr<HomegearDevice> createHomegearDevice(const DeviceXmlData& deviceXml, std::unordered_set<uint32_t>& usedTypeNumbers, std::unordered_map<std::string, uint32_t>& typeNumberIdMap);
	void addDeviceToPeerInfo(PHomegearDevice& device, int32_t address, std::string name, std::string room, std::vector<PeerInfo>& peerInfo, std::map<int32_t, std::string>& usedTypes);
};

}

#endif
