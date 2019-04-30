/* Copyright 2013-2019 Homegear GmbH */

#include "GD.h"
#include "Interfaces.h"
#include "Knx.h"
#include "KnxCentral.h"

namespace Knx
{

Knx::Knx(BaseLib::SharedObjects* bl, BaseLib::Systems::IFamilyEventSink* eventHandler) : BaseLib::Systems::DeviceFamily(bl, eventHandler, MY_FAMILY_ID, MY_FAMILY_NAME)
{
	GD::bl = bl;
	GD::family = this;
	GD::out.init(bl);
	GD::out.setPrefix(std::string("Module ") + MY_FAMILY_NAME + ": ");
	GD::out.printDebug("Debug: Loading module...");
	_physicalInterfaces.reset(new Interfaces(bl, _settings->getPhysicalInterfaceSettings()));
}

Knx::~Knx()
{

}

bool Knx::init()
{
	_bl->out.printInfo("Loading XML RPC devices...");
	std::string xmlPath = _bl->settings.familyDataPath() + std::to_string(GD::family->getFamily()) + "/desc/";
	BaseLib::Io io;
    io.init(_bl);
	if(BaseLib::Io::directoryExists(xmlPath) && !io.getFiles(xmlPath).empty()) _rpcDevices->load(xmlPath);
	return true;
}

void Knx::dispose()
{
	if(_disposed) return;
	DeviceFamily::dispose();

	_central.reset();
}

void Knx::reloadRpcDevices()
{
	_bl->out.printInfo("Reloading XML RPC devices...");
	std::string xmlPath = _bl->settings.familyDataPath() + std::to_string(GD::family->getFamily()) + "/desc/";
	if(BaseLib::Io::directoryExists(xmlPath)) _rpcDevices->load(xmlPath);
}

void Knx::createCentral()
{
	try
	{
		_central.reset(new KnxCentral(0, "VBF0000001", this));
		GD::out.printMessage("Created central with id " + std::to_string(_central->getId()) + ".");
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

std::shared_ptr<BaseLib::Systems::ICentral> Knx::initializeCentral(uint32_t deviceId, int32_t address, std::string serialNumber)
{
	return std::shared_ptr<KnxCentral>(new KnxCentral(deviceId, serialNumber, this));
}

PVariable Knx::getPairingInfo()
{
	try
	{
		if(!_central) return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
		PVariable info = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

		//{{{ General
		info->structValue->emplace("searchInterfaces", std::make_shared<BaseLib::Variable>(false));
		info->structValue->emplace("deviceInfoFileUpload", std::make_shared<BaseLib::Variable>(true));
        info->structValue->emplace("deviceInfoFileSuffix", std::make_shared<BaseLib::Variable>(std::string(".knxproj")));
		//}}}

		//{{{ Pairing methods
		PVariable pairingMethods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
		pairingMethods->structValue->emplace("searchDevices", std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
        info->structValue->emplace("pairingMethods", pairingMethods);
		//}}}

		//{{{ interfaces
		auto interfaces = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

		//{{{ KNXnet/IP
		auto interface = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
		interface->structValue->emplace("name", std::make_shared<BaseLib::Variable>(std::string("KNXnet/IP")));
		interface->structValue->emplace("ipDevice", std::make_shared<BaseLib::Variable>(true));

		auto field = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
		field->structValue->emplace("pos", std::make_shared<BaseLib::Variable>(0));
		field->structValue->emplace("label", std::make_shared<BaseLib::Variable>(std::string("l10n.common.id")));
		field->structValue->emplace("type", std::make_shared<BaseLib::Variable>(std::string("string")));
		interface->structValue->emplace("id", field);

		field = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
		field->structValue->emplace("pos", std::make_shared<BaseLib::Variable>(1));
		field->structValue->emplace("label", std::make_shared<BaseLib::Variable>(std::string("l10n.common.hostname")));
		field->structValue->emplace("type", std::make_shared<BaseLib::Variable>(std::string("string")));
		interface->structValue->emplace("host", field);

		field = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
		field->structValue->emplace("pos", std::make_shared<BaseLib::Variable>(2));
		field->structValue->emplace("label", std::make_shared<BaseLib::Variable>(std::string("l10n.common.listenip")));
		field->structValue->emplace("type", std::make_shared<BaseLib::Variable>(std::string("string")));
        field->structValue->emplace("required", std::make_shared<BaseLib::Variable>(false));
		interface->structValue->emplace("listenIp", field);

		field = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
		field->structValue->emplace("type", std::make_shared<BaseLib::Variable>(std::string("string")));
		field->structValue->emplace("const", std::make_shared<BaseLib::Variable>(std::string("3671")));
		interface->structValue->emplace("port", field);

		interfaces->structValue->emplace("knxnetip", interface);
		//}}}

		info->structValue->emplace("interfaces", interfaces);
		//}}}

		return info;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return Variable::createError(-32500, "Unknown application error.");
}
}
