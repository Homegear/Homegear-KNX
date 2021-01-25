/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst8Parser.h"
#include "../Gd.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst8Parser::parse(BaseLib::SharedObjects *bl,
                        const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                        const std::string &datapointType,
                        uint32_t datapointSubtype,
                        std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(Gd::bl));
  parameter->logical = logical;
  logical->minimumValue = -32768;
  logical->maximumValue = 32767;
  cast->type = datapointType;
  //Pulses difference
  if (datapointType == "DPST-8-1") { if (parameter->unit.empty()) parameter->unit = "pulses"; }
    //Time lag (ms)
  else if (datapointType == "DPST-8-2") { if (parameter->unit.empty()) parameter->unit = "ms"; }
    //Time lag (10 ms)
  else if (datapointType == "DPST-8-3") { if (parameter->unit.empty()) parameter->unit = "* 10 ms"; }
    //Time lag (100 ms)
  else if (datapointType == "DPST-8-4") { if (parameter->unit.empty()) parameter->unit = "* 100 ms)"; }
    //Time lag (s)
  else if (datapointType == "DPST-8-5") { if (parameter->unit.empty()) parameter->unit = "s"; }
    //Time lag (min)
  else if (datapointType == "DPST-8-6") { if (parameter->unit.empty()) parameter->unit = "min"; }
    //Time lag (h)
  else if (datapointType == "DPST-8-7") { if (parameter->unit.empty()) parameter->unit = "h"; }
    //Percentage difference (%)
  else if (datapointType == "DPST-8-10") { if (parameter->unit.empty()) parameter->unit = "%"; }
    //Rotation angle
  else if (datapointType == "DPST-8-11") { if (parameter->unit.empty()) parameter->unit = "°"; }
  else cast->type = "DPT-8";
}

}