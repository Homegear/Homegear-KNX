/* Copyright 2013-2017 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "DptConverter.h"
#include "GD.h"

namespace MyFamily
{

DptConverter::DptConverter(BaseLib::SharedObjects* baseLib) : _bl(baseLib)
{
	_ansi.reset(new BaseLib::Ansi(true, true));
}

DptConverter::~DptConverter()
{
}

bool DptConverter::fitsInFirstByte(const std::string& type)
{
	try
	{
		return 	type == "DPT-1" || type.compare(0, 7, "DPST-1-") == 0 ||
				type == "DPT-2" || type.compare(0, 7, "DPST-2-") == 0 ||
				type == "DPT-3" || type.compare(0, 7, "DPST-3-") == 0 ||
				type == "DPT-23" || type.compare(0, 8, "DPST-23-") == 0;
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return false;
}

std::vector<uint8_t> DptConverter::getDpt(const std::string& type, const PVariable& value)
{
	std::vector<uint8_t> dpt;
	try
	{
		if(type == "DPT-1")
		{
			dpt.push_back(value->booleanValue ? 1 : 0);
		}
		else if(type == "DPT-2" || type.compare(0, 7, "DPST-2-") == 0)
		{
			dpt.push_back(value->integerValue & 0x03);
		}
		else if(type == "DPT-3" || type.compare(0, 7, "DPST-3-") == 0)
		{
			dpt.push_back(value->integerValue & 0x0F);
		}
		else if(type == "DPT-4" || type.compare(0, 7, "DPST-4-") == 0)
		{
			if(type == "DPST-4-1") dpt.push_back(value->integerValue & 0x7F);
			else dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-5" || type.compare(0, 7, "DPST-5-") == 0)
		{
			if(type == "DPST-5-1") dpt.push_back(std::lround((double)value->integerValue * 2.55) & 0xFF);
			else if(type == "DPST-5-3") dpt.push_back(std::lround((double)value->integerValue / 1.4117647) & 0xFF);
			else dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-6" || type.compare(0, 7, "DPST-6-") == 0)
		{
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-7" || type.compare(0, 7, "DPST-7-") == 0)
		{
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-8" || type.compare(0, 7, "DPST-8-") == 0)
		{
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-9" || type.compare(0, 7, "DPST-9-") == 0)
		{
			dpt.reserve(2);

			double floatValue = value->floatValue * 100;
			uint16_t sign = 0;
			if(floatValue < 0)
			{
				sign = 0x8000;
				floatValue = -floatValue;
			}

			uint16_t exponent = 0;

			while(floatValue > 2048)
			{
				exponent++;
				floatValue /= 2;
			}

			if(exponent > 15)
			{
				_bl->out.printError("Error: DPT-9 is larger than 670760.");
				dpt.push_back(0x7F);
				dpt.push_back(0xFF);
				return dpt;
			}

			uint16_t result = sign | (exponent << 11) | std::lround(floatValue);
			dpt.push_back(result >> 8);
			dpt.push_back(result & 0xFF);
		}
		else if(type == "DPT-10" || type.compare(0, 8, "DPST-10-") == 0)
		{
			dpt.push_back((value->integerValue >> 16) & 0xFF);
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-11" || type.compare(0, 8, "DPST-11-") == 0)
		{
			dpt.push_back((value->integerValue >> 16) & 0xFF);
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-12" || type.compare(0, 8, "DPST-12-") == 0)
		{
			dpt.push_back(value->integerValue >> 24);
			dpt.push_back((value->integerValue >> 16) & 0xFF);
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-13" || type.compare(0, 8, "DPST-13-") == 0)
		{
			dpt.push_back(value->integerValue >> 24);
			dpt.push_back((value->integerValue >> 16) & 0xFF);
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-14" || type.compare(0, 8, "DPST-14-") == 0)
		{
			uint32_t ieee754 = BaseLib::Math::getIeee754Binary32(value->floatValue);

			dpt.push_back(ieee754 >> 24);
			dpt.push_back((ieee754 >> 16) & 0xFF);
			dpt.push_back((ieee754 >> 8) & 0xFF);
			dpt.push_back(ieee754 & 0xFF);
		}
		else if(type == "DPT-15" || type.compare(0, 8, "DPST-15-") == 0)
		{
			dpt.push_back(value->integerValue >> 24);
			dpt.push_back((value->integerValue >> 16) & 0xFF);
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-16" || type.compare(0, 8, "DPST-16-") == 0)
		{
			std::string ansiString = _ansi->toAnsi(value->stringValue);
			dpt.reserve(ansiString.size() + 1);
			if(!ansiString.empty()) dpt.insert(dpt.end(), ansiString.begin(), ansiString.end());
			dpt.resize(14, 0); //Size is always 14 bytes.
		}
		else if(type == "DPT-17" || type.compare(0, 8, "DPST-17-") == 0)
		{
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-18" || type.compare(0, 8, "DPST-18-") == 0)
		{
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-19" || type.compare(0, 8, "DPST-19-") == 0)
		{
			dpt.push_back(value->integerValue64 >> 56);
			dpt.push_back((value->integerValue64 >> 48) & 0xFF);
			dpt.push_back((value->integerValue64 >> 40) & 0xFF);
			dpt.push_back((value->integerValue64 >> 32) & 0xFF);
			dpt.push_back((value->integerValue64 >> 24) & 0xFF);
			dpt.push_back((value->integerValue64 >> 16) & 0xFF);
			dpt.push_back((value->integerValue64 >> 8) & 0xFF);
			dpt.push_back(value->integerValue64 & 0xFF);
		}
		else if(type == "DPT-20" || type.compare(0, 8, "DPST-20-") == 0)
		{
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-21" || type.compare(0, 8, "DPST-21-") == 0)
		{
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-22" || type.compare(0, 8, "DPST-22-") == 0)
		{
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-23" || type.compare(0, 8, "DPST-23-") == 0)
		{
			dpt.push_back(value->integerValue & 0x03);
		}
		else if(type == "DPT-25" || type.compare(0, 8, "DPST-25-") == 0)
		{
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-26" || type.compare(0, 8, "DPST-26-") == 0)
		{
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-27" || type.compare(0, 8, "DPST-27-") == 0)
		{
			dpt.push_back(value->integerValue >> 24);
			dpt.push_back((value->integerValue >> 16) & 0xFF);
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-29" || type.compare(0, 8, "DPST-29-") == 0)
		{
			dpt.push_back(value->integerValue64 >> 56);
			dpt.push_back((value->integerValue64 >> 48) & 0xFF);
			dpt.push_back((value->integerValue64 >> 40) & 0xFF);
			dpt.push_back((value->integerValue64 >> 32) & 0xFF);
			dpt.push_back((value->integerValue64 >> 24) & 0xFF);
			dpt.push_back((value->integerValue64 >> 16) & 0xFF);
			dpt.push_back((value->integerValue64 >> 8) & 0xFF);
			dpt.push_back(value->integerValue64 & 0xFF);
		}
		else if(type == "DPT-30" || type.compare(0, 8, "DPST-30-") == 0)
		{
			dpt.push_back((value->integerValue >> 16) & 0xFF);
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-206" || type.compare(0, 9, "DPST-206-") == 0)
		{
			dpt.push_back((value->integerValue >> 16) & 0xFF);
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-217" || type.compare(0, 9, "DPST-217-") == 0)
		{
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-219" || type.compare(0, 9, "DPST-219-") == 0)
		{
			dpt.push_back((value->integerValue64 >> 40) & 0xFF);
			dpt.push_back((value->integerValue64 >> 32) & 0xFF);
			dpt.push_back((value->integerValue64 >> 24) & 0xFF);
			dpt.push_back((value->integerValue64 >> 16) & 0xFF);
			dpt.push_back((value->integerValue64 >> 8) & 0xFF);
			dpt.push_back(value->integerValue64 & 0xFF);
		}
		else if(type == "DPT-222" || type.compare(0, 9, "DPST-222-") == 0)
		{
			dpt.push_back((value->integerValue64 >> 40) & 0xFF);
			dpt.push_back((value->integerValue64 >> 32) & 0xFF);
			dpt.push_back((value->integerValue64 >> 24) & 0xFF);
			dpt.push_back((value->integerValue64 >> 16) & 0xFF);
			dpt.push_back((value->integerValue64 >> 8) & 0xFF);
			dpt.push_back(value->integerValue64 & 0xFF);
		}
		else if(type == "DPT-229" || type.compare(0, 9, "DPST-229-") == 0)
		{
			dpt.push_back((value->integerValue64 >> 40) & 0xFF);
			dpt.push_back((value->integerValue64 >> 32) & 0xFF);
			dpt.push_back((value->integerValue64 >> 24) & 0xFF);
			dpt.push_back((value->integerValue64 >> 16) & 0xFF);
			dpt.push_back((value->integerValue64 >> 8) & 0xFF);
			dpt.push_back(value->integerValue64 & 0xFF);
		}
		else if(type == "DPT-230" || type.compare(0, 9, "DPST-230-") == 0)
		{
			dpt.push_back(value->integerValue64 >> 56);
			dpt.push_back((value->integerValue64 >> 48) & 0xFF);
			dpt.push_back((value->integerValue64 >> 40) & 0xFF);
			dpt.push_back((value->integerValue64 >> 32) & 0xFF);
			dpt.push_back((value->integerValue64 >> 24) & 0xFF);
			dpt.push_back((value->integerValue64 >> 16) & 0xFF);
			dpt.push_back((value->integerValue64 >> 8) & 0xFF);
			dpt.push_back(value->integerValue64 & 0xFF);
		}
		else if(type == "DPT-232" || type.compare(0, 9, "DPST-232-") == 0)
		{
			dpt.push_back((value->integerValue >> 16) & 0xFF);
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-234" || type.compare(0, 9, "DPST-234-") == 0)
		{
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-237" || type.compare(0, 9, "DPST-237-") == 0)
		{
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-238" || type.compare(0, 9, "DPST-238-") == 0)
		{
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-240" || type.compare(0, 9, "DPST-240-") == 0)
		{
			dpt.push_back((value->integerValue >> 16) & 0xFF);
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
		else if(type == "DPT-241" || type.compare(0, 9, "DPST-241-") == 0)
		{
			dpt.push_back(value->integerValue >> 24);
			dpt.push_back((value->integerValue >> 16) & 0xFF);
			dpt.push_back((value->integerValue >> 8) & 0xFF);
			dpt.push_back(value->integerValue & 0xFF);
		}
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return dpt;
}

PVariable DptConverter::getVariable(const std::string& type, const std::vector<uint8_t>& value)
{
	try
	{
		if(type == "DPT-1")
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-1 vector is empty.");
				return PVariable(new Variable(false));
			}
			return PVariable(new Variable((bool)(value.at(0) & 1)));
		}
		else if(type == "DPT-2" || type.compare(0, 7, "DPST-2-") == 0)
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-2 vector is empty.");
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable((int32_t)value.at(0) & 0x03));
		}
		else if(type == "DPT-3" || type.compare(0, 7, "DPST-3-") == 0)
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-3 vector is empty.");
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable((int32_t)value.at(0) & 0x0F));
		}
		else if(type == "DPT-4" || type.compare(0, 7, "DPST-4-") == 0)
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-4 vector is empty.");
				return PVariable(new Variable((int32_t)0));
			}
			if(type == "DPST-4-1") return PVariable(new Variable((int32_t)value.at(0) & 0x7F));
			else return PVariable(new Variable((int32_t)value.at(0)));
		}
		else if(type == "DPT-5" || type.compare(0, 7, "DPST-5-") == 0)
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-5 vector is empty.");
				return PVariable(new Variable((int32_t)0));
			}
			if(type == "DPST-5-1") return PVariable(new Variable((int32_t)std::lround((double)value.at(0) / 2.55)));
			else if(type == "DPST-5-3") return PVariable(new Variable((int32_t)std::lround((double)value.at(0) * 1.4117647)));
			else return PVariable(new Variable((int32_t)value.at(0)));
		}
		else if(type == "DPT-6" || type.compare(0, 7, "DPST-6-") == 0)
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-6 vector is empty.");
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable((int32_t)(int8_t)value.at(0)));
		}
		else if(type == "DPT-7" || type.compare(0, 7, "DPST-7-") == 0)
		{
			if(value.size() < 2)
			{
				_bl->out.printError("Error: DPT-7 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((uint32_t)value.at(0) << 8) | value.at(1)));
		}
		else if(type == "DPT-8" || type.compare(0, 7, "DPST-8-") == 0)
		{
			if(value.size() < 2)
			{
				_bl->out.printError("Error: DPT-8 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable((int32_t)(((int16_t)value.at(0) << 8) | value.at(1))));
		}
		else if(type == "DPT-9" || type.compare(0, 7, "DPST-9-") == 0)
		{
			if(value.size() < 2)
			{
				_bl->out.printError("Error: DPT-9 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable(0.0));
			}
			uint16_t dptValue = (value.at(0) << 8) | value.at(1);

			uint16_t exponent = (dptValue >> 11) & 0xF;
			int16_t sign = (dptValue & 0x8000) ? -1 : 1;
			int32_t mantisse = dptValue & 0x7FF;

			while(exponent > 0)
			{
				mantisse *= 2;
				exponent--;
			}

			return PVariable(new Variable(((double)mantisse) / 100.0 * (double)sign));
		}
		else if(type == "DPT-10" || type.compare(0, 8, "DPST-10-") == 0)
		{
			if(value.size() < 3)
			{
				_bl->out.printError("Error: DPT-10 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((uint32_t)value.at(0) << 16) | ((uint32_t)value.at(1) << 8) | value.at(2)));
		}
		else if(type == "DPT-11" || type.compare(0, 8, "DPST-11-") == 0)
		{
			if(value.size() < 3)
			{
				_bl->out.printError("Error: DPT-10 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((uint32_t)value.at(0) << 16) | ((uint32_t)value.at(1) << 8) | value.at(2)));
		}
		else if(type == "DPT-12" || type.compare(0, 8, "DPST-12-") == 0)
		{
			if(value.size() < 4)
			{
				_bl->out.printError("Error: DPT-12 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((uint32_t)value.at(0) << 24) | ((uint32_t)value.at(1) << 16) | ((uint32_t)value.at(2) << 8) | value.at(3)));
		}
		else if(type == "DPT-13" || type.compare(0, 8, "DPST-13-") == 0)
		{
			if(value.size() < 4)
			{
				_bl->out.printError("Error: DPT-13 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((int32_t)value.at(0) << 24) | ((int32_t)value.at(1) << 16) | ((int32_t)value.at(2) << 8) | value.at(3)));
		}
		else if(type == "DPT-14" || type.compare(0, 8, "DPST-14-") == 0)
		{
			if(value.size() < 4)
			{
				_bl->out.printError("Error: DPT-14 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable((double)BaseLib::Math::getFloatFromIeee754Binary32(((uint32_t)value.at(0) << 24) | ((uint32_t)value.at(1) << 16) | ((uint32_t)value.at(2) << 8) | value.at(3))));
		}
		else if(type == "DPT-15" || type.compare(0, 8, "DPST-15-") == 0)
		{
			if(value.size() < 4)
			{
				_bl->out.printError("Error: DPT-15 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((int32_t)value.at(0) << 24) | ((int32_t)value.at(1) << 16) | ((int32_t)value.at(2) << 8) | value.at(3)));
		}
		else if(type == "DPT-16" || type.compare(0, 8, "DPST-16-") == 0)
		{
			if(value.empty()) return PVariable(new Variable(std::string()));
			return PVariable(new Variable(_ansi->toUtf8((char*)&value[0], value.size())));
		}
		else if(type == "DPT-17" || type.compare(0, 8, "DPST-17-") == 0)
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-17 vector is empty.");
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable((int32_t)value.at(0)));
		}
		else if(type == "DPT-18" || type.compare(0, 8, "DPST-18-") == 0)
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-18 vector is empty.");
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable((int32_t)value.at(0)));
		}
		else if(type == "DPT-19" || type.compare(0, 8, "DPST-19-") == 0)
		{
			if(value.size() < 8)
			{
				_bl->out.printError("Error: DPT-19 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((int64_t)value.at(0) << 56) | ((int64_t)value.at(1) << 48) | ((int64_t)value.at(2) << 40) | ((int64_t)value.at(3) << 32) | ((int64_t)value.at(4) << 24) | ((int64_t)value.at(5) << 16) | ((int64_t)value.at(6) << 8) | value.at(7)));
		}
		else if(type == "DPT-20" || type.compare(0, 8, "DPST-20-") == 0)
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-20 vector is empty.");
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable((int32_t)value.at(0)));
		}
		else if(type == "DPT-21" || type.compare(0, 8, "DPST-21-") == 0)
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-21 vector is empty.");
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable((int32_t)value.at(0)));
		}
		else if(type == "DPT-22" || type.compare(0, 8, "DPST-22-") == 0)
		{
			if(value.size() < 2)
			{
				_bl->out.printError("Error: DPT-22 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((uint32_t)value.at(0) << 8) | value.at(1)));
		}
		else if(type == "DPT-23" || type.compare(0, 8, "DPST-23-") == 0)
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-23 vector is empty.");
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable((int32_t)value.at(0) & 0x03));
		}
		else if(type == "DPT-25" || type.compare(0, 8, "DPST-25-") == 0)
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-25 vector is empty.");
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable((int32_t)value.at(0)));
		}
		else if(type == "DPT-26" || type.compare(0, 8, "DPST-26-") == 0)
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-26 vector is empty.");
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable((int32_t)value.at(0)));
		}
		else if(type == "DPT-27" || type.compare(0, 8, "DPST-27-") == 0)
		{
			if(value.size() < 4)
			{
				_bl->out.printError("Error: DPT-27 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((int32_t)value.at(0) << 24) | ((int32_t)value.at(1) << 16) | ((int32_t)value.at(2) << 8) | value.at(3)));
		}
		else if(type == "DPT-29" || type.compare(0, 8, "DPST-29-") == 0)
		{
			if(value.size() < 8)
			{
				_bl->out.printError("Error: DPT-29 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((int64_t)value.at(0) << 56) | ((int64_t)value.at(1) << 48) | ((int64_t)value.at(2) << 40) | ((int64_t)value.at(3) << 32) | ((int64_t)value.at(4) << 24) | ((int64_t)value.at(5) << 16) | ((int64_t)value.at(6) << 8) | value.at(7)));
		}
		else if(type == "DPT-30" || type.compare(0, 8, "DPST-30-") == 0)
		{
			if(value.size() < 3)
			{
				_bl->out.printError("Error: DPT-30 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((uint32_t)value.at(0) << 16) | ((uint32_t)value.at(1) << 8) | value.at(2)));
		}
		else if(type == "DPT-206" || type.compare(0, 9, "DPST-206-") == 0)
		{
			if(value.size() < 3)
			{
				_bl->out.printError("Error: DPT-206 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((uint32_t)value.at(0) << 16) | ((uint32_t)value.at(1) << 8) | value.at(2)));
		}
		else if(type == "DPT-217" || type.compare(0, 9, "DPST-217-") == 0)
		{
			if(value.size() < 2)
			{
				_bl->out.printError("Error: DPT-217 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((uint32_t)value.at(0) << 8) | value.at(1)));
		}
		else if(type == "DPT-219" || type.compare(0, 9, "DPST-219-") == 0)
		{
			if(value.size() < 6)
			{
				_bl->out.printError("Error: DPT-219 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((int64_t)value.at(0) << 40) | ((int64_t)value.at(1) << 32) | ((int64_t)value.at(2) << 24) | ((int64_t)value.at(3) << 16) | ((int64_t)value.at(4) << 8) | value.at(5)));
		}
		else if(type == "DPT-222" || type.compare(0, 9, "DPST-222-") == 0)
		{
			if(value.size() < 6)
			{
				_bl->out.printError("Error: DPT-222 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((int64_t)value.at(0) << 40) | ((int64_t)value.at(1) << 32) | ((int64_t)value.at(2) << 24) | ((int64_t)value.at(3) << 16) | ((int64_t)value.at(4) << 8) | value.at(5)));
		}
		else if(type == "DPT-229" || type.compare(0, 9, "DPST-229-") == 0)
		{
			if(value.size() < 6)
			{
				_bl->out.printError("Error: DPT-229 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((int64_t)value.at(0) << 40) | ((int64_t)value.at(1) << 32) | ((int64_t)value.at(2) << 24) | ((int64_t)value.at(3) << 16) | ((int64_t)value.at(4) << 8) | value.at(5)));
		}
		else if(type == "DPT-230" || type.compare(0, 9, "DPST-230-") == 0)
		{
			if(value.size() < 8)
			{
				_bl->out.printError("Error: DPT-230 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((int64_t)value.at(0) << 56) | ((int64_t)value.at(1) << 48) | ((int64_t)value.at(2) << 40) | ((int64_t)value.at(3) << 32) | ((int64_t)value.at(4) << 24) | ((int64_t)value.at(5) << 16) | ((int64_t)value.at(6) << 8) | value.at(7)));
		}
		else if(type == "DPT-232" || type.compare(0, 9, "DPST-232-") == 0)
		{
			if(value.size() < 3)
			{
				_bl->out.printError("Error: DPT-232 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((uint32_t)value.at(0) << 16) | ((uint32_t)value.at(1) << 8) | value.at(2)));
		}
		else if(type == "DPT-234" || type.compare(0, 9, "DPST-234-") == 0)
		{
			if(value.size() < 2)
			{
				_bl->out.printError("Error: DPT-234 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((uint32_t)value.at(0) << 8) | value.at(1)));
		}
		else if(type == "DPT-237" || type.compare(0, 9, "DPST-237-") == 0)
		{
			if(value.size() < 2)
			{
				_bl->out.printError("Error: DPT-237 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((uint32_t)value.at(0) << 8) | value.at(1)));
		}
		else if(type == "DPT-238" || type.compare(0, 9, "DPST-238-") == 0)
		{
			if(value.empty())
			{
				_bl->out.printError("Error: DPT-238 vector is empty.");
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable((int32_t)value.at(0)));
		}
		else if(type == "DPT-240" || type.compare(0, 9, "DPST-240-") == 0)
		{
			if(value.size() < 3)
			{
				_bl->out.printError("Error: DPT-240 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((uint32_t)value.at(0) << 16) | ((uint32_t)value.at(1) << 8) | value.at(2)));
		}
		else if(type == "DPT-241" || type.compare(0, 9, "DPST-241-") == 0)
		{
			if(value.size() < 4)
			{
				_bl->out.printError("Error: DPT-241 vector is too small: " + _bl->hf.getHexString(value));
				return PVariable(new Variable((int32_t)0));
			}
			return PVariable(new Variable(((int32_t)value.at(0) << 24) | ((int32_t)value.at(1) << 16) | ((int32_t)value.at(2) << 8) | value.at(3)));
		}
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return PVariable(new Variable());
}

}
