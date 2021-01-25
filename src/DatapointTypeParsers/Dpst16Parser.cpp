/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst16Parser.h"
#include "../Gd.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst16Parser::parse(BaseLib::SharedObjects *bl,
                         const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                         const std::string &datapointType,
                         uint32_t datapointSubtype,
                         std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalString logical(new LogicalString(Gd::bl));
  parameter->logical = logical;
  logical->defaultValue = "-";
  //DPST-16-0 => ASCII (0 - 127), DPST-16-1 => ISO-8859-1 (0 - 255)
  if (datapointType == "DPST-16-0" || datapointType == "DPST-16-1") cast->type = datapointType;
  else cast->type = "DPT-16";
}

}