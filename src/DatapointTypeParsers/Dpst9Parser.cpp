/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst9Parser.h"
#include "../Gd.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst9Parser::parse(BaseLib::SharedObjects *bl,
                        const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                        const std::string &datapointType,
                        uint32_t datapointSubtype,
                        std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalDecimal logical(new LogicalDecimal(Gd::bl));
  parameter->logical = logical;
  logical->minimumValue = -670760;
  logical->maximumValue = 670760;
  cast->type = datapointType;
  //Temperature (°C)
  if (datapointType == "DPST-9-1") {
    if (parameter->unit.empty()) parameter->unit = "°C";
    logical->minimumValue = -273;
  }
    //Temperature difference (K)
  else if (datapointType == "DPST-9-2") { if (parameter->unit.empty()) parameter->unit = "K"; }
    //Kelvin/hour (K/h)
  else if (datapointType == "DPST-9-3") { if (parameter->unit.empty()) parameter->unit = "K/h"; }
    //Lux (lux);
  else if (datapointType == "DPST-9-4") {
    if (parameter->unit.empty()) parameter->unit = "lux";
    logical->minimumValue = 0;
  }
    //Speed (m/s)
  else if (datapointType == "DPST-9-5") {
    if (parameter->unit.empty()) parameter->unit = "m/s";
    logical->minimumValue = 0;
  }
    //Pressure (Pa)
  else if (datapointType == "DPST-9-6") {
    if (parameter->unit.empty()) parameter->unit = "Pa";
    logical->minimumValue = 0;
  }
    //Humidity (%)
  else if (datapointType == "DPST-9-7") {
    if (parameter->unit.empty()) parameter->unit = "%";
    logical->minimumValue = 0;
  }
    //Parts/million (ppm) [air quality]
  else if (datapointType == "DPST-9-8") {
    if (parameter->unit.empty()) parameter->unit = "ppm";
    logical->minimumValue = 0;
  }
    //Air flow (m³/h)
  else if (datapointType == "DPST-9-9") { if (parameter->unit.empty()) parameter->unit = "m³/h"; }
    //Time (s)
  else if (datapointType == "DPST-9-10") { if (parameter->unit.empty()) parameter->unit = "s"; }
    //Time (ms)
  else if (datapointType == "DPST-9-11") { if (parameter->unit.empty()) parameter->unit = "ms"; }
    //Voltage (mV)
  else if (datapointType == "DPST-9-20") { if (parameter->unit.empty()) parameter->unit = "mV"; }
    //Current (mA)
  else if (datapointType == "DPST-9-21") { if (parameter->unit.empty()) parameter->unit = "mA"; }
    //Power density (W/m²)
  else if (datapointType == "DPST-9-22") { if (parameter->unit.empty()) parameter->unit = "W/m²"; }
    //Kelvin/percent (K/%)
  else if (datapointType == "DPST-9-23") { if (parameter->unit.empty()) parameter->unit = "K/%"; }
    //Power (kW)
  else if (datapointType == "DPST-9-24") { if (parameter->unit.empty()) parameter->unit = "kW"; }
    //Volume flow (l/h)
  else if (datapointType == "DPST-9-25") { if (parameter->unit.empty()) parameter->unit = "l/h"; }
    //Rain amount (l/m²)
  else if (datapointType == "DPST-9-26") {
    if (parameter->unit.empty()) parameter->unit = "l/m²";
    logical->minimumValue = -671088.64;
    logical->maximumValue = 670760.96;
  }
    //Temperature (°F)
  else if (datapointType == "DPST-9-27") {
    if (parameter->unit.empty()) parameter->unit = "°F";
    logical->minimumValue = -459.6;
    logical->maximumValue = 670760.96;
  }
    //Wind speed (km/h)
  else if (datapointType == "DPST-9-28") {
    if (parameter->unit.empty()) parameter->unit = "km/h";
    logical->minimumValue = 0;
    logical->maximumValue = 670760.96;
  } else cast->type = "DPT-9";
}

}