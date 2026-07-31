#include "StdAfx.h"
#include "SpatialLoad.h"

#include "Shared/IMapAnchor.h"
#include "Runtime/Database/MapsDatabase.h"
#include "Runtime/Maps/Base/LoadedMap.h"
#include "Runtime/Maps/Base/BaseSpatialMap.h"
#include "Runtime/Maps/VegetationSpatialMap.h"
#include "Bootstrap/Reader/MapReader.h"

namespace JDKLevelMaps::Bootstrap
{
	void CSpatialLoad::Initialize(Maps::Database::CMapsDatabase& db, const string& directory)
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

		m_streamingStates.clear();
		for (const auto& pMap : db.GetMaps())
		{
			if (!pMap || !pMap->IsValid())
				continue;

			Maps::CBaseSpatialMap* pSpatialMap = pMap->AsSpatialMap();
			if (!pSpatialMap)
				continue;

			const uint32 tileBudget = ComputeTileBudget(pSpatialMap->GetHeader());
			pSpatialMap->SetMaximalCapacity(tileBudget, pSpatialMap->GetTileByteSize());
			m_streamingStates.emplace_back(pSpatialMap, pSpatialMap->GetMaxCapacity());
		}

		for (auto& state : m_streamingStates)
		{
			m_scratchDecrements.clear();
			m_scratchIncrements.clear();

			ProcessDynamicAnchors(state);
			ProcessStaticAnchors(state);

			for (auto& [x, y] : m_scratchIncrements)
				if (!IncrementTileRef(x, y, state))
					state.deferredIncrements.emplace_back(x, y);

			for (auto& [x, y] : m_scratchDecrements)
				DecrementTileRef(x, y, state);
		}

		if (!m_pendingLoadings.empty())
			DispatchPendingRequests();
	}

	void CSpatialLoad::UnloadAll(Maps::Database::CMapsDatabase& db)
	{
		m_streamingStates.clear();
		m_dynamicAnchors.clear();
		m_staticAnchors.clear();
		db.UnregisterAll();
	}

	uint32 CSpatialLoad::ComputeTileBudget(const SMapHeader& header) const
	{
		const uint64 totalTiles = static_cast<uint64>(header.tileCountX) * header.tileCountY;
		uint64 budget = 0;

		for (const auto& anchor : m_dynamicAnchors)
		{
			const uint64 side = static_cast<uint64>(anchor.radius) * 2 + 1;
			budget += side * side;
			if (budget >= totalTiles)
				return static_cast<uint32>(totalTiles);
		}
		for (const auto& anchor : m_staticAnchors)
		{
			const uint64 side = static_cast<uint64>(anchor.radius) * 2 + 1;
			budget += side * side;
			if (budget >= totalTiles)
				return static_cast<uint32>(totalTiles);
		}

		return static_cast<uint32>(budget);
	}

	std::unique_ptr<Maps::ILevelMap> CSpatialLoad::LoadMapInternal(const string& filePath)
	{
		JDKLevelMaps::Maps::CMapFileReader reader;
		if (!reader.Open(filePath.c_str()))
			return nullptr;

		auto map = TryConstructMap(reader.TakeHeader(), filePath);
		if (auto* pSpatial = map ? map->AsSpatialMap() : nullptr)
			pSpatial->SetTileDirectory(reader.TakeDirectory());

		return map;
	}

	std::unique_ptr<Maps::ILevelMap> CSpatialLoad::TryConstructMap(SMapHeader&& header, const string& filePath) const
	{
		switch (header.mapType)
		{
			case EMapType::VegetationDensity:
			{
				auto map = std::make_unique<Maps::CVegetationSpatialMap>(std::move(header), filePath);
				if (map && map->IsValid())
					return map;
				return nullptr;
			}
			default:
				return nullptr;
		}
	}

	void CSpatialLoad::RegisterDynamicAnchor(const Streaming::IMapAnchor* pAnchor, uint16 radius)
	{
		if (!pAnchor) return;
		m_dynamicAnchors.push_back({ pAnchor, radius, INT32_MIN, INT32_MIN });
	}

	void CSpatialLoad::UnregisterDynamicAnchor(const Streaming::IMapAnchor* pAnchor)
	{
		for (auto& anchor : m_dynamicAnchors)
		{
			if (anchor.pAnchor == pAnchor)
			{
				anchor.bPendingRemoval = true;
				break;
			}
		}
	}

	Streaming::TStaticAnchorID CSpatialLoad::RegisterPointAnchor(Vec3 anchorPos, uint16 radius)
	{
		m_staticAnchors.emplace_back(m_nextStaticAnchorId, std::move(anchorPos), radius, INT32_MIN, INT32_MIN);
		return m_nextStaticAnchorId++;
	}

	void CSpatialLoad::UnregisterPointAnchor(Streaming::TStaticAnchorID anchorId)
	{
		for (auto& anchor : m_staticAnchors)
		{
			if (anchor.id == anchorId)
			{
				anchor.bPendingRemoval = true;
				break;
			}
		}
	}

	void CSpatialLoad::UpdatePointAnchor(Streaming::TStaticAnchorID anchorId, Vec3 pos)
	{
		for (auto& anchor : m_staticAnchors)
		{
			if (anchor.id == anchorId)
			{
				anchor.pos = std::move(pos);
				break;
			}
		}
	}

	void CSpatialLoad::PreUpdate()
	{
		for (size_t i = 0; i < m_pendingLoadings.size();)
		{
			SPendingTileLoad& req = *m_pendingLoadings[i];
			if (req.jobState.IsRunning())
			{
				++i;
				continue;
			}

			if (req.succeeded && !req.abandoned)
				req.pTargetMap->CommitTile(req.tileIndex, req.reservedSlot);
			else
				req.pTargetMap->ReleaseTileSlotWithoutCommit(req.reservedSlot);

			m_pendingLoadings[i] = std::move(m_pendingLoadings.back());
			m_pendingLoadings.pop_back();
		}

		for (auto& state : m_streamingStates)
		{
			state.pMap->FlushPendingMaintenance();
			state.tileRefCounts.FlushRebuild();
		}
	}

	void CSpatialLoad::PostUpdate(Maps::Database::CMapsDatabase& db)
	{
		for (auto& state : m_streamingStates)
		{
			m_scratchIncrements = std::move(state.deferredIncrements);
			state.deferredIncrements.clear();
			m_scratchDecrements.clear();

			ProcessDynamicAnchors(state);
			ProcessStaticAnchors(state);

			for (auto& [x, y] : m_scratchIncrements)
				if (!IncrementTileRef(x, y, state))
					state.deferredIncrements.emplace_back(x, y);

			for (auto& [x, y] : m_scratchDecrements)
				DecrementTileRef(x, y, state);

			state.pMap->FlushPendingMaintenance();
			state.tileRefCounts.FlushRebuild();
		}

		if (!m_pendingLoadings.empty())
			DispatchPendingRequests();
	}

	void CSpatialLoad::ProcessDynamicAnchors(SMapStreamingState& state)
	{
		const auto& header = state.pMap->GetHeader();

		for (auto it = m_dynamicAnchors.begin(); it != m_dynamicAnchors.end();)
		{
			if (it->bPendingRemoval)
			{
				if (it->lastTileX != INT32_MIN)
				{
					const STileRect oldRect = ComputeRect(it->lastTileX, it->lastTileY, it->radius);
					const STileRect emptyRect{ 0, -1, 0, -1 };

					ForEachDiffTile(oldRect, emptyRect, [&](int32 x, int32 y) { m_scratchDecrements.emplace_back(x, y); });
				}
				it = m_dynamicAnchors.erase(it);
				continue;
			}

			float worldX, worldY;
			it->pAnchor->GetPosition(worldX, worldY);

			int32 currentTileX, currentTileY;
			WorldToTile(worldX, worldY, header, currentTileX, currentTileY);

			if (currentTileX != it->lastTileX || currentTileY != it->lastTileY)
			{
				const STileRect newRect = ComputeRect(currentTileX, currentTileY, it->radius);
				const STileRect oldRect = it->lastTileX != INT32_MIN ? ComputeRect(it->lastTileX, it->lastTileY, it->radius) : STileRect{ 0, -1, 0, -1 };

				if (it->lastTileX != INT32_MIN)
					ForEachDiffTile(oldRect, newRect, [&](int32 x, int32 y) { m_scratchDecrements.emplace_back(x, y); });
				ForEachDiffTile(newRect, oldRect, [&](int32 x, int32 y) { m_scratchIncrements.emplace_back(x, y); });

				it->lastTileX = currentTileX;
				it->lastTileY = currentTileY;
			}

			++it;
		}
	}

	void CSpatialLoad::ProcessStaticAnchors(SMapStreamingState& state)
	{
		const auto& header = state.pMap->GetHeader();

		for (auto it = m_staticAnchors.begin(); it != m_staticAnchors.end(); )
		{
			if (it->bPendingRemoval)
			{
				if (it->lastTileX != INT32_MIN)
				{
					const STileRect oldRect = ComputeRect(it->lastTileX, it->lastTileY, it->radius);
					const STileRect emptyRect{ 0, -1, 0, -1 };

					ForEachDiffTile(oldRect, emptyRect, [&](int32 x, int32 y) { m_scratchDecrements.emplace_back(x, y); });
				}
				it = m_staticAnchors.erase(it);
				continue;
			}

			int32 currentTileX, currentTileY;
			WorldToTile(it->pos.x, it->pos.y, header, currentTileX, currentTileY);

			if (currentTileX != it->lastTileX || currentTileY != it->lastTileY)
			{
				const STileRect newRect = ComputeRect(currentTileX, currentTileY, it->radius);
				const STileRect oldRect = it->lastTileX != INT32_MIN ? ComputeRect(it->lastTileX, it->lastTileY, it->radius) : STileRect{ 0, -1, 0, -1 };

				if (it->lastTileX != INT32_MIN)
					ForEachDiffTile(oldRect, newRect, [&](int32 x, int32 y) { m_scratchDecrements.emplace_back(x, y); });
				ForEachDiffTile(newRect, oldRect, [&](int32 x, int32 y) { m_scratchIncrements.emplace_back(x, y); });

				it->lastTileX = currentTileX;
				it->lastTileY = currentTileY;
			}

			++it;
		}
	}

	void CSpatialLoad::DispatchPendingRequests()
	{
		for (auto& req : m_pendingLoadings)
		{
			if (req->started || req->succeeded || req->abandoned)
				continue;

			SPendingTileLoad* pReq = req.get();
			pReq->started = true;

			gEnv->pJobManager->AddLambdaJob("LoadSpatialTile", [pReq]()
			{
				const STileEntry& entry = pReq->pTargetMap->GetTileEntry(pReq->tileIndex);
				pReq->succeeded = Maps::CMapFileReader::ReadTileRaw(pReq->pTargetMap->GetFilePath(), entry, pReq->pBuffer);
			}, JobManager::eRegularPriority, &pReq->jobState);
		}
	}

	void CSpatialLoad::WorldToTile(float worldX, float worldY, const SMapHeader& header, int32& outTileX, int32& outTileY) const
	{
		const float tileWorldSize = header.cellSize * header.tileSize;
		outTileX = static_cast<int32>((worldX - header.originX) / tileWorldSize);
		outTileY = static_cast<int32>((worldY - header.originY) / tileWorldSize);
	}

	CSpatialLoad::STileRect CSpatialLoad::ComputeRect(int32 tileX, int32 tileY, uint16 radius)
	{
		return { tileX - radius, tileX + radius, tileY - radius, tileY + radius };
	}

	bool CSpatialLoad::IncrementTileRef(int32 tileX, int32 tileY, SMapStreamingState& state)
	{
		const auto& header = state.pMap->GetHeader();
		if (tileX < 0 || tileX >= static_cast<int32>(header.tileCountX) || tileY < 0 || tileY >= static_cast<int32>(header.tileCountY))
			return true;

		const uint32 tileIndex = static_cast<uint32>(tileY) * header.tileCountX + static_cast<uint32>(tileX);
		uint32 refCount = state.tileRefCounts.Find(tileIndex);

		if (refCount != Core::Containers::kInvalidValue)
		{
			state.tileRefCounts.Insert(tileIndex, refCount + 1);
			return true;
		}

		uint8* pBuffer = nullptr;
		uint16 slot = state.pMap->ReserveTileSlot(tileIndex, &pBuffer);
		if (slot == Maps::kInvalidSlot)
			return false;

		state.tileRefCounts.Insert(tileIndex, 1);
		m_pendingLoadings.emplace_back(std::make_unique<SPendingTileLoad>(tileIndex, slot, pBuffer, false, false, false, state.pMap));
		return true;
	}

	void CSpatialLoad::DecrementTileRef(int32 tileX, int32 tileY, SMapStreamingState& state)
	{
		const auto& header = state.pMap->GetHeader();
		if (tileX < 0 || tileX >= static_cast<int32>(header.tileCountX) || tileY < 0 || tileY >= static_cast<int32>(header.tileCountY))
			return;

		const uint32 tileIndex = static_cast<uint32>(tileY) * header.tileCountX + static_cast<uint32>(tileX);
		uint32 refCount = state.tileRefCounts.Find(tileIndex);

		if (refCount != Core::Containers::kInvalidValue)
		{
			if (refCount != 1)
				state.tileRefCounts.Insert(tileIndex, refCount - 1);
			else
			{
				state.tileRefCounts.Remove(tileIndex);

				if (!state.pMap->IsTileLoaded(tileIndex))
					MarkRequestAbandoned(tileIndex, state.pMap);
				else
					state.pMap->ReleaseTile(tileIndex);
			}
		}
	}

	void CSpatialLoad::MarkRequestAbandoned(uint32 tileIndex, Maps::CBaseSpatialMap* pMap)
	{
		for (auto& req : m_pendingLoadings)
			if (req->pTargetMap == pMap && req->tileIndex == tileIndex)
				{ req->abandoned = true; return; }
	}

	template<typename Func>
	void CSpatialLoad::ForEachDiffTile(const STileRect& a, const STileRect& b, Func&& callback)
	{
		const int32 ox0 = std::max(a.minX, b.minX), ox1 = std::min(a.maxX, b.maxX);
		const int32 oy0 = std::max(a.minY, b.minY), oy1 = std::min(a.maxY, b.maxY);
		const bool hasOverlap = ox0 <= ox1 && oy0 <= oy1;

		if (!hasOverlap)
		{
			for (int32 y = a.minY; y <= a.maxY; ++y)
				for (int32 x = a.minX; x <= a.maxX; ++x)
					callback(x, y);
			return;
		}

		// Top
		for (int32 y = a.minY; y < oy0; ++y)
			for (int32 x = a.minX; x <= a.maxX; ++x)
				callback(x, y);

		// Bottom
		for (int32 y = oy1 + 1; y <= a.maxY; ++y)
			for (int32 x = a.minX; x <= a.maxX; ++x)
				callback(x, y);

		for (int32 y = oy0; y <= oy1; ++y)
		{
			//Left
			for (int32 x = a.minX; x < ox0; ++x)
				callback(x, y);
			//Right
			for (int32 x = ox1 + 1; x <= a.maxX; ++x)
				callback(x, y);
		}
	}
}