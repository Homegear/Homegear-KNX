/* Copyright 2013-2016 Sathya Laufer
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

DptConverter::DptConverter(BaseLib::Obj* baseLib) : _bl(baseLib)
{
	_ansi.reset(new BaseLib::Ansi(true, true));
}

DptConverter::~DptConverter()
{
}

std::vector<uint8_t> DptConverter::getDpt(const std::string& type, const PVariable& value, bool& fitsInFirstByte)
{
	std::vector<uint8_t> dpt;
	try
	{
		fitsInFirstByte = false;
		if(type == "DPT-1")
		{
			fitsInFirstByte = true;
			dpt.push_back(value->booleanValue ? 1 : 0);
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
		else if(type == "DPT-16" || type.compare(0, 7, "DPST-16") == 0)
		{
			std::string ansiString = _ansi->toAnsi(value->stringValue);
			dpt.reserve(ansiString.size() + 1);
			if(!ansiString.empty()) dpt.insert(dpt.end(), ansiString.begin(), ansiString.end());
			dpt.resize(14, 0); //Size is always 14 bytes.
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
		else if(type == "DPT-9" || type.compare(0, 7, "DPST-9-") == 0)
		{
			if(value.size() < 2)
			{
				_bl->out.printError("Error: DPT-9 vector is too small.");
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
		else if(type == "DPT-16" || type.compare(0, 7, "DPST-16") == 0)
		{
			if(value.empty()) return PVariable(new Variable(std::string()));
			return PVariable(new Variable(_ansi->toUtf8((char*)&value[0], value.size())));
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
