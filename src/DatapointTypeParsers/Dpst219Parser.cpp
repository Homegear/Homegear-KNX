/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst219Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx
{

void Dpst219Parser::parse(BaseLib::SharedObjects* bl, const std::shared_ptr<BaseLib::DeviceDescription::Function>& function, const std::string& datapointType, uint32_t datapointSubtype, std::shared_ptr<BaseLib::DeviceDescription::Parameter>& parameter)
{
    std::vector<PParameter> additionalParameters;
    ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

    PLogicalInteger64 logical(new LogicalInteger64(GD::bl));
    parameter->logical = logical;
    logical->minimumValue = 0;
    logical->maximumValue = 281474976710655;
    cast->type = "DPT-219";

    if(datapointType == "DPST-219-1")
    {
        std::string baseName = parameter->id;
        parameter->id = baseName + ".RAW";
        if(parameter->writeable) additionalParameters.push_back(createParameter(function, baseName + ".SUBMIT", "DPT-1", "", IPhysical::OperationType::command, parameter->readable, parameter->writeable, parameter->roles, parameter->physical->address, -1, std::make_shared<BaseLib::DeviceDescription::LogicalAction>(GD::bl)));

        PLogicalInteger logNumber(new LogicalInteger(GD::bl));
        logNumber->minimumValue = 0;
        logNumber->maximumValue = 255;
        additionalParameters.push_back(createParameter(function, baseName + ".LOG_NUMBER", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 8, logNumber));

        PLogicalEnumeration alarmPriority(new LogicalEnumeration(GD::bl));
        additionalParameters.push_back(createParameter(function, baseName + ".PRIORITY", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 8, 8, alarmPriority));
        alarmPriority->minimumValue = 0;
        alarmPriority->maximumValue = 3;
        alarmPriority->values.emplace_back("High", 0);
        alarmPriority->values.emplace_back("Medium", 1);
        alarmPriority->values.emplace_back("Low", 2);
        alarmPriority->values.emplace_back("None", 3);

        PLogicalEnumeration applicationArea(new LogicalEnumeration(GD::bl));
        additionalParameters.push_back(createParameter(function, baseName + ".APPLICATION_AREA", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 16, 8, applicationArea));
        applicationArea->minimumValue = 0;
        applicationArea->maximumValue = 50;
        applicationArea->values.emplace_back("No fault", 0);
        applicationArea->values.emplace_back("Common interest", 1);
        applicationArea->values.emplace_back("HVAC general FB's", 10);
        applicationArea->values.emplace_back("HVAC hot water heating", 11);
        applicationArea->values.emplace_back("HVAC electrical heating", 12);
        applicationArea->values.emplace_back("HVAC terminal units", 13);
        applicationArea->values.emplace_back("HVAC VAC", 14);
        applicationArea->values.emplace_back("Lighting", 20);
        applicationArea->values.emplace_back("Security", 30);
        applicationArea->values.emplace_back("Shutters and blinds", 50);

        PLogicalEnumeration errorClass(new LogicalEnumeration(GD::bl));
        additionalParameters.push_back(createParameter(function, baseName + ".ERROR_CLASS", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 24, 8, errorClass));
        errorClass->minimumValue = 0;
        errorClass->maximumValue = 5;
        errorClass->values.emplace_back("No fault", 0);
        errorClass->values.emplace_back("General device fault", 1);
        errorClass->values.emplace_back("Communication fault", 2);
        errorClass->values.emplace_back("Configuration fault", 3);
        errorClass->values.emplace_back("HW fault", 4);
        errorClass->values.emplace_back("SW fault", 5);

        additionalParameters.push_back(createParameter(function, baseName + ".ERROR_CODE_SUP", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 36, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".ALARM_TEXT_SUP", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 37, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".TIME_STAMP_SUP", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 38, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".ACK_SUP", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 39, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));

        additionalParameters.push_back(createParameter(function, baseName + ".LOCKED", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 45, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".ALARM_UN_ACK", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 46, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
        additionalParameters.push_back(createParameter(function, baseName + ".IN_ALARM", "DPT-1", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 47, 1, std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    }

    for(auto& additionalParameter : additionalParameters)
    {
        if(!additionalParameter) continue;
        function->variables->parametersOrdered.push_back(additionalParameter);
        function->variables->parameters[additionalParameter->id] = additionalParameter;
    }
}

}