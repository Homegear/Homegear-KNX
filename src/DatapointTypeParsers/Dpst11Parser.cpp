/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst11Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst11Parser::parse(BaseLib::SharedObjects *bl,
                         const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                         const std::string &datapointType,
                         uint32_t datapointSubtype,
                         std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(GD::bl));
  parameter->logical = logical;
  logical->minimumValue = 0;
  logical->maximumValue = 16777215;
  cast->type = "DPT-11";

  if (datapointType == "DPST-11-1") {
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

    PLogicalInteger day(new LogicalInteger(GD::bl));
    day->minimumValue = 1;
    day->maximumValue = 31;
    additionalParameters.push_back(createParameter(function, baseName + ".DAY", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 3, 5, day));

    PLogicalInteger month(new LogicalInteger(GD::bl));
    month->minimumValue = 1;
    month->maximumValue = 12;
    additionalParameters.push_back(createParameter(function, baseName + ".MONTH", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 12, 4, month));

    PLogicalInteger year(new LogicalInteger(GD::bl));
    year->minimumValue = 0;
    year->maximumValue = 99;
    additionalParameters.push_back(createParameter(function, baseName + ".YEAR", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 17, 7, year));
  }

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}