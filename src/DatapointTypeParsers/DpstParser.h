/* Copyright 2013-2019 Homegear GmbH */

#ifndef HOMEGEAR_KNX_DPSTPARSER_H
#define HOMEGEAR_KNX_DPSTPARSER_H

#include "DpstParserBase.h"

#include <unordered_map>

namespace BaseLib
{
namespace DeviceDescription
{
    class Function;
    class Parameter;
}
}

namespace Knx
{

class DpstParser
{
private:
    static std::unordered_map<std::string, std::shared_ptr<DpstParserBase>> getParsers();
public:
    static bool parse(const std::shared_ptr<BaseLib::DeviceDescription::Function>& function, const std::string& datapointType, std::shared_ptr<BaseLib::DeviceDescription::Parameter>& parameter);
};

}

#endif //HOMEGEAR_KNX_DPSTPARSER_H
