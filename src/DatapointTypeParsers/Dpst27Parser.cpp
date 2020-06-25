/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst27Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx
{

void Dpst27Parser::parse(BaseLib::SharedObjects* bl, const std::shared_ptr<BaseLib::DeviceDescription::Function>& function, const std::string& datapointType, uint32_t datapointSubtype, std::shared_ptr<BaseLib::DeviceDescription::Parameter>& parameter)
{
    std::vector<PParameter> additionalParameters;
    ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

    PLogicalInteger logical(new LogicalInteger(GD::bl));
    parameter->logical = logical;
    cast->type = "DPT-27";

    //Bit-combined info on/off
    if(datapointType == "DPST-27-1")
    {
        std::string baseName = parameter->id;
        parameter->id = baseName + ".RAW";
        if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->roles, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(GD::bl)));

        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_16_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_15_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 1, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_14_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 2, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_13_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 3, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_12_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 4, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_11_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 5, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_10_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 6, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_9_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 7, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_8_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 8, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_7_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 9, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_6_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 10, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_5_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 11, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_4_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 12, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_3_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 13, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_2_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 14, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".OUTPUT_1_VALID", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 15, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_16", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 16, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_15", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 17, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_14", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 18, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_13", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 19, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_12", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 20, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_11", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 21, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_10", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 22, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_9", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 23, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_8", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 24, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_7", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 25, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_6", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 26, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_5", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 27, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_4", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 28, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_3", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 29, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_2", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 30, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".STATE_1", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 31, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    }

    for(auto& additionalParameter : additionalParameters)
    {
        if(!additionalParameter) continue;
        function->variables->parametersOrdered.push_back(additionalParameter);
        function->variables->parameters[additionalParameter->id] = additionalParameter;
    }
}

}