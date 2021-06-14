#include "DpstParserBase.h"
#include "../Gd.h"

#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/Function.h>

namespace Knx {

std::shared_ptr<BaseLib::DeviceDescription::Parameter> DpstParserBase::createParameter(const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                                                                                       const std::string &name,
                                                                                       const std::string &metadata,
                                                                                       const std::string &unit,
                                                                                       BaseLib::DeviceDescription::IPhysical::OperationType::Enum operationType,
                                                                                       bool readable,
                                                                                       bool writeable,
                                                                                       bool readOnInit,
                                                                                       const std::unordered_map<uint64_t, BaseLib::Role> &roles,
                                                                                       uint16_t address,
                                                                                       int32_t size,
                                                                                       std::shared_ptr<BaseLib::DeviceDescription::ILogical> logical,
                                                                                       bool noCast) {
  BaseLib::DeviceDescription::PParameter parameter = std::make_shared<BaseLib::DeviceDescription::Parameter>(Gd::bl, function->variables);
  parameter->id = name;
  parameter->metadata = metadata;
  parameter->unit = unit;
  parameter->roles = roles;
  parameter->readable = readable;
  parameter->writeable = writeable;
  parameter->readOnInit = readOnInit;
  if (logical) parameter->logical = logical;
  parameter->physical = std::make_shared<BaseLib::DeviceDescription::Physical>(Gd::bl);
  parameter->physical->operationType = operationType;
  parameter->physical->address = address;
  parameter->physical->bitSize = size;
  if (!noCast) {
    ParameterCast::PGeneric cast(new ParameterCast::Generic(Gd::bl));
    parameter->casts.push_back(cast);
    cast->type = metadata;
  }
  return parameter;
}

}