/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst251Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst251Parser::parse(BaseLib::SharedObjects *bl,
                          const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                          const std::string &datapointType,
                          uint32_t datapointSubtype,
                          std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(GD::bl));
  parameter->logical = logical;
  cast->type = "DPT-251";

  if (datapointType == "DPST-251-600") {
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
                                                     (uint16_t)parameter->physical->address,
                                                     -1,
                                                     std::make_shared<BaseLib::DeviceDescription::LogicalAction>(GD::bl)));

    PLogicalInteger red(new LogicalInteger(GD::bl));
    red->minimumValue = 0;
    red->maximumValue = 255;
    additionalParameters.push_back(createParameter(function, baseName + ".RED", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 8, 8, red));

    PLogicalInteger green(new LogicalInteger(GD::bl));
    green->minimumValue = 0;
    green->maximumValue = 255;
    additionalParameters.push_back(createParameter(function, baseName + ".RED", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 16, 8, green));

    PLogicalInteger blue(new LogicalInteger(GD::bl));
    blue->minimumValue = 0;
    blue->maximumValue = 255;
    additionalParameters.push_back(createParameter(function, baseName + ".RED", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 24, 8, blue));

    PLogicalInteger white(new LogicalInteger(GD::bl));
    white->minimumValue = 0;
    white->maximumValue = 255;
    additionalParameters.push_back(createParameter(function, baseName + ".RED", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 32, 8, white));

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".RED_VALID",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   44,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".GREEN_VALID",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   45,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".BLUE_VALID",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   46,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".WHITE_VALID",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   47,
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