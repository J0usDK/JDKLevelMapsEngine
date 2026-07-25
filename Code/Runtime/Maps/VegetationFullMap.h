#pragma once
#include <vector>
	
#include "Base/BaseFullMap.h"
#include "Shared/IVegetationMap.h"
#include "Shared/MapLayers.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace JDKLevelMaps::Maps
{
	struct SLoadedMap;

	class CVegetationFullMap final : public CBaseFullMap, public IVegetationMap
	{
	public:
		explicit CVegetationFullMap(SLoadedMap&& loadedMap);
		~CVegetationFullMap() override = default;

		uint8 GetDensity(MapLayers::EVegetationLayers layer, float worldX, float worldY) const override;
	};
}