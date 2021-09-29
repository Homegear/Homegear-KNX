/* Copyright 2013-2019 Homegear GmbH */

#include "Gd.h"

namespace Knx {
BaseLib::SharedObjects *Gd::bl = nullptr;
Knx *Gd::family = nullptr;
std::map<std::string, std::shared_ptr<MainInterface>> Gd::physicalInterfaces;
std::shared_ptr<MainInterface> Gd::defaultPhysicalInterface;
BaseLib::Output Gd::out;
}
