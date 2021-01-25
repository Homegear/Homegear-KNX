/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst22Parser.h"
#include "../Gd.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst22Parser::parse(BaseLib::SharedObjects *bl,
                         const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                         const std::string &datapointType,
                         uint32_t datapointSubtype,
                         std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(Gd::bl));
  parameter->logical = logical;
  logical->minimumValue = 0;
  logical->maximumValue = 65535;
  cast->type = "DPT-22";

  //DHW controller status
  if (datapointType == "DPST-22-100") {
    logical->maximumValue = 255;

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
                                                     parameter->roles,
                                                     parameter->physical->address,
                                                     -1,
                                                     std::make_shared<BaseLib::DeviceDescription::LogicalAction>(Gd::bl)));

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".TEMP_OPTIM_SHIFT_ACTIVE",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   8,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".SOLAR_ENERGY_SUPPORT",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   9,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".SOLAR_ENERGY_ONLY",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   10,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".OTHER_ENERGY_SOURCE_ACTIVE",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   11,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".DHW_PUSH_ACTIVE",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   12,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".LEGIO_PROT_ACTIVE",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   13,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".DHW_LOAD_ACTIVE",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   14,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".FAULT",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   15,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
  }
    //RHCC status
  else if (datapointType == "DPST-22-101") {
    logical->maximumValue = 32767;

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
                                                     parameter->roles,
                                                     parameter->physical->address,
                                                     -1,
                                                     std::make_shared<BaseLib::DeviceDescription::LogicalAction>(Gd::bl)));

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".OVERHEAT_ALARM",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   1,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".FROST_ALARM",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   2,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".DEW_POINT_STATUS",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   3,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".COOLING_DISABLED",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   4,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATUS_PRE_COOL",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   5,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATUS_ECO_C",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   6,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".HEAT_COOL_MODE",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   7,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".HEATING_DISABLED",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   8,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATUS_STOP_OPTIM",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   9,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATUS_START_OPTIM",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   10,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATUS_MORNING_BOOST_H",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   11,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".TEMP_FLOW_RETURN_LIMIT",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   12,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".TEMP_FLOW_LIMIT",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   13,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATUS_ECO_H",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   14,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".FAULT",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   15,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
  }
    //Media
  else if (datapointType == "DPST-22-1000") {
    logical->maximumValue = 63;

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
                                                     parameter->roles,
                                                     parameter->physical->address,
                                                     -1,
                                                     std::make_shared<BaseLib::DeviceDescription::LogicalAction>(Gd::bl)));

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".KNX_IP",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   10,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".RF",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   11,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".PL110",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   13,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".TP1",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   14,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
  }
    //Channel activation for 16 channels
  else if (datapointType == "DPST-22-1010") {
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
                                                     parameter->roles,
                                                     parameter->physical->address,
                                                     -1,
                                                     std::make_shared<BaseLib::DeviceDescription::LogicalAction>(Gd::bl)));

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_1",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   0,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_2",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   1,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_3",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   2,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_4",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   3,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_5",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   4,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_6",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   5,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_7",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   6,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_8",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   7,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_9",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   8,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_10",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   9,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_11",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   10,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_12",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   11,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_13",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   12,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_14",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   13,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_15",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   14,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".STATE_16",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   15,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
  }

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}