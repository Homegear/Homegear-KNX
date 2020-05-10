/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst23Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx
{

void Dpst23Parser::parse(BaseLib::SharedObjects* bl, const std::shared_ptr<BaseLib::DeviceDescription::Function>& function, const std::string& datapointType, uint32_t datapointSubtype, std::shared_ptr<BaseLib::DeviceDescription::Parameter>& parameter)
{
    ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

    PLogicalEnumeration logical(new LogicalEnumeration(GD::bl));
    parameter->logical = logical;
    cast->type = datapointType;
    //On/off action
    if(datapointType == "DPST-23-1")
    {
        logical->minimumValue = 0;
        logical->maximumValue = 3;
        logical->values.emplace_back("Off", 0);
        logical->values.emplace_back("On", 1);
        logical->values.emplace_back("Off/on", 2);
        logical->values.emplace_back("On/off", 3);
    }
        //Alarm reaction
    else if(datapointType == "DPST-23-2")
    {
        logical->minimumValue = 0;
        logical->maximumValue = 2;
        logical->values.emplace_back("No alarm is used", 0);
        logical->values.emplace_back("Alarm position is UP", 1);
        logical->values.emplace_back("Alarm position is DOWN", 2);
    }
        //Up/down action
    else if(datapointType == "DPST-23-3")
    {
        logical->minimumValue = 0;
        logical->maximumValue = 3;
        logical->values.emplace_back("Up", 0);
        logical->values.emplace_back("Down", 1);
        logical->values.emplace_back("UpDown", 2);
        logical->values.emplace_back("DownUp", 3);
    }
        //HVAC push button action
    else if(datapointType == "DPST-23-102")
    {
        logical->minimumValue = 0;
        logical->maximumValue = 3;
        logical->values.emplace_back("Comfort/Economy", 0);
        logical->values.emplace_back("Comfort/Nothing", 1);
        logical->values.emplace_back("Economy/Nothing", 2);
        logical->values.emplace_back("Building prot/Auto", 3);
    }
    else
    {
        auto logicalInteger = std::make_shared<LogicalInteger>(GD::bl);
        parameter->logical = logicalInteger;
        logicalInteger->minimumValue = 0;
        logicalInteger->maximumValue = 3;
        cast->type = "DPT-23";
    }
}

}