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
		int32_t type = -1;
	};

	Search(BaseLib::SharedObjects* baseLib);
	virtual ~Search();

	std::vector<PeerInfo> search();
protected:
	struct XmlData
	{
		uint16_t address = 0;
		std::string mainGroupName;
		std::string middleGroupName;
		std::string groupVariableName;
		std::string datapointType;
		BaseLib::PVariable description;
	};

	std::string _xmlPath;
	BaseLib::SharedObjects* _bl = nullptr;

	void createDirectories();
	void createXmlMaintenanceChannel(PHomegearDevice& device);
	void parseDatapointType(PFunction& function, std::string& datapointType, PParameter& parameter);
	PParameter createParameter(PFunction& function, std::string name, std::string metadata, std::string unit, IPhysical::OperationType::Enum operationType, bool readable, bool writeable, uint16_t address, int32_t size = -1, std::shared_ptr<ILogical> logical = std::shared_ptr<ILogical>(), bool noCast = false);
	std::vector<std::shared_ptr<std::vector<char>>> extractKnxProjectFiles();
	std::vector<XmlData> extractXmlData(std::vector<std::shared_ptr<std::vector<char>>>& knxProjectFiles);
};

}

#endif
