/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst13Parser.h"
#include "../Gd.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst13Parser::parse(BaseLib::SharedObjects *bl,
                         const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                         const std::string &datapointType,
                         uint32_t datapointSubtype,
                         std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(Gd::bl));
  parameter->logical = logical;
  logical->minimumValue = -2147483648;
  logical->maximumValue = 2147483647;
  //Counter pulses (signed)
  if (datapointType == "DPST-13-1") parameter->unit = "counter pulses";
    //Flow rate (m³/h)
  else if (datapointType == "DPST-13-2") parameter->unit = "m³/h";
    //Active energy (Wh)
  else if (datapointType == "DPST-13-10") parameter->unit = "Wh";
    //Apparent energy (VAh)
  else if (datapointType == "DPST-13-11") parameter->unit = "VAh";
    //Reactive energy (VARh)
  else if (datapointType == "DPST-13-12") parameter->unit = "VARh";
    //Active energy
  else if (datapointType == "DPST-13-13") parameter->unit = "kWh";
    //Apparent energy
  else if (datapointType == "DPST-13-14") parameter->unit = "kVAh";
    //Reactive energy
  else if (datapointType == "DPST-13-15") parameter->unit = "kVARh";
    //Time lag (s)
  else if (datapointType == "DPST-13-100") parameter->unit = "s";
  else cast->type = "DPT-12";
}

}