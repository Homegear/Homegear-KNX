/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst5Parser.h"
#include "../Gd.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst5Parser::parse(BaseLib::SharedObjects *bl,
                        const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                        const std::string &datapointType,
                        uint32_t datapointSubtype,
                        std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(Gd::bl));
  parameter->logical = logical;
  logical->minimumValue = 0;
  logical->maximumValue = 255;
  cast->type = datapointType;
  // Percentage (0..100%)
  if (datapointType == "DPST-5-1") {
    logical->minimumValue = 0;
    logical->maximumValue = 100;
    if (parameter->unit.empty()) parameter->unit = "%";
  }
    // Angle
  else if (datapointType == "DPST-5-3") {
    logical->minimumValue = 0;
    logical->maximumValue = 360;
    if (parameter->unit.empty()) parameter->unit = "°";
  }
    // Percentage (0..255%)
  else if (datapointType == "DPST-5-4") { if (parameter->unit.empty()) parameter->unit = "%"; }
    // Tariff (0..255)
  else if (datapointType == "DPST-5-6") {
    logical->minimumValue = 0;
    logical->maximumValue = 254; //Don't know if this is correct it says "0..255" but also "MaxInclusive=254"
  }
    // Counter pulses (0..255)
  else if (datapointType == "DPST-5-10") { if (parameter->unit.empty()) parameter->unit = "counter pulses"; }
  else cast->type = "DPT-5";
}

}