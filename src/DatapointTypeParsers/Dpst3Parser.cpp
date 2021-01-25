/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst3Parser.h"
#include "../Gd.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst3Parser::parse(BaseLib::SharedObjects *bl,
                        const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                        const std::string &datapointType,
                        uint32_t datapointSubtype,
                        std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  cast->type = datapointType;
  PLogicalInteger logical(new LogicalInteger(bl));
  parameter->logical = logical;
  logical->minimumValue = 0;
  logical->maximumValue = 15;

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
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalAction>(Gd::bl)));

  additionalParameters.push_back(createParameter(function,
                                                 baseName + ".CONTROL",
                                                 "DPT-1",
                                                 "",
                                                 IPhysical::OperationType::store,
                                                 parameter->readable,
                                                 parameter->writeable,
                                                 parameter->roles,
                                                 4,
                                                 1,
                                                 std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));

  PLogicalInteger stepCode(new LogicalInteger(bl));
  stepCode->minimumValue = 0;
  stepCode->minimumValue = 7;
  additionalParameters.push_back(createParameter(function, baseName + ".STEP_CODE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 5, 3, stepCode));

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}