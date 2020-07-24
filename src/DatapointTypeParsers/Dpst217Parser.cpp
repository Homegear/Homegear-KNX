/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst217Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst217Parser::parse(BaseLib::SharedObjects *bl,
                          const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                          const std::string &datapointType,
                          uint32_t datapointSubtype,
                          std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(GD::bl));
  parameter->logical = logical;
  logical->minimumValue = 0;
  logical->maximumValue = 65535;
  cast->type = "DPT-217";

  if (datapointType == "DPST-217-1") {
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

    PLogicalInteger magic(new LogicalInteger(GD::bl));
    magic->minimumValue = 0;
    magic->maximumValue = 31;
    additionalParameters.push_back(createParameter(function, baseName + ".MAGIC_NUMBER", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 5, magic));

    PLogicalInteger version(new LogicalInteger(GD::bl));
    version->minimumValue = 0;
    version->maximumValue = 31;
    additionalParameters.push_back(createParameter(function, baseName + ".VERSION_NUMBER", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 5, 5, version));

    PLogicalInteger revision(new LogicalInteger(GD::bl));
    revision->minimumValue = 0;
    revision->maximumValue = 63;
    additionalParameters.push_back(createParameter(function, baseName + ".REVISION_NUMBER", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 10, 6, revision));
  }

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}