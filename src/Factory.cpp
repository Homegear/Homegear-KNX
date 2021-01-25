/* Copyright 2013-2019 Homegear GmbH */

#include "Factory.h"
#include "../config.h"
#include "Gd.h"

BaseLib::Systems::DeviceFamily *MyFactory::createDeviceFamily(BaseLib::SharedObjects *bl, BaseLib::Systems::IFamilyEventSink *eventHandler) {
  return new Knx::Knx(bl, eventHandler);
}

std::string getVersion() {
  return VERSION;
}

int32_t getFamilyId() {
  return MY_FAMILY_ID;
}

std::string getFamilyName() {
  return MY_FAMILY_NAME;
}

BaseLib::Systems::SystemFactory *getFactory() {
  return (BaseLib::Systems::SystemFactory *)(new MyFactory);
}
