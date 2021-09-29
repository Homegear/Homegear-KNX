/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst245Parser.h"
#include "../Gd.h"

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst245Parser::parse(BaseLib::SharedObjects *bl,
                          const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                          const std::string &datapointType,
                          uint32_t datapointSubtype,
                          std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  std::vector<PParameter> additionalParameters;
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalInteger logical(new LogicalInteger(Gd::bl));
  parameter->logical = logical;
  cast->type = "DPT-245";

  if (datapointType == "DPST-245-600") {
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
                                                     parameter->readOnInit,
                                                     parameter->roles,
                                                     parameter->physical->address,
                                                     -1,
                                                     std::make_shared<BaseLib::DeviceDescription::LogicalAction>(Gd::bl)));

    PLogicalEnumeration ltrf(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".LTRF", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 0, 4, ltrf));
    ltrf->minimumValue = 0;
    ltrf->maximumValue = 5;
    ltrf->values.emplace_back("Unknown", 0);
    ltrf->values.emplace_back("Passed in time", 1);
    ltrf->values.emplace_back("Passed max delay exceeded", 2);
    ltrf->values.emplace_back("Failed, test executed in time", 3);
    ltrf->values.emplace_back("Failed, max delay exceeded", 4);
    ltrf->values.emplace_back("Test manually stopped", 5);

    PLogicalEnumeration ltrd(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".LTRF", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 4, 4, ltrd));
    ltrd->minimumValue = 0;
    ltrd->maximumValue = 5;
    ltrd->values.emplace_back("Unknown", 0);
    ltrd->values.emplace_back("Passed in time", 1);
    ltrd->values.emplace_back("Passed max delay exceeded", 2);
    ltrd->values.emplace_back("Failed, test executed in time", 3);
    ltrd->values.emplace_back("Failed, max delay exceeded", 4);
    ltrd->values.emplace_back("Test manually stopped", 5);

    PLogicalEnumeration ltrp(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".LTRF", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 8, 4, ltrp));
    ltrp->minimumValue = 0;
    ltrp->maximumValue = 5;
    ltrp->values.emplace_back("Unknown", 0);
    ltrp->values.emplace_back("Passed in time", 1);
    ltrp->values.emplace_back("Passed max delay exceeded", 2);
    ltrp->values.emplace_back("Failed, test executed in time", 3);
    ltrp->values.emplace_back("Failed, max delay exceeded", 4);
    ltrp->values.emplace_back("Test manually stopped", 5);

    PLogicalEnumeration sf(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".LTRF", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 16, 2, sf));
    sf->minimumValue = 0;
    sf->maximumValue = 2;
    sf->values.emplace_back("Unknown", 0);
    sf->values.emplace_back("Started automatically", 1);
    sf->values.emplace_back("Started by gateway", 2);

    PLogicalEnumeration sd(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".LTRF", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 18, 2, sd));
    sd->minimumValue = 0;
    sd->maximumValue = 2;
    sd->values.emplace_back("Unknown", 0);
    sd->values.emplace_back("Started automatically", 1);
    sd->values.emplace_back("Started by gateway", 2);

    PLogicalEnumeration sp(new LogicalEnumeration(Gd::bl));
    additionalParameters.push_back(createParameter(function, baseName + ".LTRF", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 20, 2, sp));
    sp->minimumValue = 0;
    sp->maximumValue = 2;
    sp->values.emplace_back("Unknown", 0);
    sp->values.emplace_back("Started automatically", 1);
    sp->values.emplace_back("Started by gateway", 2);

    PLogicalInteger ldtr(new LogicalInteger(Gd::bl));
    ldtr->minimumValue = 0;
    ldtr->maximumValue = 510;
    additionalParameters.push_back(createParameter(function, baseName + ".LDTR", "DPT-7", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 24, 16, ldtr));

    PLogicalInteger lpdtr(new LogicalInteger(Gd::bl));
    lpdtr->minimumValue = 0;
    lpdtr->maximumValue = 255;
    additionalParameters.push_back(createParameter(function, baseName + ".LPDTR", "DPT-5", "", IPhysical::OperationType::store, parameter->readable, parameter->writeable, parameter->readOnInit, parameter->roles, 40, 8, lpdtr));
  }

  for (auto &additionalParameter : additionalParameters) {
    if (!additionalParameter) continue;
    function->variables->parametersOrdered.push_back(additionalParameter);
    function->variables->parameters[additionalParameter->id] = additionalParameter;
  }
}

}