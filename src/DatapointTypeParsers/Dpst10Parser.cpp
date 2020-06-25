/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst10Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx
{

void Dpst10Parser::parse(BaseLib::SharedObjects* bl, const std::shared_ptr<BaseLib::DeviceDescription::Function>& function, const std::string& datapointType, uint32_t datapointSubtype, std::shared_ptr<BaseLib::DeviceDescription::Parameter>& parameter)
{
    std::vector<PParameter> additionalParameters;
    ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

    PLogicalInteger logical(new LogicalInteger(GD::bl));
    parameter->logical = logical;
    logical->minimumValue = 0;
    logical->maximumValue = 16777215;
    cast->type = "DPT-10";

    if(datapointType == "DPST-10-1")
    {
        std::string baseName = parameter->id;
        parameter->id = baseName + ".RAW";
        if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->roles, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(GD::bl)));

        PLogicalEnumeration weekDays(new LogicalEnumeration(GD::bl));
        additionalParameters.push_back(createParameter(function, baseName + ".DAY", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 3, weekDays));
        weekDays->minimumValue = 0;
        weekDays->maximumValue = 7;
        weekDays->values.emplace_back("No day", 0);
        weekDays->values.emplace_back("Monday", 1);
        weekDays->values.emplace_back("Tuesday", 2);
        weekDays->values.emplace_back("Wednesday", 3);
        weekDays->values.emplace_back("Thursday", 4);
        weekDays->values.emplace_back("Friday", 5);
        weekDays->values.emplace_back("Saturday", 6);
        weekDays->values.emplace_back("Sunday", 7);

        PLogicalInteger hours(new LogicalInteger(GD::bl));
        hours->minimumValue = 0;
        hours->maximumValue = 23;
        additionalParameters.push_back(createParameter(function, baseName + ".HOURS", "DPT-5", "h", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 3, 5, hours));

        PLogicalInteger minutes(new LogicalInteger(GD::bl));
        hours->minimumValue = 0;
        hours->maximumValue = 59;
        additionalParameters.push_back(createParameter(function, baseName + ".MINUTES", "DPT-5", "min", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 10, 6, minutes));

        PLogicalInteger seconds(new LogicalInteger(GD::bl));
        hours->minimumValue = 0;
        hours->maximumValue = 59;
        additionalParameters.push_back(createParameter(function, baseName + ".SECONDS", "DPT-5", "s", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 18, 6, minutes));
    }

    for(auto& additionalParameter : additionalParameters)
    {
        if(!additionalParameter) continue;
        function->variables->parametersOrdered.push_back(additionalParameter);
        function->variables->parameters[additionalParameter->id] = additionalParameter;
    }
}

}