/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst250Parser.h"
#include "../Gd.h"

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst250Parser::parse(BaseLib::SharedObjects *bl,
                          const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                          const std::string &datapointType,
                          uint32_t datapointSubtype,
                          std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(Gd::bl));
  parameter->logical = logical;
  cast->type = "DPT-250";

  if (datapointType == "DPST-250-600") {
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

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".CCT_INCREASE",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->readOnInit,
                                                   parameter->roles,
                                                   4,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));

    PLogicalInteger cctStep(new LogicalInteger(Gd::bl));
    cctStep->minimumValue = 1;
    cctStep->maximumValue = 7;
    additionalParameters.push_back(createParameter(function, baseName + ".CCT_STEP", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 5, 3, cctStep));

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".CB_INCREASE",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->readOnInit,
                                                   parameter->roles,
                                                   12,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));

    PLogicalInteger cbStep(new LogicalInteger(Gd::bl));
    cbStep->minimumValue = 1;
    cbStep->maximumValue = 7;
    additionalParameters.push_back(createParameter(function, baseName + ".CB_STEP", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 13, 3, cbStep));

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".CCT_VALID",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->readOnInit,
                                                   parameter->roles,
                                                   22,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".CB_VALID",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->readOnInit,
                                                   parameter->roles,
                                                   23,
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