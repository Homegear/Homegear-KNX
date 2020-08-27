/* Copyright 2013-2019 Homegear GmbH */

#include "DpstParser.h"
#include "../GD.h"

#include "Dpst1Parser.h"
#include "Dpst2Parser.h"
#include "Dpst3Parser.h"
#include "Dpst4Parser.h"
#include "Dpst5Parser.h"
#include "Dpst6Parser.h"
#include "Dpst7Parser.h"
#include "Dpst8Parser.h"
#include "Dpst9Parser.h"
#include "Dpst10Parser.h"
#include "Dpst11Parser.h"
#include "Dpst12Parser.h"
#include "Dpst13Parser.h"
#include "Dpst14Parser.h"
#include "Dpst15Parser.h"
#include "Dpst16Parser.h"
#include "Dpst17Parser.h"
#include "Dpst18Parser.h"
#include "Dpst19Parser.h"
#include "Dpst20Parser.h"
#include "Dpst21Parser.h"
#include "Dpst22Parser.h"
#include "Dpst23Parser.h"
#include "Dpst25Parser.h"
#include "Dpst26Parser.h"
#include "Dpst27Parser.h"
#include "Dpst29Parser.h"
#include "Dpst30Parser.h"
#include "Dpst206Parser.h"
#include "Dpst217Parser.h"
#include "Dpst219Parser.h"
#include "Dpst222Parser.h"
#include "Dpst229Parser.h"
#include "Dpst230Parser.h"
#include "Dpst232Parser.h"
#include "Dpst234Parser.h"
#include "Dpst237Parser.h"
#include "Dpst238Parser.h"
#include "Dpst240Parser.h"
#include "Dpst241Parser.h"
#include "Dpst244Parser.h"
#include "Dpst245Parser.h"
#include "Dpst249Parser.h"
#include "Dpst250Parser.h"
#include "Dpst251Parser.h"

namespace Knx {

std::unordered_map<std::string, std::shared_ptr<DpstParserBase>> DpstParser::getParsers() {
  std::unordered_map<std::string, std::shared_ptr<DpstParserBase>> parsers;
  parsers.emplace("DPT-1", std::make_shared<Dpst1Parser>());
  parsers.emplace("DPST-1", std::make_shared<Dpst1Parser>());
  parsers.emplace("DPT-2", std::make_shared<Dpst2Parser>());
  parsers.emplace("DPST-2", std::make_shared<Dpst2Parser>());
  parsers.emplace("DPT-3", std::make_shared<Dpst3Parser>());
  parsers.emplace("DPST-3", std::make_shared<Dpst3Parser>());
  parsers.emplace("DPT-4", std::make_shared<Dpst4Parser>());
  parsers.emplace("DPST-4", std::make_shared<Dpst4Parser>());
  parsers.emplace("DPT-5", std::make_shared<Dpst5Parser>());
  parsers.emplace("DPST-5", std::make_shared<Dpst5Parser>());
  parsers.emplace("DPT-6", std::make_shared<Dpst6Parser>());
  parsers.emplace("DPST-6", std::make_shared<Dpst6Parser>());
  parsers.emplace("DPT-7", std::make_shared<Dpst7Parser>());
  parsers.emplace("DPST-7", std::make_shared<Dpst7Parser>());
  parsers.emplace("DPT-8", std::make_shared<Dpst8Parser>());
  parsers.emplace("DPST-8", std::make_shared<Dpst8Parser>());
  parsers.emplace("DPT-9", std::make_shared<Dpst9Parser>());
  parsers.emplace("DPST-9", std::make_shared<Dpst9Parser>());
  parsers.emplace("DPT-10", std::make_shared<Dpst10Parser>());
  parsers.emplace("DPST-10", std::make_shared<Dpst10Parser>());
  parsers.emplace("DPT-11", std::make_shared<Dpst11Parser>());
  parsers.emplace("DPST-11", std::make_shared<Dpst11Parser>());
  parsers.emplace("DPT-12", std::make_shared<Dpst12Parser>());
  parsers.emplace("DPST-12", std::make_shared<Dpst12Parser>());
  parsers.emplace("DPT-13", std::make_shared<Dpst13Parser>());
  parsers.emplace("DPST-13", std::make_shared<Dpst13Parser>());
  parsers.emplace("DPT-14", std::make_shared<Dpst14Parser>());
  parsers.emplace("DPST-14", std::make_shared<Dpst14Parser>());
  parsers.emplace("DPT-15", std::make_shared<Dpst15Parser>());
  parsers.emplace("DPST-15", std::make_shared<Dpst15Parser>());
  parsers.emplace("DPT-16", std::make_shared<Dpst16Parser>());
  parsers.emplace("DPST-16", std::make_shared<Dpst16Parser>());
  parsers.emplace("DPT-17", std::make_shared<Dpst17Parser>());
  parsers.emplace("DPST-17", std::make_shared<Dpst17Parser>());
  parsers.emplace("DPT-18", std::make_shared<Dpst18Parser>());
  parsers.emplace("DPST-18", std::make_shared<Dpst18Parser>());
  parsers.emplace("DPT-19", std::make_shared<Dpst19Parser>());
  parsers.emplace("DPST-19", std::make_shared<Dpst19Parser>());
  parsers.emplace("DPT-20", std::make_shared<Dpst20Parser>());
  parsers.emplace("DPST-20", std::make_shared<Dpst20Parser>());
  parsers.emplace("DPT-21", std::make_shared<Dpst21Parser>());
  parsers.emplace("DPST-21", std::make_shared<Dpst21Parser>());
  parsers.emplace("DPT-22", std::make_shared<Dpst22Parser>());
  parsers.emplace("DPST-22", std::make_shared<Dpst22Parser>());
  parsers.emplace("DPT-23", std::make_shared<Dpst23Parser>());
  parsers.emplace("DPST-23", std::make_shared<Dpst23Parser>());
  parsers.emplace("DPT-25", std::make_shared<Dpst25Parser>());
  parsers.emplace("DPST-25", std::make_shared<Dpst25Parser>());
  parsers.emplace("DPT-26", std::make_shared<Dpst26Parser>());
  parsers.emplace("DPST-26", std::make_shared<Dpst26Parser>());
  parsers.emplace("DPT-27", std::make_shared<Dpst27Parser>());
  parsers.emplace("DPST-27", std::make_shared<Dpst27Parser>());
  parsers.emplace("DPT-29", std::make_shared<Dpst29Parser>());
  parsers.emplace("DPST-29", std::make_shared<Dpst29Parser>());
  parsers.emplace("DPT-30", std::make_shared<Dpst30Parser>());
  parsers.emplace("DPST-30", std::make_shared<Dpst30Parser>());
  parsers.emplace("DPT-206", std::make_shared<Dpst206Parser>());
  parsers.emplace("DPST-206", std::make_shared<Dpst206Parser>());
  parsers.emplace("DPT-217", std::make_shared<Dpst217Parser>());
  parsers.emplace("DPST-217", std::make_shared<Dpst217Parser>());
  parsers.emplace("DPT-219", std::make_shared<Dpst219Parser>());
  parsers.emplace("DPST-219", std::make_shared<Dpst219Parser>());
  parsers.emplace("DPT-222", std::make_shared<Dpst222Parser>());
  parsers.emplace("DPST-222", std::make_shared<Dpst222Parser>());
  parsers.emplace("DPT-229", std::make_shared<Dpst229Parser>());
  parsers.emplace("DPST-229", std::make_shared<Dpst229Parser>());
  parsers.emplace("DPT-230", std::make_shared<Dpst230Parser>());
  parsers.emplace("DPST-230", std::make_shared<Dpst230Parser>());
  parsers.emplace("DPT-232", std::make_shared<Dpst232Parser>());
  parsers.emplace("DPST-232", std::make_shared<Dpst232Parser>());
  parsers.emplace("DPT-234", std::make_shared<Dpst234Parser>());
  parsers.emplace("DPST-234", std::make_shared<Dpst234Parser>());
  parsers.emplace("DPT-237", std::make_shared<Dpst237Parser>());
  parsers.emplace("DPST-237", std::make_shared<Dpst237Parser>());
  parsers.emplace("DPT-238", std::make_shared<Dpst238Parser>());
  parsers.emplace("DPST-238", std::make_shared<Dpst238Parser>());
  parsers.emplace("DPT-240", std::make_shared<Dpst240Parser>());
  parsers.emplace("DPST-240", std::make_shared<Dpst240Parser>());
  parsers.emplace("DPT-241", std::make_shared<Dpst241Parser>());
  parsers.emplace("DPST-241", std::make_shared<Dpst241Parser>());
  parsers.emplace("DPT-244", std::make_shared<Dpst244Parser>());
  parsers.emplace("DPST-244", std::make_shared<Dpst244Parser>());
  parsers.emplace("DPT-245", std::make_shared<Dpst245Parser>());
  parsers.emplace("DPST-245", std::make_shared<Dpst245Parser>());
  parsers.emplace("DPT-249", std::make_shared<Dpst249Parser>());
  parsers.emplace("DPST-249", std::make_shared<Dpst249Parser>());
  parsers.emplace("DPT-250", std::make_shared<Dpst250Parser>());
  parsers.emplace("DPST-250", std::make_shared<Dpst250Parser>());
  parsers.emplace("DPT-251", std::make_shared<Dpst251Parser>());
  parsers.emplace("DPST-251", std::make_shared<Dpst251Parser>());
  return parsers;
}

bool DpstParser::parse(const std::shared_ptr<BaseLib::DeviceDescription::Function> &function, const std::string &datapointType, std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  static std::unordered_map<std::string, std::shared_ptr<DpstParserBase>> parsers = getParsers();

  uint32_t datapointSubtype = 0;

  std::unordered_map<std::string, std::shared_ptr<DpstParserBase>>::iterator parsersIterator;
  if (datapointType.compare(0, 5, "DPST-") == 0) {
    auto pair = BaseLib::HelperFunctions::splitLast(datapointType, '-');
    parsersIterator = parsers.find(pair.first);
    datapointSubtype = BaseLib::Math::getUnsignedNumber(pair.second);
  } else parsersIterator = parsers.find(datapointType);

  if (parsersIterator == parsers.end()) return false;

  parsersIterator->second->parse(GD::bl, function, datapointType, datapointSubtype, parameter);
  return true;
}

}