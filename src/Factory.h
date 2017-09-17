/* Copyright 2013-2017 Homegear UG (haftungsbeschränkt) */

#ifndef FACTORY_H
#define FACTORY_H

#include <homegear-base/BaseLib.h>
#include "MyFamily.h"

class MyFactory : BaseLib::Systems::SystemFactory
{
public:
	virtual BaseLib::Systems::DeviceFamily* createDeviceFamily(BaseLib::SharedObjects* bl, BaseLib::Systems::DeviceFamily::IFamilyEventSink* eventHandler);
};

extern "C" std::string getVersion();
extern "C" int32_t getFamilyId();
extern "C" std::string getFamilyName();
extern "C" BaseLib::Systems::SystemFactory* getFactory();

#endif
