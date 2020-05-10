/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst249Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx
{

void Dpst249Parser::parse(BaseLib::SharedObjects* bl, const std::shared_ptr<BaseLib::DeviceDescription::Function>& function, const std::string& datapointType, uint32_t datapointSubtype, std::shared_ptr<BaseLib::DeviceDescription::Parameter>& parameter)
{
    std::vector<PParameter> additionalParameters;
    ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

    PLogicalInteger logical(new LogicalInteger(GD::bl));
    parameter->logical = logical;
    cast->type = "DPT-249";

    if(datapointType == "DPST-249-600")
    {
        std::string baseName = parameter->id;
        parameter->id = baseName + ".RAW";
        if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->roles, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(GD::bl)));

        PLogicalInteger duration(new LogicalInteger(GD::bl));
        duration->minimumValue = 0;
        duration->maximumValue = 65535;
        additionalParameters.push_back(createParameter(function, baseName + ".DURATION", "DPST-7-4", "100 ms", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 16, duration));

        PLogicalInteger temperature(new LogicalInteger(GD::bl));
        temperature->minimumValue = 0;
        temperature->maximumValue = 65535;
        additionalParameters.push_back(createParameter(function, baseName + ".TEMPERATURE", "DPT-7", "K", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 16, 16, temperature));

        PLogicalInteger brightness(new LogicalInteger(GD::bl));
        brightness->minimumValue = 0;
        brightness->maximumValue = 100;
        additionalParameters.push_back(createParameter(function, baseName + ".BRIGHTNESS", "DPST-5-1", "%", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 32, 8, brightness));

        additionalParameters.push_back(createParameter(function, baseName + ".DURATION_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 45, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".TEMPERATURE_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 46, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".BRIGHTNESS_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 47, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    }

    for(auto& additionalParameter : additionalParameters)
    {
        if(!additionalParameter) continue;
        function->variables->parametersOrdered.push_back(additionalParameter);
        function->variables->parameters[additionalParameter->id] = additionalParameter;
    }
}

}