/* Copyright 2013-2017 Homegear UG (haftungsbeschränkt) */

#include "Factory.h"
#include "../config.h"
#include "GD.h"

BaseLib::Systems::DeviceFamily* MyFactory::createDeviceFamily(BaseLib::SharedObjects* bl, BaseLib::Systems::DeviceFamily::IFamilyEventSink* eventHandler)
{
	return new MyFamily::MyFamily(bl, eventHandler);
}

std::string getVersion()
{
	return VERSION;
}

int32_t getFamilyId()
{
	return MY_FAMILY_ID;
}

std::string getFamilyName()
{
	return MY_FAMILY_NAME;
}

BaseLib::Systems::SystemFactory* getFactory()
{
	return (BaseLib::Systems::SystemFactory*)(new MyFactory);
}
