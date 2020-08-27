/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst241Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst241Parser::parse(BaseLib::SharedObjects *bl,
                          const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                          const std::string &datapointType,
                          uint32_t datapointSubtype,
                          std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(GD::bl));
  parameter->logical = logical;
  cast->type = "DPT-241";

  if (datapointType == "DPST-241-800") {
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
                                                     std::make_shared<BaseLib::DeviceDescription::LogicalAction>(GD::bl)));

    PLogicalInteger height(new LogicalInteger(GD::bl));
    height->minimumValue = 0;
    height->maximumValue = 100;
    additionalParameters.push_back(createParameter(function, baseName + ".HEIGHT_POSITION", "DPST-5-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 8, height));

    PLogicalInteger slats(new LogicalInteger(GD::bl));
    slats->minimumValue = 0;
    slats->maximumValue = 100;
    additionalParameters.push_back(createParameter(function, baseName + ".SLATS_POSITION", "DPST-5-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 8, 8, slats));

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".UPPER_END_REACHED",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   16,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".LOWER_END_REACHED",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   17,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".LOWER_PREDEF_REACHED",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   18,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".TARGET_REACHED",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   19,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".CANT_REACH_TARGET_POS",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   20,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".CANT_REACH_SLATS_POS",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   21,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".IN_ALARM",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   22,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".UP_DOWN_FORCED_INPUT",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   23,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".LOCKED",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   24,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".OVERRIDDEN",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   25,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".FAILURE",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   26,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".HEIGHT_POS_VALID",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   30,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".SLATS_POS_VALID",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   13,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
  }

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}