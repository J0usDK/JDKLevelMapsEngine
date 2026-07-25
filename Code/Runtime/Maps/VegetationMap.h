#pragma once
#include <vector>

#include "Shared/IVegetationMap.h"
#include "Shared/MapHeader.h"
#include "Shared/MapLayers.h"

namespace JDKLevelMaps::Maps
{
	struct SLoadedMap;

	class CVegetationMap final : public IVegetationMap
	{
	public:
		explicit CVegetationMap(SLoadedMap&& loadedMap);

		EMapType GetType() const override;
		bool IsValid() const override;
		size_t GetMemoryUsage() const override;

		uint8 GetDensity(MapLayers::EVegetationLayers layer, float worldX, float worldY) const override;

	private:
		SMapHeader m_header;
		std::unique_ptr<uint8[]> m_packedData;
		size_t m_packedDataSize = 0;

		std::vector<uint32> m_tileOffsets;
		bool m_isValid = false;
	};
}