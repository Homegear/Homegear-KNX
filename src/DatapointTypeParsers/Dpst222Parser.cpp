/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst222Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst222Parser::parse(BaseLib::SharedObjects *bl,
                          const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                          const std::string &datapointType,
                          uint32_t datapointSubtype,
                          std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger64 logical(new LogicalInteger64(GD::bl));
  parameter->logical = logical;
  logical->minimumValue = 0;
  logical->maximumValue = 281474976710655;
  cast->type = "DPT-222";

  //Room temperature setpoint
  if (datapointType == "DPST-222-100") {
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

    PLogicalDecimal comfort(new LogicalDecimal(GD::bl));
    comfort->minimumValue = -273.0;
    comfort->maximumValue = 670760.0;
    additionalParameters.push_back(createParameter(function, baseName + ".COMFORT_TEMPERATURE", "DPT-9", "°C", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 16, comfort));

    PLogicalDecimal standBy(new LogicalDecimal(GD::bl));
    standBy->minimumValue = -273.0;
    standBy->maximumValue = 670760.0;
    additionalParameters.push_back(createParameter(function, baseName + ".STANDBY_TEMPERATURE", "DPT-9", "°C", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 16, 16, standBy));

    PLogicalDecimal eco(new LogicalDecimal(GD::bl));
    eco->minimumValue = -273.0;
    eco->maximumValue = 670760.0;
    additionalParameters.push_back(createParameter(function, baseName + ".ECO_TEMPERATURE", "DPT-9", "°C", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 32, 16, eco));
  }
    //Room temperature setpoint shift
  else if (datapointType == "DPST-222-101") {
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

    PLogicalDecimal comfort(new LogicalDecimal(GD::bl));
    comfort->minimumValue = -670760.0;
    comfort->maximumValue = 670760.0;
    additionalParameters.push_back(createParameter(function, baseName + ".COMFORT_TEMPERATURE_SHIFT", "DPT-9", "°C", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 16, comfort));

    PLogicalDecimal standBy(new LogicalDecimal(GD::bl));
    standBy->minimumValue = -670760.0;
    standBy->maximumValue = 670760.0;
    additionalParameters.push_back(createParameter(function, baseName + ".STANDBY_TEMPERATURE_SHIFT", "DPT-9", "°C", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 16, 16, standBy));

    PLogicalDecimal eco(new LogicalDecimal(GD::bl));
    eco->minimumValue = -670760.0;
    eco->maximumValue = 670760.0;
    additionalParameters.push_back(createParameter(function, baseName + ".ECO_TEMPERATURE_SHIFT", "DPT-9", "°C", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 32, 16, eco));
  }

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}