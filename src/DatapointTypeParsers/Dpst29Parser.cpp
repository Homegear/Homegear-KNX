/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst29Parser.h"
#include "../Gd.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst29Parser::parse(BaseLib::SharedObjects *bl,
                         const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                         const std::string &datapointType,
                         uint32_t datapointSubtype,
                         std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger64 logical(new LogicalInteger64(Gd::bl));
  parameter->logical = logical;
  cast->type = datapointType;
  //Active energy
  if (datapointType == "DPST-29-10") { if (parameter->unit.empty()) parameter->unit = "Wh"; }
    //Apparent energy
  else if (datapointType == "DPST-29-11") { if (parameter->unit.empty()) parameter->unit = "VAh"; }
    //Reactive energy
  else if (datapointType == "DPST-29-12") { if (parameter->unit.empty()) parameter->unit = "VARh"; }
  else cast->type = "DPT-29";
}

}