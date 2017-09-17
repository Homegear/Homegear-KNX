/* Copyright 2013-2017 Homegear UG (haftungsbeschränkt) */

#include "GD.h"

namespace MyFamily
{
	BaseLib::SharedObjects* GD::bl = nullptr;
	MyFamily* GD::family = nullptr;
	std::map<std::string, std::shared_ptr<MainInterface>> GD::physicalInterfaces;
	std::shared_ptr<MainInterface> GD::defaultPhysicalInterface;
	BaseLib::Output GD::out;
}
