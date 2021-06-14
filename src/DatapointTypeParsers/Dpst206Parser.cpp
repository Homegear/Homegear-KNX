/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst206Parser.h"
#include "../Gd.h"

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst206Parser::parse(BaseLib::SharedObjects *bl,
                          const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                          const std::string &datapointType,
                          uint32_t datapointSubtype,
                          std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(Gd::bl));
  parameter->logical = logical;
  logical->minimumValue = 0;
  logical->maximumValue = 16777215;
  cast->type = "DPT-206";

  //Time delay & HVAC mode
  if (datapointType == "DPST-206-100") {
    std::string baseName = parameter->id;
    parameter->id = baseName + ".RAW";
    if (parameter->writeable)
      additionalParameters.push_back(createParameter(function,
                                                     baseName + ".SUBMIT",
                                                     "DPT-1",
                                                     "",
                                                     IPhysical::OperationType::command,
                                                     parameter->readable,
                                                     parameter->writeable,
                                                     parameter->readOnInit,
                                                     parameter->roles,
                                                     parameter->physical->address,
                                                     -1,
                                                     std::make_shared<BaseLib::DeviceDescription::LogicalAction>(Gd::bl)));

    PLogicalInteger delay(new LogicalInteger(Gd::bl));
    delay->minimumValue = 0;
    delay->maximumValue = 65535;
    additionalParameters.push_back(createParameter(function, baseName + ".DELAY", "DPT-7", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 0, 16, delay));

    PLogicalEnumeration hvacMode(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".HVAC_MODE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 16, 8, hvacMode));
    hvacMode->minimumValue = 0;
    hvacMode->maximumValue = 4;
    hvacMode->values.emplace_back("Undefined", 0);
    hvacMode->values.emplace_back("Comfort", 1);
    hvacMode->values.emplace_back("Stand by", 2);
    hvacMode->values.emplace_back("Economy", 3);
    hvacMode->values.emplace_back("Building protection", 4);
  }
    //Time delay & DHW mode
  else if (datapointType == "DPST-206-102") {
    std::string baseName = parameter->id;
    parameter->id = baseName + ".RAW";
    if (parameter->writeable)
      additionalParameters.push_back(createParameter(function,
                                                     baseName + ".SUBMIT",
                                                     "DPT-1",
                                                     "",
                                                     IPhysical::OperationType::command,
                                                     parameter->readable,
                                                     parameter->writeable,
                                                     parameter->readOnInit,
                                                     parameter->roles,
                                                     parameter->physical->address,
                                                     -1,
                                                     std::make_shared<BaseLib::DeviceDescription::LogicalAction>(Gd::bl)));

    PLogicalInteger delay(new LogicalInteger(Gd::bl));
    delay->minimumValue = 0;
    delay->maximumValue = 65535;
    additionalParameters.push_back(createParameter(function, baseName + ".DELAY", "DPT-7", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 0, 16, delay));

    PLogicalEnumeration dhwMode(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".DHW_MODE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 16, 8, dhwMode));
    dhwMode->minimumValue = 0;
    dhwMode->maximumValue = 4;
    dhwMode->values.emplace_back("Undefined", 0);
    dhwMode->values.emplace_back("Legio protect", 1);
    dhwMode->values.emplace_back("Normal", 2);
    dhwMode->values.emplace_back("Reduced", 3);
    dhwMode->values.emplace_back("Off / frost protection", 4);
  }
    //Time delay & occupancy mode
  else if (datapointType == "DPST-206-104") {
    std::string baseName = parameter->id;
    parameter->id = baseName + ".RAW";
    if (parameter->writeable)
      additionalParameters.push_back(createParameter(function,
                                                     baseName + ".SUBMIT",
                                                     "DPT-1",
                                                     "",
                                                     IPhysical::OperationType::command,
                                                     parameter->readable,
                                                     parameter->writeable,
                                                     parameter->readOnInit,
                                                     parameter->roles,
                                                     parameter->physical->address,
                                                     -1,
                                                     std::make_shared<BaseLib::DeviceDescription::LogicalAction>(Gd::bl)));

    PLogicalInteger delay(new LogicalInteger(Gd::bl));
    delay->minimumValue = 0;
    delay->maximumValue = 65535;
    additionalParameters.push_back(createParameter(function, baseName + ".DELAY", "DPT-7", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 0, 16, delay));

    PLogicalEnumeration occupancyMode(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".OCCUPANCY_MODE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 16, 8, occupancyMode));
    occupancyMode->minimumValue = 0;
    occupancyMode->maximumValue = 2;
    occupancyMode->values.emplace_back("Occupied", 0);
    occupancyMode->values.emplace_back("Stand by", 1);
    occupancyMode->values.emplace_back("Not occupied", 2);
  }
    //Time delay & building mode
  else if (datapointType == "DPST-206-105") {
    std::string baseName = parameter->id;
    parameter->id = baseName + ".RAW";
    if (parameter->writeable)
      additionalParameters.push_back(createParameter(function,
                                                     baseName + ".SUBMIT",
                                                     "DPT-1",
                                                     "",
                                                     IPhysical::OperationType::command,
                                                     parameter->readable,
                                                     parameter->writeable,
                                                     parameter->readOnInit,
                                                     parameter->roles,
                                                     parameter->physical->address,
                                                     -1,
                                                     std::make_shared<BaseLib::DeviceDescription::LogicalAction>(Gd::bl)));

    PLogicalInteger delay(new LogicalInteger(Gd::bl));
    delay->minimumValue = 0;
    delay->maximumValue = 65535;
    additionalParameters.push_back(createParameter(function, baseName + ".DELAY", "DPT-7", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 0, 16, delay));

    PLogicalEnumeration buildingMode(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".BUILDING_MODE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 16, 8, buildingMode));
    buildingMode->minimumValue = 0;
    buildingMode->maximumValue = 2;
    buildingMode->values.emplace_back("In use", 0);
    buildingMode->values.emplace_back("Not in use", 1);
    buildingMode->values.emplace_back("Protected", 2);
  }

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}