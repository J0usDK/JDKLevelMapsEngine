#pragma once
#include "ILoadingStrategy.h"

namespace JDKLevelMaps::Maps
{
	class ILevelMap;
	struct SLoadedMap;
}

namespace JDKLevelMaps::Bootstrap
{
	class CFullLoad final : public ILoadingStrategy
	{
	public:
		void Initialize(Maps::Database::CMapsDatabase& db, const string& directory) override;
		void UnloadAll(Maps::Database::CMapsDatabase& db) override;

	private:
		std::unique_ptr<Maps::ILevelMap> LoadMapInternal(const string& filePath) const;
		std::unique_ptr<Maps::ILevelMap> TryConstructMap(Maps::SLoadedMap& rawMap) const;
	};
}