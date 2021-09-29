/* Copyright 2013-2019 Homegear GmbH */

#ifndef GD_H_
#define GD_H_

#define MY_FAMILY_ID 14
#define MY_FAMILY_NAME "KNX"

#include <homegear-base/BaseLib.h>
#include "Knx.h"
#include "PhysicalInterfaces/MainInterface.h"

namespace Knx {

class Gd {
 public:
  virtual ~Gd();

  static BaseLib::SharedObjects *bl;
  static Knx *family;
  static std::map<std::string, std::shared_ptr<MainInterface>> physicalInterfaces;
  static std::shared_ptr<MainInterface> defaultPhysicalInterface;
  static BaseLib::Output out;
 private:
  Gd();
};

}

#endif /* GD_H_ */
