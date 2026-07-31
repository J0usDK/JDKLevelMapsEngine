#pragma once
#include "CryCore/BaseTypes.h"

namespace JDKLevelMaps
{
	struct SMapHeader;
	enum class EMapType : uint16;
}

namespace JDKLevelMaps::Maps
{
	class CBaseFullMap;
	class CBaseSpatialMap;
	class IVegetationMap;

	class ILevelMap
	{
	public:
		virtual ~ILevelMap() = default;

		virtual EMapType GetType() const = 0;
		virtual const char* GetFilePath() const = 0;
		virtual bool IsValid() const = 0;
		virtual size_t GetMemoryUsage() const = 0;
		virtual const SMapHeader& GetHeader() const = 0;
		virtual uint32 GetMaxCapacity() const = 0;

		virtual CBaseFullMap* AsFullMap() { return nullptr; }
		virtual const CBaseFullMap* AsFullMap() const { return nullptr; }
		virtual CBaseSpatialMap* AsSpatialMap() { return nullptr; }
		virtual const CBaseSpatialMap* AsSpatialMap() const { return nullptr; }

		virtual const Maps::IVegetationMap* AsVegetationMap() const { return nullptr; }
	};
}