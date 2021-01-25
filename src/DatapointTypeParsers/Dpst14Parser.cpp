/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst14Parser.h"
#include "../Gd.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst14Parser::parse(BaseLib::SharedObjects *bl,
                         const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                         const std::string &datapointType,
                         uint32_t datapointSubtype,
                         std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalDecimal logical(new LogicalDecimal(Gd::bl));
  parameter->logical = logical;
  //Acceleration
  if (datapointType == "DPST-14-0") parameter->unit = "m/s²";
    //Angular acceleration
  else if (datapointType == "DPST-14-1") parameter->unit = "rad/s²";
    //Activation energy
  else if (datapointType == "DPST-14-2") parameter->unit = "J/mol";
    //Radioactive activity
  else if (datapointType == "DPST-14-3") parameter->unit = "1/s";
    //Amount of substance
  else if (datapointType == "DPST-14-4") parameter->unit = "mol";
    //Angle (radiant)
  else if (datapointType == "DPST-14-6") parameter->unit = "rad";
    //Angle (degree)
  else if (datapointType == "DPST-14-7") parameter->unit = "°";
    //Angular momentum
  else if (datapointType == "DPST-14-8") parameter->unit = "Js";
    //Angular velocity
  else if (datapointType == "DPST-14-9") parameter->unit = "rad/s";
    //Area
  else if (datapointType == "DPST-14-10") parameter->unit = "m²";
    //Capacitance
  else if (datapointType == "DPST-14-11") parameter->unit = "F";
    //Flux density
  else if (datapointType == "DPST-14-12") parameter->unit = "C/m²";
    //Charge density
  else if (datapointType == "DPST-14-13") parameter->unit = "C/m³";
    //Compressibility
  else if (datapointType == "DPST-14-14") parameter->unit = "m²/N";
    //Conductance
  else if (datapointType == "DPST-14-15") parameter->unit = "S";
    //Conductivity
  else if (datapointType == "DPST-14-16") parameter->unit = "S/m";
    //Density
  else if (datapointType == "DPST-14-17") parameter->unit = "kg/m³";
    //Electric charge
  else if (datapointType == "DPST-14-18") parameter->unit = "C";
    //Electric current
  else if (datapointType == "DPST-14-19") parameter->unit = "A";
    //Electric current density
  else if (datapointType == "DPST-14-20") parameter->unit = "A/m²";
    //Electric dipole moment
  else if (datapointType == "DPST-14-21") parameter->unit = "Cm";
    //Electric displacement
  else if (datapointType == "DPST-14-22") parameter->unit = "C/m²";
    //Electric field strength
  else if (datapointType == "DPST-14-23") parameter->unit = "V/m";
    //Electric flux
  else if (datapointType == "DPST-14-24") parameter->unit = "C";
    //Electric flux density
  else if (datapointType == "DPST-14-25") parameter->unit = "C/m²";
    //Electric polarization
  else if (datapointType == "DPST-14-26") parameter->unit = "C/m²";
    //Electric potential
  else if (datapointType == "DPST-14-27") parameter->unit = "V";
    //Electric potential difference
  else if (datapointType == "DPST-14-28") parameter->unit = "V";
    //Electromagnetic moment
  else if (datapointType == "DPST-14-29") parameter->unit = "Am²";
    //Electromotive force
  else if (datapointType == "DPST-14-30") parameter->unit = "V";
    //Energy
  else if (datapointType == "DPST-14-31") parameter->unit = "J";
    //Force
  else if (datapointType == "DPST-14-32") parameter->unit = "N";
    //Frequency
  else if (datapointType == "DPST-14-33") parameter->unit = "Hz";
    //Angular frequency
  else if (datapointType == "DPST-14-34") parameter->unit = "rad/s";
    //Heat capacity
  else if (datapointType == "DPST-14-35") parameter->unit = "J/K";
    //Heat flow rate
  else if (datapointType == "DPST-14-36") parameter->unit = "W";
    //Heat quantity
  else if (datapointType == "DPST-14-37") parameter->unit = "J";
    //Impedance
  else if (datapointType == "DPST-14-38") parameter->unit = "Ω";
    //Length
  else if (datapointType == "DPST-14-39") parameter->unit = "m";
    //Light quantity
  else if (datapointType == "DPST-14-40") parameter->unit = "J";
    //Luminance
  else if (datapointType == "DPST-14-41") parameter->unit = "cd/m²";
    //Luminous flux
  else if (datapointType == "DPST-14-42") parameter->unit = "lm";
    //Luminous intensity
  else if (datapointType == "DPST-14-43") parameter->unit = "cd";
    //Magnetic field strength
  else if (datapointType == "DPST-14-44") parameter->unit = "A/m";
    //Magnetic flux
  else if (datapointType == "DPST-14-45") parameter->unit = "Wb";
    //Magnetic flux density
  else if (datapointType == "DPST-14-46") parameter->unit = "T";
    //Magnetic moment
  else if (datapointType == "DPST-14-47") parameter->unit = "Am²";
    //Magnetic polarization
  else if (datapointType == "DPST-14-48") parameter->unit = "T";
    //Magnetization
  else if (datapointType == "DPST-14-49") parameter->unit = "A/m";
    //Magnetomotive force
  else if (datapointType == "DPST-14-50") parameter->unit = "A";
    //Mass
  else if (datapointType == "DPST-14-51") parameter->unit = "kg";
    //Mass flux
  else if (datapointType == "DPST-14-52") parameter->unit = "kg/s";
    //Momentum
  else if (datapointType == "DPST-14-53") parameter->unit = "N/s";
    //Phase angle (radiant)
  else if (datapointType == "DPST-14-54") parameter->unit = "rad";
    //Phase angle (degrees)
  else if (datapointType == "DPST-14-55") parameter->unit = "°";
    //Power
  else if (datapointType == "DPST-14-56") parameter->unit = "W";
    //Power factor
  else if (datapointType == "DPST-14-57") parameter->unit = "cos Φ";
    //Pressure
  else if (datapointType == "DPST-14-58") parameter->unit = "Pa";
    //Reactance
  else if (datapointType == "DPST-14-59") parameter->unit = "Ω";
    //Resistance
  else if (datapointType == "DPST-14-60") parameter->unit = "Ω";
    //Resistivity
  else if (datapointType == "DPST-14-61") parameter->unit = "Ωm";
    //Self inductance
  else if (datapointType == "DPST-14-62") parameter->unit = "H";
    //Solid angle
  else if (datapointType == "DPST-14-63") parameter->unit = "sr";
    //Sound intensity
  else if (datapointType == "DPST-14-64") parameter->unit = "W/m²";
    //Speed
  else if (datapointType == "DPST-14-65") parameter->unit = "m/s";
    //Stress
  else if (datapointType == "DPST-14-66") parameter->unit = "Pa";
    //Surface tension
  else if (datapointType == "DPST-14-67") parameter->unit = "N/m";
    //Temperature
  else if (datapointType == "DPST-14-68") parameter->unit = "°C";
    //Temperature absolute
  else if (datapointType == "DPST-14-69") parameter->unit = "°C";
    //Temperature difference
  else if (datapointType == "DPST-14-70") parameter->unit = "K";
    //Thermal capacity
  else if (datapointType == "DPST-14-71") parameter->unit = "J/K";
    //Thermal conductivity
  else if (datapointType == "DPST-14-72") parameter->unit = "W/mK";
    //Thermoelectriy power
  else if (datapointType == "DPST-14-73") parameter->unit = "V/K";
    //Time (s)
  else if (datapointType == "DPST-14-74") parameter->unit = "s";
    //Torque
  else if (datapointType == "DPST-14-75") parameter->unit = "Nm";
    //Volume
  else if (datapointType == "DPST-14-76") parameter->unit = "m³";
    //Volume flux
  else if (datapointType == "DPST-14-77") parameter->unit = "m³/s";
    //Weight
  else if (datapointType == "DPST-14-78") parameter->unit = "N";
    //Work
  else if (datapointType == "DPST-14-79") parameter->unit = "J";
  else cast->type = "DPT-14";
}

}