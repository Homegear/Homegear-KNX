/* Copyright 2013-2019 Homegear GmbH */

#ifndef HOMEGEAR_KNX_DPST14PARSER_H
#define HOMEGEAR_KNX_DPST14PARSER_H

#include "DpstParserBase.h"

namespace Knx {

/**
 * DPT-14 and DPST-14-* => 4 byte float value
 */
class Dpst14Parser : public DpstParserBase {
 public:
  void parse(BaseLib::SharedObjects *bl, const std::shared_ptr<BaseLib::DeviceDescription::Function> &function, const std::string &datapointType, uint32_t datapointSubtype, std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) override;
};

}

#endif //HOMEGEAR_KNX_DPST1PARSER_H
