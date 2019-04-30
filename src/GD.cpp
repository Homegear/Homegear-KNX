/* Copyright 2013-2019 Homegear GmbH */

#include "GD.h"

namespace Knx
{
	BaseLib::SharedObjects* GD::bl = nullptr;
	Knx* GD::family = nullptr;
	std::map<std::string, std::shared_ptr<MainInterface>> GD::physicalInterfaces;
	std::shared_ptr<MainInterface> GD::defaultPhysicalInterface;
	BaseLib::Output GD::out;
}
