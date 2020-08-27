/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst229Parser.h"
#include "../GD.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst229Parser::parse(BaseLib::SharedObjects *bl,
                          const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                          const std::string &datapointType,
                          uint32_t datapointSubtype,
                          std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger64 logical(new LogicalInteger64(GD::bl));
  parameter->logical = logical;
  logical->minimumValue = 0;
  logical->maximumValue = 281474976710655;
  cast->type = "DPT-229";

  //Metering value (value, encoding, cmd)
  if (datapointType == "DPST-229-1") {
    std::string baseName = parameter->id;
    parameter->id = baseName + ".RAW";
    if (parameter->writeable)
      additionalParameters.push_back(createParameter(function,
                                                     baseName + ".SUBMIT",
                                                     "DPT-1",
                                                     "",
                                                     IPhysical::OperationType::command,
                                                     parameter->readable,
                                                     parameter->writeable,
                                                     parameter->roles,
                                                     parameter->physical->address,
                                                     -1,
                                                     std::make_shared<BaseLib::DeviceDescription::LogicalAction>(GD::bl)));

    PLogicalInteger counter(new LogicalInteger(GD::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".COUNTER", "DPT-12", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 0, 32, counter));

    PLogicalEnumeration valueInformation(new LogicalEnumeration(GD::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".APPLICATION_AREA", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->roles, 32, 8, valueInformation));
    valueInformation->minimumValue = 0;
    valueInformation->maximumValue = 177;
    valueInformation->values.emplace_back("Energy, 0.001 Wh", 0);
    valueInformation->values.emplace_back("Energy, 0.01 Wh", 1);
    valueInformation->values.emplace_back("Energy, 0.1 Wh", 2);
    valueInformation->values.emplace_back("Energy, 1 Wh", 3);
    valueInformation->values.emplace_back("Energy, 10 Wh", 4);
    valueInformation->values.emplace_back("Energy, 100 Wh", 5);
    valueInformation->values.emplace_back("Energy, 1000 Wh", 6);
    valueInformation->values.emplace_back("Energy, 10000 Wh", 7);
    valueInformation->values.emplace_back("Energy, 0.001kJ", 8);
    valueInformation->values.emplace_back("Energy, 0.01 kJ", 9);
    valueInformation->values.emplace_back("Energy, 0.1 kJ", 10);
    valueInformation->values.emplace_back("Energy, 1 kJ", 11);
    valueInformation->values.emplace_back("Energy, 10 kJ", 12);
    valueInformation->values.emplace_back("Energy, 100 kJ", 13);
    valueInformation->values.emplace_back("Energy, 1000 kJ", 14);
    valueInformation->values.emplace_back("Energy, 10000 kJ", 15);
    valueInformation->values.emplace_back("Volume, 0.001 l", 16);
    valueInformation->values.emplace_back("Volume, 0.01 l", 17);
    valueInformation->values.emplace_back("Volume, 0.1 l", 18);
    valueInformation->values.emplace_back("Volume, 1 l", 19);
    valueInformation->values.emplace_back("Volume, 10 l", 20);
    valueInformation->values.emplace_back("Volume, 100 l", 21);
    valueInformation->values.emplace_back("Volume, 1000 l", 22);
    valueInformation->values.emplace_back("Volume, 10000 l", 23);
    valueInformation->values.emplace_back("Mass, 0.001 kg", 24);
    valueInformation->values.emplace_back("Mass, 0.01 kg", 25);
    valueInformation->values.emplace_back("Mass, 0.1 kg", 26);
    valueInformation->values.emplace_back("Mass, 1 kg", 27);
    valueInformation->values.emplace_back("Mass, 10 kg", 28);
    valueInformation->values.emplace_back("Mass, 100 kg", 29);
    valueInformation->values.emplace_back("Mass, 1000 kg", 30);
    valueInformation->values.emplace_back("Mass, 10000 kg", 31);

    valueInformation->values.emplace_back("Power, 0.001 W", 40);
    valueInformation->values.emplace_back("Power, 0.01 W", 41);
    valueInformation->values.emplace_back("Power, 0.1 W", 42);
    valueInformation->values.emplace_back("Power, 1 W", 43);
    valueInformation->values.emplace_back("Power, 10 W", 44);
    valueInformation->values.emplace_back("Power, 100 W", 45);
    valueInformation->values.emplace_back("Power, 1000 W", 46);
    valueInformation->values.emplace_back("Power, 10000 W", 47);
    valueInformation->values.emplace_back("Power, 0.001 kJ/h", 48);
    valueInformation->values.emplace_back("Power, 0.01 kJ/h", 49);
    valueInformation->values.emplace_back("Power, 0.1 kJ/h", 50);
    valueInformation->values.emplace_back("Power, 1 kJ/h", 51);
    valueInformation->values.emplace_back("Power, 10 kJ/h", 52);
    valueInformation->values.emplace_back("Power, 100 kJ/h", 53);
    valueInformation->values.emplace_back("Power, 1000 kJ/h", 54);
    valueInformation->values.emplace_back("Power, 10000 kJ/h", 55);
    valueInformation->values.emplace_back("Volume flow, 0.001 l/h", 56);
    valueInformation->values.emplace_back("Volume flow, 0.01 l/h", 57);
    valueInformation->values.emplace_back("Volume flow, 0.1 l/h", 58);
    valueInformation->values.emplace_back("Volume flow, 1 l/h", 59);
    valueInformation->values.emplace_back("Volume flow, 10 l/h", 60);
    valueInformation->values.emplace_back("Volume flow, 100 l/h", 61);
    valueInformation->values.emplace_back("Volume flow, 1000 l/h", 62);
    valueInformation->values.emplace_back("Volume flow, 10000 l/h", 63);
    valueInformation->values.emplace_back("Volume flow, 0.0001 l/min", 64);
    valueInformation->values.emplace_back("Volume flow, 0.001 l/min", 65);
    valueInformation->values.emplace_back("Volume flow, 0.01 l/min", 66);
    valueInformation->values.emplace_back("Volume flow, 0.1 l/min", 67);
    valueInformation->values.emplace_back("Volume flow, 1 l/min", 68);
    valueInformation->values.emplace_back("Volume flow, 10 l/min", 69);
    valueInformation->values.emplace_back("Volume flow, 100 l/min", 70);
    valueInformation->values.emplace_back("Volume flow, 1000 l/min", 71);
    valueInformation->values.emplace_back("Volume flow, 0.001 ml/s", 72);
    valueInformation->values.emplace_back("Volume flow, 0.01 ml/s", 73);
    valueInformation->values.emplace_back("Volume flow, 0.1 ml/s", 74);
    valueInformation->values.emplace_back("Volume flow, 1 ml/s", 75);
    valueInformation->values.emplace_back("Volume flow, 10 ml/s", 76);
    valueInformation->values.emplace_back("Volume flow, 100 ml/s", 77);
    valueInformation->values.emplace_back("Volume flow, 1000 ml/s", 78);
    valueInformation->values.emplace_back("Volume flow, 10000 ml/s", 79);
    valueInformation->values.emplace_back("Mass flow, 0.001 kg/h", 80);
    valueInformation->values.emplace_back("Mass flow, 0.01 kg/h", 81);
    valueInformation->values.emplace_back("Mass flow, 0.1 kg/h", 82);
    valueInformation->values.emplace_back("Mass flow, 1 kg/h", 83);
    valueInformation->values.emplace_back("Mass flow, 10 kg/h", 84);
    valueInformation->values.emplace_back("Mass flow, 100 kg/h", 85);
    valueInformation->values.emplace_back("Mass flow, 1000 kg/h", 86);
    valueInformation->values.emplace_back("Mass flow, 10000 kg/h", 87);

    valueInformation->values.emplace_back("Units for HCA", 110);

    valueInformation->values.emplace_back("Energy, 0.1 MWh", 128);
    valueInformation->values.emplace_back("Energy, 1 MWh", 129);

    valueInformation->values.emplace_back("Energy, 0.1 GJ", 136);
    valueInformation->values.emplace_back("Energy, 1 GJ", 137);

    valueInformation->values.emplace_back("Power, 0.1 MW", 168);
    valueInformation->values.emplace_back("Power, 1 MW", 169);

    valueInformation->values.emplace_back("Power, 0.1 GJ/h", 176);
    valueInformation->values.emplace_back("Power, 1 GJ/h", 177);

    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".ALARM_UN_ACK",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   43,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".IN_ALARM",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   44,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".OVERRIDDEN",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   45,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".FAULT",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   46,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
    additionalParameters.push_back(createParameter(function,
                                                   baseName + ".OUT_OF_SERVICE",
                                                   "DPT-1",
                                                   "",
                                                   IPhysical::OperationType::store,
                                                   parameter->readable,
                                                   parameter->writeable,
                                                   parameter->roles,
                                                   47,
                                                   1,
                                                   std::make_shared<BaseLib::DeviceDescription::LogicalBoolean>(GD::bl)));
  }

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}