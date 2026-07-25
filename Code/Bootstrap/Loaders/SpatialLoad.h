#pragma once
#include "ILoadingStrategy.h"

namespace JDKLevelMaps::Bootstrap
{
	class CSpatialLoad : public ILoadingStrategy
	{
	public:
		CSpatialLoad() = default;
		~CSpatialLoad() = default;

		void Initialize(Maps::Database::CMapsDatabase& db, const string& directory) override;
		void UnloadAll(Maps::Database::CMapsDatabase& db) override;

		void Update(Maps::Database::CMapsDatabase& db) override;

		// Registers anchor for objects or entities that can change their position
		void RegisterAnchor(const Streaming::IMapAnchor* anchor, uint16 radius) override;

		// Registers anchor for objects or entities that have constant position
		Streaming::TStaticAnchorId RegisterAnchor(const Vec3& anchorPos, uint16 radius) override;

		void UnregisterAnchor(const Streaming::IMapAnchor* anchor) override;
		void UnregisterAnchor(Streaming::TStaticAnchorId anchorId) override;
	};
}