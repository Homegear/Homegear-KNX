/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst12Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx
{

void Dpst12Parser::parse(BaseLib::SharedObjects* bl, const std::shared_ptr<BaseLib::DeviceDescription::Function>& function, const std::string& datapointType, uint32_t datapointSubtype, std::shared_ptr<BaseLib::DeviceDescription::Parameter>& parameter)
{
    ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

    PLogicalInteger logical(new LogicalInteger(GD::bl));
    parameter->logical = logical;
    logical->minimumValue = -2147483648;
    logical->maximumValue = 2147483647;
    //Counter pulses (unsigned)
    if(datapointType == "DPST-12-1") parameter->unit = "counter pulses";
    else cast->type = "DPT-12";
}

}