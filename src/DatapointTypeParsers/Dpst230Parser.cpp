/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst230Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx
{

void Dpst230Parser::parse(BaseLib::SharedObjects* bl, const std::shared_ptr<BaseLib::DeviceDescription::Function>& function, const std::string& datapointType, uint32_t datapointSubtype, std::shared_ptr<BaseLib::DeviceDescription::Parameter>& parameter)
{
    std::vector<PParameter> additionalParameters;
    ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

    PLogicalInteger64 logical(new LogicalInteger64(GD::bl));
    parameter->logical = logical;
    cast->type = "DPT-230";

    if(datapointType == "DPST-230-1000")
    {
        std::string baseName = parameter->id;
        parameter->id = baseName + ".RAW";
        if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->roles, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(GD::bl)));

        PLogicalInteger manufacturerId(new LogicalInteger(GD::bl));
        additionalParameters.push_back(createParameter(function, baseName + ".MANUFACTURER_ID", "DPT-7", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 16, manufacturerId));

        PLogicalInteger identNumber(new LogicalInteger(GD::bl));
        additionalParameters.push_back(createParameter(function, baseName + ".IDENT_NUMBER", "DPT-12", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 16, 32, identNumber));

        PLogicalInteger version(new LogicalInteger(GD::bl));
        additionalParameters.push_back(createParameter(function, baseName + ".IDENT_NUMBER", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 48, 8, version));

        PLogicalEnumeration medium(new LogicalEnumeration(GD::bl));
        additionalParameters.push_back(createParameter(function, baseName + ".MEDIUM", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 56, 8, medium));
        medium->minimumValue = 0;
        medium->maximumValue = 55;
        medium->values.emplace_back("Other", 0);
        medium->values.emplace_back("Oil meter", 1);
        medium->values.emplace_back("Electricity meter", 2);
        medium->values.emplace_back("Gas meter", 3);
        medium->values.emplace_back("Heat meter", 4);
        medium->values.emplace_back("Steam meter", 5);
        medium->values.emplace_back("Warm water meter", 6);
        medium->values.emplace_back("Water meter", 7);
        medium->values.emplace_back("Heat cost allocator", 8);
        medium->values.emplace_back("Compressed air", 9);
        medium->values.emplace_back("Cooling load meter (inlet)", 10);
        medium->values.emplace_back("Cooling load meter (outlet)", 11);
        medium->values.emplace_back("Heat (inlet)", 12);
        medium->values.emplace_back("Heat and cool", 13);
        medium->values.emplace_back("Bus/system", 14);
        medium->values.emplace_back("Unknown device type", 15);

        medium->values.emplace_back("Breaker (electricity)", 32);
        medium->values.emplace_back("Valve (gas or water)", 33);

        medium->values.emplace_back("Waste water meter", 40);
        medium->values.emplace_back("Garbage", 41);

        medium->values.emplace_back("Radio converter", 55);
    }

    for(auto& additionalParameter : additionalParameters)
    {
        if(!additionalParameter) continue;
        function->variables->parametersOrdered.push_back(additionalParameter);
        function->variables->parameters[additionalParameter->id] = additionalParameter;
    }
}

}