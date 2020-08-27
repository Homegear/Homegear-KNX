/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst15Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst15Parser::parse(BaseLib::SharedObjects *bl,
                         const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                         const std::string &datapointType,
                         uint32_t datapointSubtype,
                         std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(GD::bl));
  parameter->logical = logical;
  cast->type = "DPT-15";

  if (datapointType == "DPST-15-0") {
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

    PLogicalInteger field1(new LogicalInteger(GD::bl));
    field1->minimumValue = 0;
    field1->maximumValue = 9;
    additionalParameters.push_back(createParameter(function, baseName + ".DATA1", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 4, field1));

    PLogicalInteger field2(new LogicalInteger(GD::bl));
    field2->minimumValue = 0;
    field2->maximumValue = 9;
    additionalParameters.push_back(createParameter(function, baseName + ".DATA2", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 4, 4, field2));

    PLogicalInteger field3(new LogicalInteger(GD::bl));
    field3->minimumValue = 0;
    field3->maximumValue = 9;
    additionalParameters.push_back(createParameter(function, baseName + ".DATA3", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 8, 4, field3));

    PLogicalInteger field4(new LogicalInteger(GD::bl));
    field4->minimumValue = 0;
    field4->maximumValue = 9;
    additionalParameters.push_back(createParameter(function, baseName + ".DATA4", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 12, 4, field4));

    PLogicalInteger field5(new LogicalInteger(GD::bl));
    field5->minimumValue = 0;
    field5->maximumValue = 9;
    additionalParameters.push_back(createParameter(function, baseName + ".DATA5", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 16, 4, field5));

    PLogicalInteger field6(new LogicalInteger(GD::bl));
    field6->minimumValue = 0;
    field6->maximumValue = 9;
    additionalParameters.push_back(createParameter(function, baseName + ".DATA6", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 20, 4, field6));

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".DETECTION_ERROR",
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
                                                   baseName + ".PERMISSION",
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
                                                   baseName + ".READ_DIRECTION",
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
                                                   baseName + ".ENCRYPTION",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   27,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));

    PLogicalInteger codeIndex(new LogicalInteger(GD::bl));
    codeIndex->minimumValue = 0;
    codeIndex->maximumValue = 9;
    additionalParameters.push_back(createParameter(function, baseName + ".CODE_INDEX", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 28, 4, codeIndex));
  }

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}