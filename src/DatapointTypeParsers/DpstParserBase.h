/* Copyright 2013-2019 Homegear GmbH */

#ifndef HOMEGEAR_KNX_DPSTPARSERBASE_H
#define HOMEGEAR_KNX_DPSTPARSERBASE_H

#include <homegear-base/DeviceDescription/Physical.h>
#include <homegear-base/Systems/Role.h>

#include <unordered_map>

namespace BaseLib {

class SharedObjects;

namespace DeviceDescription {
class Function;
class Parameter;
class ILogical;
}
}

namespace Knx {

class DpstParserBase {
 public:
  static std::shared_ptr<BaseLib::DeviceDescription::Parameter> createParameter(const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                                                                                const std::string &name,
                                                                                const std::string &metadata,
                                                                                const std::string &unit,
                                                                                BaseLib::DeviceDescription::IPhysical::OperationType::Enum operationType,
                                                                                bool readable,
                                                                                bool writeable,
                                                                                bool readOnInit,
                                                                                const std::unordered_map<uint64_t, BaseLib::Role> &roles,
                                                                                uint16_t address,
                                                                                int32_t size = -1,
                                                                                std::shared_ptr<BaseLib::DeviceDescription::ILogical> logical = std::shared_ptr<BaseLib::DeviceDescription::ILogical>(),
                                                                                bool noCast = false);
  virtual void parse(BaseLib::SharedObjects *bl,
                     const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                     const std::string &datapointType,
                     uint32_t datapointSubtype,
                     std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) = 0;
};

}

#endif //HOMEGEAR_KNX_DPSTPARSERBASE_H
