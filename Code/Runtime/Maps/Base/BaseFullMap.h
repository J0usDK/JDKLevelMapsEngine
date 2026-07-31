#pragma once
#include <memory>
#include <vector>

#include "Shared/ILevelMap.h"
#include "Shared/MapHeader.h"

namespace JDKLevelMaps::Maps
{
	struct SLoadedMap;

	class CBaseFullMap : public virtual ILevelMap
	{
	public:
		explicit CBaseFullMap(SLoadedMap&& loadedMap);
		virtual ~CBaseFullMap() = default;

		EMapType GetType() const override;
		const char* GetFilePath() const override;
		bool IsValid() const override;
		size_t GetMemoryUsage() const override;
		const SMapHeader& GetHeader() const override;
		uint32 GetMaxCapacity() const override;

		CBaseFullMap* AsFullMap() override;
		const CBaseFullMap* AsFullMap() const override;

	protected:
		const uint8* GetTileData(uint32 tileIndex) const;

	protected:
		SMapHeader m_header;
		bool m_isValid = false;

	private:
		std::unique_ptr<uint8[]> m_packedData;
		size_t m_packedDataSize = 0;

		string m_mapFilePath;

		std::vector<uint32> m_tileOffsets;
	};
}