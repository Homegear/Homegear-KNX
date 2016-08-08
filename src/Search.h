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

#ifndef SEARCH_H_
#define SEARCH_H_

#include "../config.h"
#include <homegear-base/BaseLib.h>

#include <sys/stat.h>
#include <zip.h>

using namespace BaseLib;
using namespace BaseLib::DeviceDescription;

namespace MyFamily
{
class Search
{
public:
	struct PeerInfo
	{
		std::string serialNumber;
		int32_t type = -1;
	};

	Search(BaseLib::Obj* baseLib);
	virtual ~Search();

	std::vector<PeerInfo> search();
protected:
	struct XmlData
	{
		uint16_t address = 0;
		std::string mainGroupName;
		std::string middleGroupName;
		std::string groupVariableName;
		std::string datapointType;
		BaseLib::PVariable description;
	};

	std::string _xmlPath;
	BaseLib::Obj* _bl = nullptr;

	void createDirectories();
	void createXmlMaintenanceChannel(PHomegearDevice& device);
	void parseDatapointType(std::string& datapointType, BaseLib::DeviceDescription::PParameter& parameter);
	std::vector<std::shared_ptr<std::vector<char>>> extractKnxProjectFiles();
	std::vector<XmlData> extractXmlData(std::vector<std::shared_ptr<std::vector<char>>>& knxProjectFiles);
};

}

#endif
