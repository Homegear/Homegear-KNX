/* Copyright 2013-2019 Homegear GmbH */

#ifndef HOMEGEAR_KNX_DPST237PARSER_H
#define HOMEGEAR_KNX_DPST237PARSER_H

#include "DpstParserBase.h"

namespace Knx {

/**
 * DPT-237 and DPST-237-* => Configuration / diagnostics
 */
class Dpst237Parser : public DpstParserBase {
 public:
  void parse(BaseLib::SharedObjects *bl, const std::shared_ptr<BaseLib::DeviceDescription::Function> &function, const std::string &datapointType, uint32_t datapointSubtype, std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) override;
};

}

#endif //HOMEGEAR_KNX_DPST1PARSER_H
