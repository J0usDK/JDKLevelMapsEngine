#pragma once
#include <CryMath/Cry_Math.h>
#include <CryString/CryString.h>

namespace JDKLevelMaps
{
	enum class EMapType : uint16;
}

namespace JDKLevelMaps::Streaming
{
	class IMapAnchor;
	using TStaticAnchorID = uint32;
}

namespace JDKLevelMaps::Maps::Database
{
	class CMapsDatabase;
}

namespace JDKLevelMaps::Bootstrap
{
	class ILoadingStrategy
	{
	public:
		virtual ~ILoadingStrategy() = default;

		virtual void Initialize(Maps::Database::CMapsDatabase& db, const string& directory) = 0;
		virtual void UnloadAll(Maps::Database::CMapsDatabase& db) = 0;

		virtual void RegisterDynamicAnchor(EMapType targetMap, const Streaming::IMapAnchor* pAnchor, uint16 radius) {}
		virtual void UnregisterDynamicAnchor(EMapType targetMap, const Streaming::IMapAnchor* pAnchor) {}

		virtual Streaming::TStaticAnchorID RegisterPointAnchor(EMapType targetMap, Vec3 anchorPos, uint16 radius) { return 0; }
		virtual void UnregisterPointAnchor(EMapType targetMap, Streaming::TStaticAnchorID id) {}
		virtual void UpdatePointAnchor(EMapType targetMap, Streaming::TStaticAnchorID id, Vec3 pos) {}

		virtual void PreUpdate() {}
		virtual void PostUpdate(Maps::Database::CMapsDatabase& db) {}
	};
}