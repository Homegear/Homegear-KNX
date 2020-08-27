/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst4Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst4Parser::parse(BaseLib::SharedObjects *bl,
                        const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                        const std::string &datapointType,
                        uint32_t datapointSubtype,
                        std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(GD::bl));
  parameter->logical = logical;
  cast->type = datapointType;
  logical->minimumValue = 0;
  logical->maximumValue = 255;
  //ASCII
  if (datapointType == "DPST-4-1") logical->maximumValue = 127;
    //ISO-8859-1
  else if (datapointType == "DPST-4-2") {}
  else cast->type = "DPT-4";
}

}