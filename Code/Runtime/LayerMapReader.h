#pragma once
#include <vector>
#include <optional>
#include "Shared/LayerMapHeader.h"

namespace JDKLevelMaps
{
	struct SLoadedLayerMap
	{
		SLayerMapHeader header;
		std::vector<uint8> data;
	};

	std::optional<SLoadedLayerMap> LoadLayerMap(const char* filePath);
}