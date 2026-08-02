#pragma once
#include "Bootstrap/Loaders/ILoadingStrategy.h"

namespace JDKLevelMaps::Maps::Database
{
	class CMapsDatabase;
}

namespace JDKLevelMaps::Bootstrap
{
	class CBootstrapManager final
	{
	public:
		void SetLoadingDirectory(const string& directory);
		void SetLoadingStrategy(std::unique_ptr<ILoadingStrategy> strategy);

		void RegisterDynamicAnchor(EMapType targetMap, const Streaming::IMapAnchor* pAnchor, uint16 radius);
		void UnregisterDynamicAnchor(EMapType targetMap, const Streaming::IMapAnchor* pAnchor);

		Streaming::TStaticAnchorID RegisterPointAnchor(EMapType targetMap, Vec3 anchorPos, uint16 radius);
		void UnregisterPointAnchor(EMapType targetMap, Streaming::TStaticAnchorID id);
		void UpdatePointAnchor(EMapType targetMap, Streaming::TStaticAnchorID id, Vec3 pos);

		void LoadMaps(Maps::Database::CMapsDatabase& db);

		void PreUpdate();
		void PostUpdate(Maps::Database::CMapsDatabase& db);

	private:
		string m_loadingDirectory = "";
		std::unique_ptr<ILoadingStrategy> m_loadingStrategy = nullptr;
	};
}