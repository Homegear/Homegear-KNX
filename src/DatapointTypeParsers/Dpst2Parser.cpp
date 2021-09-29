/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst2Parser.h"
#include "../Gd.h"

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst2Parser::parse(BaseLib::SharedObjects *bl,
                        const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                        const std::string &datapointType,
                        uint32_t datapointSubtype,
                        std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  cast->type = datapointType;
  PLogicalEnumeration logical(new LogicalEnumeration(bl));
  parameter->logical = logical;
  logical->minimumValue = 0;
  logical->maximumValue = 3;
  logical->values.emplace_back("NoControlFalse", 0);
  logical->values.emplace_back("NoControlTrue", 1);
  logical->values.emplace_back("ControlFalse", 2);
  logical->values.emplace_back("ControlTrue", 3);

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
                                                 parameter->readOnInit,
                                                 parameter->roles,
                                                 6,
                                                 1,
                                                 std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(Gd::bl)));
  additionalParameters.push_back(createParameter(function,
                                                 baseName + ".STATE",
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

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}