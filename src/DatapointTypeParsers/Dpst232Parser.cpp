/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst232Parser.h"
#include "../Gd.h"

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst232Parser::parse(BaseLib::SharedObjects *bl,
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
  cast->type = "DPT-232";

  if (datapointType == "DPST-232-600") {
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

    PLogicalInteger red(new LogicalInteger(Gd::bl));
    red->minimumValue = 0;
    red->maximumValue = 255;
    additionalParameters.push_back(createParameter(function, baseName + ".RED", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 0, 8, red));

    PLogicalInteger green(new LogicalInteger(Gd::bl));
    green->minimumValue = 0;
    green->maximumValue = 255;
    additionalParameters.push_back(createParameter(function, baseName + ".GREEN", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 8, 8, green));

    PLogicalInteger blue(new LogicalInteger(Gd::bl));
    blue->minimumValue = 0;
    blue->maximumValue = 255;
    additionalParameters.push_back(createParameter(function, baseName + ".BLUE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 16, 8, blue));
  }

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}