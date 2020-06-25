/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst25Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx
{

void Dpst25Parser::parse(BaseLib::SharedObjects* bl, const std::shared_ptr<BaseLib::DeviceDescription::Function>& function, const std::string& datapointType, uint32_t datapointSubtype, std::shared_ptr<BaseLib::DeviceDescription::Parameter>& parameter)
{
    std::vector<PParameter> additionalParameters;
    ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

    PLogicalInteger logical(new LogicalInteger(GD::bl));
    parameter->logical = logical;
    logical->minimumValue = 0;
    logical->maximumValue = 255;
    cast->type = "DPT-25";

    if(datapointType == "DPST-25-1000")
    {
        std::string baseName = parameter->id;
        parameter->id = baseName + ".RAW";
        if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->roles, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(GD::bl)));

        PLogicalInteger nibble1(new LogicalInteger(GD::bl));
        nibble1->minimumValue = 0;
        nibble1->maximumValue = 3;
        additionalParameters.push_back(createParameter(function, baseName + ".BUSY", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 4, nibble1));

        PLogicalInteger nibble2(new LogicalInteger(GD::bl));
        nibble2->minimumValue = 0;
        nibble2->maximumValue = 3;
        additionalParameters.push_back(createParameter(function, baseName + ".NAK", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 4, 4, nibble2));
    }

    for(auto& additionalParameter : additionalParameters)
    {
        if(!additionalParameter) continue;
        function->variables->parametersOrdered.push_back(additionalParameter);
        function->variables->parameters[additionalParameter->id] = additionalParameter;
    }
}

}