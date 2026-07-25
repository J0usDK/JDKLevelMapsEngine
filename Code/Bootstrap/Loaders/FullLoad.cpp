#include "StdAfx.h"
#include "FullLoad.h"

#include <CryThreading/IJobManager.h>

#include "Runtime/Database/MapsDatabase.h"
#include "Bootstrap/Reader/MapReader.h"

#include "Runtime/Maps/VegetationMap.h"

void JDKLevelMaps::Bootstrap::CFullLoad::Initialize(Maps::Database::CMapsDatabase& db, const string& directory)
{
	std::vector<string> files = JDKLevelMaps::Maps::GetFiles(directory, "*.jdkm");
	if (files.empty()) return;

	std::vector<std::unique_ptr<JDKLevelMaps::Maps::ILevelMap>> loadedResults(files.size());
	JobManager::SJobState jobSyncState;

	for (size_t i = 0; i < files.size(); ++i)
	{
		gEnv->pJobManager->AddLambdaJob("LoadLevelMap", [this, &loadedResults, &files, i]()
		{
			if (auto map = LoadMapInternal(files[i]))
				loadedResults[i] = std::move(map);
		}, JobManager::eRegularPriority, &jobSyncState);
	}

	gEnv->pJobManager->WaitForJob(jobSyncState);
	db.RegisterMapsBatch(loadedResults);
}

void JDKLevelMaps::Bootstrap::CFullLoad::UnloadAll(Maps::Database::CMapsDatabase& db)
{
	db.UnregisterAll();
}

std::unique_ptr<JDKLevelMaps::Maps::ILevelMap> JDKLevelMaps::Bootstrap::CFullLoad::LoadMapInternal(const string& filePath) const
{
	JDKLevelMaps::Maps::CMapFileReader reader;
	if (!reader.Open(filePath.c_str()))
		return nullptr;

	const auto& header = reader.GetHeader();
	const auto& directory = reader.GetDirectory();
	const size_t totalTiles = directory.size();
	const size_t totalDataBytes = reader.GetTotalPayloadSize();

	JDKLevelMaps::Maps::SLoadedMap rawMap;
	rawMap.header = header;
	rawMap.packedDataSize = totalDataBytes;
	rawMap.tileOffsets.assign(totalTiles, JDKLevelMaps::Maps::kEmptyTileOffset);

	if (totalDataBytes == 0)
	{
		reader.Close();
		return TryConstructMap(rawMap);
	}

	rawMap.packedData = std::make_unique<uint8[]>(totalDataBytes);
	size_t currentOffset = 0;

	for (uint32 tileIdx = 0; tileIdx < totalTiles; ++tileIdx)
	{
		const auto& entry = directory[tileIdx];
		if (entry.flags == 1 || entry.byteSize == 0)
			continue;

		rawMap.tileOffsets[tileIdx] = static_cast<uint32>(currentOffset);
		reader.ReadTile(tileIdx, rawMap.packedData.get() + currentOffset);

		currentOffset += entry.byteSize;
	}

	reader.Close();
	return TryConstructMap(rawMap);
}

std::unique_ptr<JDKLevelMaps::Maps::ILevelMap> JDKLevelMaps::Bootstrap::CFullLoad::TryConstructMap(Maps::SLoadedMap& rawMap) const
{
	switch (rawMap.header.mapType)
	{
		case EMapType::VegetationDensity:
		{
			auto map = std::make_unique<Maps::CVegetationMap>(std::move(rawMap));
			if (map->IsValid())
				return map;
			return nullptr;
		}
		default:
			return nullptr;
	}
}