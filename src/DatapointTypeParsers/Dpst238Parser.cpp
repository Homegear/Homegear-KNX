/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst238Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx
{

void Dpst238Parser::parse(BaseLib::SharedObjects* bl, const std::shared_ptr<BaseLib::DeviceDescription::Function>& function, const std::string& datapointType, uint32_t datapointSubtype, std::shared_ptr<BaseLib::DeviceDescription::Parameter>& parameter)
{
    std::vector<PParameter> additionalParameters;
    ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

    PLogicalInteger logical(new LogicalInteger(GD::bl));
    parameter->logical = logical;
    logical->minimumValue = 0;
    logical->maximumValue = 255;
    cast->type = "DPT-238";

    //DPT DALI diagnostics
    if(datapointType == "DPST-238-600")
    {
        std::string baseName = parameter->id;
        parameter->id = baseName + ".RAW";
        if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->roles, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(GD::bl)));

        additionalParameters.push_back(createParameter(function, baseName + ".BALLAST_FAILURE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".LAMP_FAILURE", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 1, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));

        PLogicalInteger red(new LogicalInteger(GD::bl));
        red->minimumValue = 0;
        red->maximumValue = 63;
        additionalParameters.push_back(createParameter(function, baseName + ".DEVICE_ADDRESS", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 2, 6, red));
    }

    for(auto& additionalParameter : additionalParameters)
    {
        if(!additionalParameter) continue;
        function->variables->parametersOrdered.push_back(additionalParameter);
        function->variables->parameters[additionalParameter->id] = additionalParameter;
    }
}

}