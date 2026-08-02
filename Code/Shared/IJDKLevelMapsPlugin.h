#pragma once
#include <CrySystem/ICryPlugin.h>

#include "ILevelMap.h"

namespace JDKLevelMaps::Maps
{
	class IVegetationMap;
}

namespace JDKLevelMaps::Streaming
{
	class IMapAnchor;
	using TStaticAnchorID = uint32;
}

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
		virtual void FinishInit() = 0;
		virtual void Shutdown() = 0;

		virtual void RegisterDynamicAnchor(EMapType targetMap, const Streaming::IMapAnchor* pAnchor, uint16 radius) = 0;
		virtual void UnregisterDynamicAnchor(EMapType targetMap, const Streaming::IMapAnchor* pAnchor) = 0;

		virtual Streaming::TStaticAnchorID RegisterPointAnchor(EMapType targetMap, Vec3 anchorPos, uint16 radius) = 0;
		virtual void UnregisterPointAnchor(EMapType targetMap, Streaming::TStaticAnchorID id) = 0;
		virtual void UpdatePointAnchor(EMapType targetMap, Streaming::TStaticAnchorID id, Vec3 pos) = 0;

		virtual void UnloadAll() = 0;

		virtual const Maps::ILevelMap* GetMap(EMapType type) const = 0;
	};
}