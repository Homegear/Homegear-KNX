/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst240Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx
{

void Dpst240Parser::parse(BaseLib::SharedObjects* bl, const std::shared_ptr<BaseLib::DeviceDescription::Function>& function, const std::string& datapointType, uint32_t datapointSubtype, std::shared_ptr<BaseLib::DeviceDescription::Parameter>& parameter)
{
    std::vector<PParameter> additionalParameters;
    ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

    PLogicalInteger logical(new LogicalInteger(GD::bl));
    parameter->logical = logical;
    logical->minimumValue = 0;
    logical->maximumValue = 16777215;
    cast->type = "DPT-240";

    if(datapointType == "DPST-240-800")
    {
        std::string baseName = parameter->id;
        parameter->id = baseName + ".RAW";
        if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->roles, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(GD::bl)));

        PLogicalInteger height(new LogicalInteger(GD::bl));
        height->minimumValue = 0;
        height->maximumValue = 100;
        additionalParameters.push_back(createParameter(function, baseName + ".HEIGHT_POSITION", "DPST-5-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 8, height));

        PLogicalInteger slats(new LogicalInteger(GD::bl));
        slats->minimumValue = 0;
        slats->maximumValue = 100;
        additionalParameters.push_back(createParameter(function, baseName + ".SLATS_POSITION", "DPST-5-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 8, 8, slats));

        additionalParameters.push_back(createParameter(function, baseName + ".HEIGHT_POS_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 22, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".SLATS_POS_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 23, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    }

    for(auto& additionalParameter : additionalParameters)
    {
        if(!additionalParameter) continue;
        function->variables->parametersOrdered.push_back(additionalParameter);
        function->variables->parameters[additionalParameter->id] = additionalParameter;
    }
}

}