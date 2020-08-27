/* Copyright 2013-2019 Homegear GmbH */

#ifndef HOMEGEAR_KNX_DPST229PARSER_H
#define HOMEGEAR_KNX_DPST229PARSER_H

#include "DpstParserBase.h"

namespace Knx {

/**
 * DPT-229 and DPST-229-* => 4-1-1 byte combined information
 */
class Dpst229Parser : public DpstParserBase {
 public:
  void parse(BaseLib::SharedObjects *bl, const std::shared_ptr<BaseLib::DeviceDescription::Function> &function, const std::string &datapointType, uint32_t datapointSubtype, std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) override;
};

}

#endif //HOMEGEAR_KNX_DPST1PARSER_H
