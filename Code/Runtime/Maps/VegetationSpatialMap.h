#pragma once
#include "Base/BaseSpatialMap.h"
#include "Shared/IVegetationMap.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace JDKLevelMaps::Maps
{
	class CVegetationSpatialMap : public CBaseSpatialMap, public IVegetationMap
	{
	public:
		explicit CVegetationSpatialMap(SMapHeader&& loadedMap, const string& mapFilePath);
		~CVegetationSpatialMap() override = default;

		size_t GetTileByteSize() const override;

		uint8 GetDensity(MapLayers::EVegetationLayers layer, float worldX, float worldY) const override;
		virtual const Maps::IVegetationMap* AsVegetationMap() const override;
	};
}