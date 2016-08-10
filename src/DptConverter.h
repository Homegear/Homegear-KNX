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

#ifndef DPTCONVERTER_H_
#define DPTCONVERTER_H_

#include <homegear-base/BaseLib.h>

using namespace BaseLib;

namespace MyFamily
{
class DptConverter
{
public:
	DptConverter(BaseLib::Obj* baseLib);
	virtual ~DptConverter();

	bool fitsInFirstByte(const std::string& type);
	std::vector<uint8_t> getDpt(const std::string& type, const PVariable& value);
	PVariable getVariable(const std::string& type, const std::vector<uint8_t>& value);

	std::vector<uint8_t> getPositionV(uint32_t position, uint32_t size, const std::vector<uint8_t>& data);
	uint8_t getPosition8(uint32_t position, uint32_t size, const std::vector<uint8_t>& data);
	uint16_t getPosition16(uint32_t position, uint32_t size, const std::vector<uint8_t>& data);
	uint32_t getPosition32(uint32_t position, uint32_t size, const std::vector<uint8_t>& data);
	uint64_t getPosition64(uint32_t position, uint32_t size, const std::vector<uint8_t>& data);
	void setPosition(uint32_t position, uint32_t size, std::vector<uint8_t>& target, const std::vector<uint8_t>& source);
	void setPosition(uint32_t position, uint32_t size, std::vector<uint8_t>& target, uint8_t source);
	void setPosition(uint32_t position, uint32_t size, std::vector<uint8_t>& target, uint16_t source);
	void setPosition(uint32_t position, uint32_t size, std::vector<uint8_t>& target, uint32_t source);
	void setPosition(uint32_t position, uint32_t size, std::vector<uint8_t>& target, uint64_t source);
protected:
	const uint8_t _bitMaskGet[8] = { 0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };
	const uint8_t _bitMaskSetStart[8] = { 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };
	const uint8_t _bitMaskSetEnd[8] = { 0x00, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };

	BaseLib::Obj* _bl = nullptr;
	std::shared_ptr<BaseLib::Ansi> _ansi;
};

}

#endif
