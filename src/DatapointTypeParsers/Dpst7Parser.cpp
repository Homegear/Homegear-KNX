/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst7Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst7Parser::parse(BaseLib::SharedObjects *bl,
                        const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                        const std::string &datapointType,
                        uint32_t datapointSubtype,
                        std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(GD::bl));
  parameter->logical = logical;
  logical->minimumValue = 0;
  logical->maximumValue = 65535;
  cast->type = datapointType;
  //Pulses
  if (datapointType == "DPST-7-1") { if (parameter->unit.empty()) parameter->unit = "pulses"; }
    //Time (ms)
  else if (datapointType == "DPST-7-2") { if (parameter->unit.empty()) parameter->unit = "ms"; }
    //Time (10 ms)
  else if (datapointType == "DPST-7-3") { if (parameter->unit.empty()) parameter->unit = "* 10 ms"; }
    //Time (100 ms)
  else if (datapointType == "DPST-7-4") { if (parameter->unit.empty()) parameter->unit = "* 100 ms)"; }
    //Time (s)
  else if (datapointType == "DPST-7-5") { if (parameter->unit.empty()) parameter->unit = "s"; }
    //Time (min)
  else if (datapointType == "DPST-7-6") { if (parameter->unit.empty()) parameter->unit = "min"; }
    //Time (h)
  else if (datapointType == "DPST-7-7") { if (parameter->unit.empty()) parameter->unit = "h"; }
    //Length (mm)
  else if (datapointType == "DPST-7-11") { if (parameter->unit.empty()) parameter->unit = "mm"; }
    //Current (mA)
  else if (datapointType == "DPST-7-12") { if (parameter->unit.empty()) parameter->unit = "mA"; }
    //Brightness (lux)
  else if (datapointType == "DPST-7-13") { if (parameter->unit.empty()) parameter->unit = "lux"; }
  else cast->type = "DPT-7";
}

}