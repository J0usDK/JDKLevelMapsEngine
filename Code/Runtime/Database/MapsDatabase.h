#pragma once
#include <memory>
#include <vector>

namespace JDKLevelMaps
{
	enum class EMapType : uint16;
}

namespace JDKLevelMaps::Maps
{
	class ILevelMap;
}

namespace JDKLevelMaps::Maps::Database
{
	class CMapsDatabase final
	{
	public:
		void Reserve(size_t size);
		void RegisterMapsBatch(std::vector<std::unique_ptr<ILevelMap>>& levelMaps);

		void RegisterMap(std::unique_ptr<ILevelMap> levelMap);
		void UnregisterMap(EMapType type);
		void UnregisterAll();

		const ILevelMap* GetMap(EMapType type) const;

	private:
		std::vector<std::unique_ptr<ILevelMap>> m_levelMaps;
	};
}