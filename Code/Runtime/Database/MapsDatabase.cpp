#include "StdAfx.h"
#include "MapsDatabase.h"

#include <algorithm>

#include "Shared/MapHeader.h"
#include "Shared/ILevelMap.h"

void JDKLevelMaps::Maps::Database::CMapsDatabase::Reserve(size_t size)
{
	m_levelMaps.reserve(size);
}

void JDKLevelMaps::Maps::Database::CMapsDatabase::RegisterMapsBatch(std::vector<std::unique_ptr<ILevelMap>>& levelMaps)
{
	m_levelMaps.reserve(m_levelMaps.size() + levelMaps.size());

	for (auto&& map : levelMaps)
		if (map)
			m_levelMaps.push_back(std::move(map));

	std::sort(m_levelMaps.begin(), m_levelMaps.end(),
		[](const std::unique_ptr<ILevelMap>& a, const std::unique_ptr<ILevelMap>& b)
		{ return a->GetType() < b->GetType(); });

	auto duplicatesStart = std::unique(m_levelMaps.begin(), m_levelMaps.end(),
		[](const std::unique_ptr<ILevelMap>& a, std::unique_ptr<ILevelMap>& b)
		{ return a->GetType() == b->GetType(); });

	if (duplicatesStart != m_levelMaps.end())
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "[JDKLevelMaps] Duplicate maps found during batch registration. They were ignored.");
		m_levelMaps.erase(duplicatesStart, m_levelMaps.end());
	}
}

void JDKLevelMaps::Maps::Database::CMapsDatabase::RegisterMap(std::unique_ptr<ILevelMap> levelMap)
{
	EMapType newType = levelMap->GetType();

	auto it = std::lower_bound(m_levelMaps.begin(), m_levelMaps.end(), newType,
		[](const std::unique_ptr<ILevelMap>& map, EMapType type)
		{ return map->GetType() < type; });

	if (it != m_levelMaps.end() && (*it)->GetType() == newType)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "[JDKLevelMaps] Tried to register already existing map");
		return;
	}

	m_levelMaps.insert(it, std::move(levelMap));
}

void JDKLevelMaps::Maps::Database::CMapsDatabase::UnregisterMap(EMapType type)
{
	auto it = std::lower_bound(m_levelMaps.begin(), m_levelMaps.end(), type,
		[type](const std::unique_ptr<ILevelMap>& map, EMapType t)
		{ return map->GetType() < t; });

	if (it != m_levelMaps.end())
		m_levelMaps.erase(it);
}

void JDKLevelMaps::Maps::Database::CMapsDatabase::UnregisterAll()
{
	m_levelMaps.clear();
}

const JDKLevelMaps::Maps::ILevelMap* JDKLevelMaps::Maps::Database::CMapsDatabase::GetMap(EMapType type) const
{
	auto it = std::lower_bound(m_levelMaps.begin(), m_levelMaps.end(), type,
		[type](const std::unique_ptr<ILevelMap>& map, EMapType t)
		{ return map->GetType() < t; });

	if (it != m_levelMaps.end())
		return it->get();

	return nullptr;
}