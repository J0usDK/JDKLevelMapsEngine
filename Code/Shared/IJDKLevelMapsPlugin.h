#pragma once
#include <CrySystem/ICryPlugin.h>

#include "ILevelMap.h"

namespace JDKLevelMaps
{
	enum class EMapType : uint16;

	enum class ELoadingMode
	{
		FullLoad,
		SpatialStream //TODO
	};

	class IJDKLevelMapsPlugin : public Cry::IEnginePlugin
	{
	public:
		CRYINTERFACE_DECLARE_GUID(IJDKLevelMapsPlugin, "{947416AF-BFB9-4616-8376-4D12FBEDC081}"_cry_guid);

		virtual void Init(ELoadingMode mode, const string& directory = "") = 0;
		virtual void Shutdown() = 0;

		virtual void LoadAll() = 0;
		virtual void UnloadAll() = 0;

		virtual const Maps::ILevelMap* GetMap(EMapType type) const = 0;

		template<typename TMapInterface>
		const TMapInterface* GetMapAs(JDKLevelMaps::EMapType type) const
		{
			const JDKLevelMaps::Maps::ILevelMap* pBaseMap = GetMap(type);
			if (pBaseMap && pBaseMap->GetType() == type)
				return static_cast<const TMapInterface*>(pBaseMap);
			return nullptr;
		}
	};
}