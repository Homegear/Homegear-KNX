/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst244Parser.h"
#include "../Gd.h"

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst244Parser::parse(BaseLib::SharedObjects *bl,
                          const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                          const std::string &datapointType,
                          uint32_t datapointSubtype,
                          std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(Gd::bl));
  parameter->logical = logical;
  cast->type = "DPT-244";

  if (datapointType == "DPST-244-600") {
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

    PLogicalEnumeration converterMode(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".CONVERTER_MODE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 0, 4, converterMode));
    converterMode->minimumValue = 0;
    converterMode->maximumValue = 9;
    converterMode->values.emplace_back("Unknown", 0);
    converterMode->values.emplace_back("Normal mode active, all OK", 1);
    converterMode->values.emplace_back("Inhibit mode active", 2);
    converterMode->values.emplace_back("Hardwired inhibit mode active", 3);
    converterMode->values.emplace_back("Rest mode active", 4);
    converterMode->values.emplace_back("Emergency mode active", 5);
    converterMode->values.emplace_back("Extended emergency mode active", 6);
    converterMode->values.emplace_back("FT in progress", 7);
    converterMode->values.emplace_back("DT in progress", 8);
    converterMode->values.emplace_back("PDT in progress", 9);

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".HARDWIRED_SWITCH_ACTIVE",
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
                                                   baseName + ".HARDWIRED_INHIBIT_ACTIVE",
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

    PLogicalEnumeration functionTestPending(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".FUNCTION_TEST_PENDING", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 8, 2, functionTestPending));
    functionTestPending->minimumValue = 0;
    functionTestPending->maximumValue = 2;
    functionTestPending->values.emplace_back("Unknown", 0);
    functionTestPending->values.emplace_back("No test pending", 1);
    functionTestPending->values.emplace_back("Test pending", 2);

    PLogicalEnumeration durationTestPending(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".DURATION_TEST_PENDING", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 10, 2, durationTestPending));
    durationTestPending->minimumValue = 0;
    durationTestPending->maximumValue = 2;
    durationTestPending->values.emplace_back("Unknown", 0);
    durationTestPending->values.emplace_back("No test pending", 1);
    durationTestPending->values.emplace_back("Test pending", 2);

    PLogicalEnumeration partialDurationTestPending(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".PARTIAL_DURATION_TEST_PENDING", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 12, 2, partialDurationTestPending));
    partialDurationTestPending->minimumValue = 0;
    partialDurationTestPending->maximumValue = 2;
    partialDurationTestPending->values.emplace_back("Unknown", 0);
    partialDurationTestPending->values.emplace_back("No test pending", 1);
    partialDurationTestPending->values.emplace_back("Test pending", 2);

    PLogicalEnumeration converterFailure(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".CONVERTER_FAILURE", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 14, 2, converterFailure));
    converterFailure->minimumValue = 0;
    converterFailure->maximumValue = 2;
    converterFailure->values.emplace_back("Unknown", 0);
    converterFailure->values.emplace_back("No failure detected", 1);
    converterFailure->values.emplace_back("Failure detected", 2);
  }

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}