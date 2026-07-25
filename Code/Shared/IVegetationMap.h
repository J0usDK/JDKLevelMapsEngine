#pragma once
#include "ILevelMap.h"
#include "MapLayers.h"

namespace JDKLevelMaps::Maps
{
	class IVegetationMap : public ILevelMap
	{
	public:
		virtual uint8 GetDensity(MapLayers::EVegetationLayers layer, float worldX, float worldY) const = 0;
	};
}