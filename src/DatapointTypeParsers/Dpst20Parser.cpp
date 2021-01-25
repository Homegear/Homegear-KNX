/* Copyright 2013-2019 Homegear GmbH */

#include "Dpst20Parser.h"
#include "../Gd.h"

#include <homegear-base/DeviceDescription/Function.h>
#include <homegear-base/DeviceDescription/Parameter.h>
#include <homegear-base/DeviceDescription/ParameterCast.h>

using namespace BaseLib::DeviceDescription;

namespace Knx {

void Dpst20Parser::parse(BaseLib::SharedObjects *bl,
                         const std::shared_ptr<BaseLib::DeviceDescription::Function> &function,
                         const std::string &datapointType,
                         uint32_t datapointSubtype,
                         std::shared_ptr<BaseLib::DeviceDescription::Parameter> &parameter) {
  ParameterCast::PGeneric cast = std::dynamic_pointer_cast<ParameterCast::Generic>(parameter->casts.front());

  PLogicalEnumeration logical(new LogicalEnumeration(Gd::bl));
  parameter->logical = logical;
  cast->type = datapointType;
  //SCLO mode
  if (datapointType == "DPST-20-1") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("Autonomous", 0);
    logical->values.emplace_back("Slave", 1);
    logical->values.emplace_back("Master", 2);
  }
    //Building mode
  else if (datapointType == "DPST-20-2") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("Building in use", 0);
    logical->values.emplace_back("Building not used", 1);
    logical->values.emplace_back("Building protection", 2);
  }
    //Occupied
  else if (datapointType == "DPST-20-3") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("Occupied", 0);
    logical->values.emplace_back("Stand by", 1);
    logical->values.emplace_back("Not occupied", 2);
  }
    //Priority
  else if (datapointType == "DPST-20-4") {
    logical->minimumValue = 0;
    logical->maximumValue = 3;
    logical->values.emplace_back("High", 0);
    logical->values.emplace_back("Medium", 1);
    logical->values.emplace_back("Low", 2);
    logical->values.emplace_back("Void", 3);
  }
    //Light application mode
  else if (datapointType == "DPST-20-5") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("Normal", 0);
    logical->values.emplace_back("Presence simulation", 1);
    logical->values.emplace_back("Night round", 2);
  }
    //Light application area
  else if (datapointType == "DPST-20-6") {
    logical->minimumValue = 0;
    logical->maximumValue = 50;
    logical->values.emplace_back("No fault", 0);
    logical->values.emplace_back("System an functions of common interest", 1);
    logical->values.emplace_back("HVAC general FBs", 10);
    logical->values.emplace_back("HVAC hot water heating", 11);
    logical->values.emplace_back("HVAC direct electrical heating", 12);
    logical->values.emplace_back("HVAC terminal units", 13);
    logical->values.emplace_back("HVAC VAC", 14);
    logical->values.emplace_back("Lighting", 20);
    logical->values.emplace_back("Security", 30);
    logical->values.emplace_back("Load management", 40);
    logical->values.emplace_back("Shutters and blinds", 50);
  }
    //Alarm class
  else if (datapointType == "DPST-20-7") {
    logical->minimumValue = 1;
    logical->maximumValue = 3;
    logical->values.emplace_back("Simple alarm", 1);
    logical->values.emplace_back("Basic alarm", 2);
    logical->values.emplace_back("Extended alarm", 3);
  }
    //PSU mode
  else if (datapointType == "DPST-20-8") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("Disabled (PSU/DPSU fixed off)", 0);
    logical->values.emplace_back("Enabled (PSU/DPSU fixed on)", 1);
    logical->values.emplace_back("Auto (PSU/DPSU automatic on/off)", 2);
  }
    //System error class
  else if (datapointType == "DPST-20-11") {
    logical->minimumValue = 0;
    logical->maximumValue = 18;
    logical->values.emplace_back("No fault", 0);
    logical->values.emplace_back("General device fault (e.g. RAM, EEPROM, UI, watchdog, ...)", 1);
    logical->values.emplace_back("Communication fault", 2);
    logical->values.emplace_back("Configuration fault", 3);
    logical->values.emplace_back("Hardware fault", 4);
    logical->values.emplace_back("Software fault", 5);
    logical->values.emplace_back("Insufficient non volatile memory", 6);
    logical->values.emplace_back("Insufficient volatile memory", 7);
    logical->values.emplace_back("Memory allocation command with size 0 received", 8);
    logical->values.emplace_back("CRC error", 9);
    logical->values.emplace_back("Watchdog reset detected", 10);
    logical->values.emplace_back("Invalid opcode detected", 11);
    logical->values.emplace_back("General protection fault", 12);
    logical->values.emplace_back("Maximum table length exceeded", 13);
    logical->values.emplace_back("Undefined load command received", 14);
    logical->values.emplace_back("Group address table is not sorted", 15);
    logical->values.emplace_back("Invalid connection number (TSAP)", 16);
    logical->values.emplace_back("Invalid group object number (ASAP)", 17);
    logical->values.emplace_back("Group object type exceeds (PID_MAX_APDU_LENGTH - 2)", 18);
  }
    //HVAC error class
  else if (datapointType == "DPST-20-12") {
    logical->minimumValue = 0;
    logical->maximumValue = 4;
    logical->values.emplace_back("No fault", 0);
    logical->values.emplace_back("Sensor fault", 1);
    logical->values.emplace_back("Process fault / controller fault", 2);
    logical->values.emplace_back("Actuator fault", 3);
    logical->values.emplace_back("Other fault", 4);
  }
    //Time delay
  else if (datapointType == "DPST-20-13") {
    logical->minimumValue = 0;
    logical->maximumValue = 25;
    logical->values.emplace_back("Not active", 0);
    logical->values.emplace_back("1 s", 1);
    logical->values.emplace_back("2 s", 2);
    logical->values.emplace_back("3 s", 3);
    logical->values.emplace_back("5 s", 4);
    logical->values.emplace_back("10 s", 5);
    logical->values.emplace_back("15 s", 6);
    logical->values.emplace_back("20 s", 7);
    logical->values.emplace_back("30 s", 8);
    logical->values.emplace_back("45 s", 9);
    logical->values.emplace_back("1 min", 10);
    logical->values.emplace_back("1.25 min", 11);
    logical->values.emplace_back("1.5 min", 12);
    logical->values.emplace_back("2 min", 13);
    logical->values.emplace_back("2.5 min", 14);
    logical->values.emplace_back("3 min", 15);
    logical->values.emplace_back("5 min", 16);
    logical->values.emplace_back("15 min", 17);
    logical->values.emplace_back("20 min", 18);
    logical->values.emplace_back("30 min", 19);
    logical->values.emplace_back("1 h", 20);
    logical->values.emplace_back("2 h", 21);
    logical->values.emplace_back("3 h", 22);
    logical->values.emplace_back("5 h", 23);
    logical->values.emplace_back("12 h", 24);
    logical->values.emplace_back("24 h", 25);
  }
    //Wind force scale (0..12)
  else if (datapointType == "DPST-20-14") {
    logical->minimumValue = 0;
    logical->maximumValue = 12;
    logical->values.emplace_back("Calm (no wind)", 0);
    logical->values.emplace_back("Light air", 1);
    logical->values.emplace_back("Light breeze", 2);
    logical->values.emplace_back("Gentle breeze", 3);
    logical->values.emplace_back("Moderate breeze", 4);
    logical->values.emplace_back("Fresh breeze", 5);
    logical->values.emplace_back("Strong breeze", 6);
    logical->values.emplace_back("Near gale / moderate gale", 7);
    logical->values.emplace_back("Fresh gale", 8);
    logical->values.emplace_back("Strong gale", 9);
    logical->values.emplace_back("Whole gale / storm", 10);
    logical->values.emplace_back("Violent storm", 11);
    logical->values.emplace_back("Hurricane", 12);
  }
    //Sensor mode
  else if (datapointType == "DPST-20-17") {
    logical->minimumValue = 0;
    logical->maximumValue = 4;
    logical->values.emplace_back("Inactive", 0);
    logical->values.emplace_back("Digital input not inverted", 1);
    logical->values.emplace_back("Digital input inverted", 2);
    logical->values.emplace_back("Analog input -&gt; 0% to 100%", 3);
    logical->values.emplace_back("Temperature sensor input", 4);
  }
    //Actuator connect type
  else if (datapointType == "DPST-20-20") {
    logical->minimumValue = 1;
    logical->maximumValue = 2;
    logical->values.emplace_back("Sensor connection", 1);
    logical->values.emplace_back("Controller connection", 2);
  }
    //Fuel type
  else if (datapointType == "DPST-20-100") {
    logical->minimumValue = 0;
    logical->maximumValue = 3;
    logical->values.emplace_back("Value 1", 0);
    logical->values.emplace_back("Value 2", 1);
    logical->values.emplace_back("Value 3", 2);
    logical->values.emplace_back("Value 4", 3);
  }
    //Burner type
  else if (datapointType == "DPST-20-101") {
    logical->minimumValue = 0;
    logical->maximumValue = 3;
    logical->values.emplace_back("Reserved", 0);
    logical->values.emplace_back("1 stage", 1);
    logical->values.emplace_back("2 stage", 2);
    logical->values.emplace_back("Modulating", 3);
  }
    //HVAC mode
  else if (datapointType == "DPST-20-102") {
    logical->minimumValue = 0;
    logical->maximumValue = 4;
    logical->values.emplace_back("Auto", 0);
    logical->values.emplace_back("Comfort", 1);
    logical->values.emplace_back("Standby", 2);
    logical->values.emplace_back("Economy", 3);
    logical->values.emplace_back("Building protection", 4);
  }
    //DHW mode
  else if (datapointType == "DPST-20-103") {
    logical->minimumValue = 0;
    logical->maximumValue = 4;
    logical->values.emplace_back("Auto", 0);
    logical->values.emplace_back("Legio protect", 1);
    logical->values.emplace_back("Normal", 2);
    logical->values.emplace_back("Reduced", 3);
    logical->values.emplace_back("Off / frost protect", 4);
  }
    //Load priority
  else if (datapointType == "DPST-20-104") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("None", 0);
    logical->values.emplace_back("Shift load priority", 1);
    logical->values.emplace_back("Absolut load priority", 2);
  }
    //HVAC control mode
  else if (datapointType == "DPST-20-105") {
    logical->minimumValue = 0;
    logical->maximumValue = 20;
    logical->values.emplace_back("Auto", 0);
    logical->values.emplace_back("Heat", 1);
    logical->values.emplace_back("Morning warmup", 2);
    logical->values.emplace_back("Cool", 3);
    logical->values.emplace_back("Night purge", 4);
    logical->values.emplace_back("Precool", 5);
    logical->values.emplace_back("Off", 6);
    logical->values.emplace_back("Test", 7);
    logical->values.emplace_back("Emergency heat", 8);
    logical->values.emplace_back("Fan only", 9);
    logical->values.emplace_back("Free cool", 10);
    logical->values.emplace_back("Ice", 11);
    logical->values.emplace_back("Maximum heating mode", 12);
    logical->values.emplace_back("Economic heat/cool mode", 13);
    logical->values.emplace_back("Dehumidification", 14);
    logical->values.emplace_back("Calibration mode", 15);
    logical->values.emplace_back("Emergency cool mode", 16);
    logical->values.emplace_back("Emergency steam mode", 17);
    logical->values.emplace_back("NoDem", 20);
  }
    //HVAC emergency mode
  else if (datapointType == "DPST-20-106") {
    logical->minimumValue = 0;
    logical->maximumValue = 5;
    logical->values.emplace_back("Normal", 0);
    logical->values.emplace_back("EmergPressure", 1);
    logical->values.emplace_back("EmergDepressure", 2);
    logical->values.emplace_back("EmergPurge", 3);
    logical->values.emplace_back("EmergShutdown", 4);
    logical->values.emplace_back("EmergFire", 5);
  }
    //Changeover mode
  else if (datapointType == "DPST-20-107") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("Auto", 0);
    logical->values.emplace_back("Cooling only", 1);
    logical->values.emplace_back("Heating only", 2);
  }
    //Valve mode
  else if (datapointType == "DPST-20-108") {
    logical->minimumValue = 1;
    logical->maximumValue = 5;
    logical->values.emplace_back("Heat stage A for normal heating", 1);
    logical->values.emplace_back("Heat stage B for heating with two stages (A + B)", 2);
    logical->values.emplace_back("Cool stage A for normal cooling", 3);
    logical->values.emplace_back("Cool stage B for cooling with two stages (A + B)", 4);
    logical->values.emplace_back("Heat/Cool for changeover applications", 5);
  }
    //Damper mode
  else if (datapointType == "DPST-20-109") {
    logical->minimumValue = 1;
    logical->maximumValue = 4;
    logical->values.emplace_back("Fresh air, e.g. for fancoils", 1);
    logical->values.emplace_back("Supply air, e.g. for VAV", 2);
    logical->values.emplace_back("Extract air, e.g. for VAV", 3);
    logical->values.emplace_back("Extract air, e.g. for VAV", 4);
  }
    //Heater mode
  else if (datapointType == "DPST-20-110") {
    logical->minimumValue = 1;
    logical->maximumValue = 3;
    logical->values.emplace_back("Heat stage A on/off", 1);
    logical->values.emplace_back("Heat stage A proportional", 2);
    logical->values.emplace_back("Heat stage B proportional", 3);
  }
    //Fan mode
  else if (datapointType == "DPST-20-111") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("Nor running", 0);
    logical->values.emplace_back("Permanently running", 1);
    logical->values.emplace_back("Running in intervals", 2);
  }
    //Master/slave mode
  else if (datapointType == "DPST-20-112") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("Autonomous", 0);
    logical->values.emplace_back("Master", 1);
    logical->values.emplace_back("Slave", 2);
  }
    //Status room setpoint
  else if (datapointType == "DPST-20-113") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("Normal setpoint", 0);
    logical->values.emplace_back("Alternative setpoint", 1);
    logical->values.emplace_back("Building protection setpoint", 2);
  }
    //Metering device type
  else if (datapointType == "DPST-20-114") {
    logical->minimumValue = 0;
    logical->maximumValue = 255;
    logical->values.emplace_back("Other device", 0);
    logical->values.emplace_back("Oil meter", 1);
    logical->values.emplace_back("Electricity meter", 2);
    logical->values.emplace_back("Gas device", 3);
    logical->values.emplace_back("Heat meter", 4);
    logical->values.emplace_back("Steam meter", 5);
    logical->values.emplace_back("Warm water meter", 6);
    logical->values.emplace_back("Water meter", 7);
    logical->values.emplace_back("Heat cost allocator", 8);
    logical->values.emplace_back("Reserved", 9);
    logical->values.emplace_back("Cooling load meter (outlet)", 10);
    logical->values.emplace_back("Cooling load meter (inlet)", 11);
    logical->values.emplace_back("Heat (inlet)", 12);
    logical->values.emplace_back("Heat and cool", 13);
    logical->values.emplace_back("Breaker (electricity)", 32);
    logical->values.emplace_back("Valve (gas or water)", 33);
    logical->values.emplace_back("Waste water meter", 40);
    logical->values.emplace_back("Garbage", 41);
    logical->values.emplace_back("Void device type", 255);
  }
    //ADA type
  else if (datapointType == "DPST-20-120") {
    logical->minimumValue = 1;
    logical->maximumValue = 2;
    logical->values.emplace_back("Air damper", 1);
    logical->values.emplace_back("VAV", 2);
  }
    //Backup mode
  else if (datapointType == "DPST-20-121") {
    logical->minimumValue = 0;
    logical->maximumValue = 1;
    logical->values.emplace_back("Backup value", 0);
    logical->values.emplace_back("Keep last state", 1);
  }
    //Start synchronization type
  else if (datapointType == "DPST-20-122") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("Position unchanged", 0);
    logical->values.emplace_back("Single close", 1);
    logical->values.emplace_back("Single open", 2);
  }
    //Behavior lock/unlock
  else if (datapointType == "DPST-20-600") {
    logical->minimumValue = 0;
    logical->maximumValue = 6;
    logical->values.emplace_back("Off", 0);
    logical->values.emplace_back("On", 1);
    logical->values.emplace_back("No change", 2);
    logical->values.emplace_back("Value according to additional parameter", 3);
    logical->values.emplace_back("Memory function value", 4);
    logical->values.emplace_back("Updated value", 5);
    logical->values.emplace_back("Value before locking", 6);
  }
    //Behavior bus power up/down
  else if (datapointType == "DPST-20-601") {
    logical->minimumValue = 0;
    logical->maximumValue = 4;
    logical->values.emplace_back("Off", 0);
    logical->values.emplace_back("On", 1);
    logical->values.emplace_back("No change", 2);
    logical->values.emplace_back("Value according to additional parameter", 3);
    logical->values.emplace_back("Last (value before bus power down)", 4);
  }
    //Dali fade time
  else if (datapointType == "DPST-20-602") {
    logical->minimumValue = 0;
    logical->maximumValue = 15;
    logical->values.emplace_back("0 s (disabled)", 0);
    logical->values.emplace_back("0.7 s", 1);
    logical->values.emplace_back("1.0 s", 2);
    logical->values.emplace_back("1.4 s", 3);
    logical->values.emplace_back("2.0 s", 4);
    logical->values.emplace_back("2.8 s", 5);
    logical->values.emplace_back("4.0 s", 6);
    logical->values.emplace_back("5.7 s", 7);
    logical->values.emplace_back("8.0 s", 8);
    logical->values.emplace_back("11.3 s", 9);
    logical->values.emplace_back("16.0 s", 10);
    logical->values.emplace_back("22.6 s", 11);
    logical->values.emplace_back("32.0 s", 12);
    logical->values.emplace_back("45.3 s", 13);
    logical->values.emplace_back("64.0 s", 14);
    logical->values.emplace_back("90.5 s", 15);
  }
    //Blink mode
  else if (datapointType == "DPST-20-603") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("BlinkingDisabled", 0);
    logical->values.emplace_back("WithoutAcknowledge", 1);
    logical->values.emplace_back("BlinkingWithAcknowledge", 2);
  }
    //Light control mode
  else if (datapointType == "DPST-20-604") {
    logical->minimumValue = 0;
    logical->maximumValue = 1;
    logical->values.emplace_back("Automatic light control", 0);
    logical->values.emplace_back("Manual light control", 1);
  }
    //PB switch mode
  else if (datapointType == "DPST-20-605") {
    logical->minimumValue = 1;
    logical->maximumValue = 2;
    logical->values.emplace_back("One PB/binary input mode", 1);
    logical->values.emplace_back("Two PBs/binary inputs mode", 2);
  }
    //PB action mode
  else if (datapointType == "DPST-20-606") {
    logical->minimumValue = 0;
    logical->maximumValue = 3;
    logical->values.emplace_back("Inactive (no message sent)", 0);
    logical->values.emplace_back("SwitchOff message sent", 1);
    logical->values.emplace_back("SwitchOn message sent", 2);
    logical->values.emplace_back("Inverse value of InfoOnOff is sent", 3);
  }
    //PB dimm mode
  else if (datapointType == "DPST-20-607") {
    logical->minimumValue = 1;
    logical->maximumValue = 4;
    logical->values.emplace_back("One PB/binary input; SwitchOnOff inverts on each transmission", 1);
    logical->values.emplace_back("One PB/binary input, On / DimUp message sent", 2);
    logical->values.emplace_back("One PB/binary input, Off / DimDown message sent", 3);
    logical->values.emplace_back("Two PBs/binary inputs mode", 4);
  }
    //Switch on mode
  else if (datapointType == "DPST-20-608") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("Last actual value", 0);
    logical->values.emplace_back("Value according to additional parameter", 1);
    logical->values.emplace_back("Last received absolute set value", 2);
  }
    //Load type
  else if (datapointType == "DPST-20-609") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("Automatic", 0);
    logical->values.emplace_back("Leading edge (inductive load)", 1);
    logical->values.emplace_back("Trailing edge (capacitive load)", 2);
  }
    //Load type detection
  else if (datapointType == "DPST-20-610") {
    logical->minimumValue = 0;
    logical->maximumValue = 3;
    logical->values.emplace_back("Undefined", 0);
    logical->values.emplace_back("Leading edge (inductive load)", 1);
    logical->values.emplace_back("Trailing edge (capacitive load)", 2);
    logical->values.emplace_back("Detection not possible or error", 3);
  }
    //SAB except behavior
  else if (datapointType == "DPST-20-801") {
    logical->minimumValue = 0;
    logical->maximumValue = 4;
    logical->values.emplace_back("Up", 0);
    logical->values.emplace_back("Down", 1);
    logical->values.emplace_back("No change", 2);
    logical->values.emplace_back("Value according to additional parameter", 3);
    logical->values.emplace_back("Stop", 4);
  }
    //SAB behavior on lock/unlock
  else if (datapointType == "DPST-20-802") {
    logical->minimumValue = 0;
    logical->maximumValue = 6;
    logical->values.emplace_back("Up", 0);
    logical->values.emplace_back("Down", 1);
    logical->values.emplace_back("No change", 2);
    logical->values.emplace_back("Value according to additional parameter", 3);
    logical->values.emplace_back("Stop", 4);
    logical->values.emplace_back("Updated value", 5);
    logical->values.emplace_back("Value before locking", 6);
  }
    //SSSB mode
  else if (datapointType == "DPST-20-803") {
    logical->minimumValue = 1;
    logical->maximumValue = 4;
    logical->values.emplace_back("One push button/binary input", 1);
    logical->values.emplace_back("One push button/binary input, MoveUp / StepUp message sent", 2);
    logical->values.emplace_back("One push button/binary input, MoveDown / StepDown message sent", 3);
    logical->values.emplace_back("Two push buttons/binary inputs mode", 4);
  }
    //Blinds control mode
  else if (datapointType == "DPST-20-804") {
    logical->minimumValue = 0;
    logical->maximumValue = 1;
    logical->values.emplace_back("Automatic control", 0);
    logical->values.emplace_back("Manual control", 1);
  }
    //Communication mode
  else if (datapointType == "DPST-20-1000") {
    logical->minimumValue = 0;
    logical->maximumValue = 255;
    logical->values.emplace_back("Data link layer", 0);
    logical->values.emplace_back("Data link layer bus monitor", 1);
    logical->values.emplace_back("Data link layer raw frames", 2);
    logical->values.emplace_back("cEMI transport layer", 6);
    logical->values.emplace_back("No layer", 255);
  } else if (datapointType == "DPST-20-1001") {
    logical->minimumValue = 1;
    logical->maximumValue = 7;
    logical->values.emplace_back("PL medium domain address", 1);
    logical->values.emplace_back("RF control octet and serial number or DoA", 2);
    logical->values.emplace_back("Bus monitor error flags", 3);
    logical->values.emplace_back("Relative timestamp", 4);
    logical->values.emplace_back("Time delay", 5);
    logical->values.emplace_back("Extended relative timestamp", 6);
    logical->values.emplace_back("BiBat information", 7);
  } else if (datapointType == "DPST-20-1002") {
    logical->minimumValue = 0;
    logical->maximumValue = 2;
    logical->values.emplace_back("Asynchronous", 0);
    logical->values.emplace_back("Asynchronous + BiBat master", 1);
    logical->values.emplace_back("Asynchronous + BiBat slave", 2);
  } else if (datapointType == "DPST-20-1003") {
    logical->minimumValue = 0;
    logical->maximumValue = 3;
    logical->values.emplace_back("No filtering, all supported received frames shall be passed to the cEMI client using L_Data.ind", 0);
    logical->values.emplace_back("Filtering by domain address", 1);
    logical->values.emplace_back("Filtering by KNX serial number table", 2);
    logical->values.emplace_back("Filtering by domain address and by serial number", 3);
  } else {
    auto logicalInteger = std::make_shared<LogicalInteger>(Gd::bl);
    parameter->logical = logicalInteger;
    logicalInteger->minimumValue = 0;
    logicalInteger->maximumValue = 255;
    cast->type = "DPT-20";
  }
}

}