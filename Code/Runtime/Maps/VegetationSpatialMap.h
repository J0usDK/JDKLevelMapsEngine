#pragma once
#include "Base/BaseSpatialMap.h"
#include "Shared/IVegetationMap.h"

namespace JDKLevelMaps::Maps
{
	class CVegetationSpatialMap : public CBaseSpatialMap, public IVegetationMap
	{
	public:
		explicit CVegetationSpatialMap(SMapHeader& loadedMap);
		~CVegetationSpatialMap() override = default;

		uint8 GetDensity(MapLayers::EVegetationLayers layer, float worldX, float worldY) const override;
	};
}