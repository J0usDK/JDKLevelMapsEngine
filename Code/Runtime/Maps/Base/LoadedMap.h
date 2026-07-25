#pragma once
#include <memory>
#include <vector>

#include "Shared/MapHeader.h"

namespace JDKLevelMaps::Maps
{
	struct SLoadedMap
	{
		SMapHeader header;
		std::unique_ptr<uint8[]> packedData;
		size_t packedDataSize = 0;
		std::vector<uint32> tileOffsets;
	};
}