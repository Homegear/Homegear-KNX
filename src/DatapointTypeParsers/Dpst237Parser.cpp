/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst237Parser.h"
#include "../Gd.h"

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst237Parser::parse(BaseLib::SharedObjects *bl,
                          const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                          const std::string &datapointType,
                          uint32_t datapointSubtype,
                          std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(Gd::bl));
  parameter->logical = logical;
  logical->minimumValue = 0;
  logical->maximumValue = 2047;
  cast->type = "DPT-237";

  //DPT DALI control gear diagnostic
  if (datapointType == "DPST-237-600") {
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
                                                   baseName + ".CONVERTER_ERROR",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->readOnInit,
                                                   parameter->roles,
                                                   5,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".BALLAST_FAILURE",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->readOnInit,
                                                   parameter->roles,
                                                   6,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".LAMP_FAILURE",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->readOnInit,
                                                   parameter->roles,
                                                   7,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".READ",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->readOnInit,
                                                   parameter->roles,
                                                   8,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".GROUP_ADDRESS",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->readOnInit,
                                                   parameter->roles,
                                                   9,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));

    PLogicalInteger red(new LogicalInteger(Gd::bl));
    red->minimumValue = 0;
    red->maximumValue = 63;
    additionalParameters.push_back(createParameter(function, baseName + ".DALI_ADDRESS", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 10, 6, red));
  }

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}