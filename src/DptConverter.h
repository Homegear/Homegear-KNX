/* Copyright 2013-2019 Homegear GmbH */

#ifndef DPTCONVERTER_H_
#define DPTCONVERTER_H_

#include <homegear-base/BaseLib.h>

using namespace BaseLib;

namespace MyFamily
{
class DptConverter
{
public:
	DptConverter(BaseLib::SharedObjects* baseLib);
	virtual ~DptConverter();

	bool fitsInFirstByte(const std::string& type);
	std::vector<uint8_t> getDpt(const std::string& type, const PVariable& value);
	PVariable getVariable(const std::string& type, const std::vector<uint8_t>& value);
protected:
	BaseLib::SharedObjects* _bl = nullptr;
	std::shared_ptr<BaseLib::Ansi> _ansi;
};

}

#endif
