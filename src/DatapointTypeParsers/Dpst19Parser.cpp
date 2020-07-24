/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst19Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst19Parser::parse(BaseLib::SharedObjects *bl,
                         const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                         const std::string &datapointType,
                         uint32_t datapointSubtype,
                         std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger64 logical(new LogicalInteger64(GD::bl));
  parameter->logical = logical;
  cast->type = "DPT-19";
  if (datapointType == "DPST-19-1") {
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

    PLogicalInteger year(new LogicalInteger(GD::bl));
    year->minimumValue = 0;
    year->maximumValue = 255;
    additionalParameters.push_back(createParameter(function, baseName + ".YEAR", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 8, year));

    PLogicalInteger month(new LogicalInteger(GD::bl));
    month->minimumValue = 1;
    month->maximumValue = 12;
    additionalParameters.push_back(createParameter(function, baseName + ".MONTH", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 12, 4, month));

    PLogicalInteger dayOfMonth(new LogicalInteger(GD::bl));
    dayOfMonth->minimumValue = 0;
    dayOfMonth->maximumValue = 31;
    additionalParameters.push_back(createParameter(function, baseName + ".DAY_OF_MONTH", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 19, 5, dayOfMonth));

    PLogicalInteger dayOfWeek(new LogicalInteger(GD::bl));
    dayOfWeek->minimumValue = 0;
    dayOfWeek->maximumValue = 7;
    additionalParameters.push_back(createParameter(function, baseName + ".DAY_OF_WEEK", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 24, 3, dayOfWeek));

    PLogicalInteger hours(new LogicalInteger(GD::bl));
    hours->minimumValue = 0;
    hours->maximumValue = 24;
    additionalParameters.push_back(createParameter(function, baseName + ".HOURS", "DPT-5", "h", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 27, 5, hours));

    PLogicalInteger minutes(new LogicalInteger(GD::bl));
    minutes->minimumValue = 0;
    minutes->maximumValue = 59;
    additionalParameters.push_back(createParameter(function, baseName + ".MINUTES", "DPT-5", "min", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 34, 6, minutes));

    PLogicalInteger seconds(new LogicalInteger(GD::bl));
    seconds->minimumValue = 0;
    seconds->maximumValue = 59;
    additionalParameters.push_back(createParameter(function, baseName + ".SECONDS", "DPT-5", "s", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 42, 6, seconds));

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".FAULT",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   48,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".WORKING_DAY",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   49,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".WD_FIELD_INVALID",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   50,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".YEAR_FIELD_INVALID",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   51,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".MONTH_AND_DOM_FIELDS_INVALID",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   52,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".DOW_FIELD_INVALID",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   53,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".TIME_FIELDS_INVALID",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   54,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".SUMMERTIME",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   55,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".EXT_SYNC_SIGNAL",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   56,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
  }

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}