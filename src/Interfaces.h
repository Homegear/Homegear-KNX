/* Copyright 2013-2019 Homegear GmbH */

#ifndef INTERFACES_H_
#define INTERFACES_H_

#include <homegear-base/BaseLib.h>

namespace Knx
{

class KnxIpForwarder;

using namespace BaseLib;

class Interfaces : public BaseLib::Systems::PhysicalInterfaces
{
public:
	Interfaces(BaseLib::SharedObjects* bl, std::map<std::string, Systems::PPhysicalInterfaceSettings> physicalInterfaceSettings);
	virtual ~Interfaces();

protected:
    std::unordered_map<std::string, std::shared_ptr<KnxIpForwarder>> _forwarders;

	virtual void create();
};

}

#endif
