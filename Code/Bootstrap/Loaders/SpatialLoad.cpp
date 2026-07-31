#include "StdAfx.h"
#include "SpatialLoad.h"

#include "Shared/IMapAnchor.h"
#include "Runtime/Database/MapsDatabase.h"
#include "Runtime/Maps/Base/LoadedMap.h"
#include "Runtime/Maps/VegetationSpatialMap.h"
#include "Bootstrap/Reader/MapReader.h"

namespace JDKLevelMaps::Bootstrap
{
	void CSpatialLoad::Initialize(Maps::Database::CMapsDatabase& db, const string& directory)
	{
		LoadMapsAsync(db, directory);
		AllocatePools(db);

		for (auto& state : m_streamingStates)
			FlushStreamingState(state);

		if (!m_pendingDispatches.empty())
			DispatchPendingRequests();
	}

	void CSpatialLoad::UnloadAll(Maps::Database::CMapsDatabase& db)
	{
		m_streamingStates.clear();
		m_registeredDynamicAnchors.clear();
		m_registeredStaticAnchors.clear();

		m_loadingPool.clear();
		m_freeLoadingSlots.clear();
		m_pendingDispatches.clear();
		m_runningJobs.clear();

		db.UnregisterAll();
	}

	void CSpatialLoad::LoadMapsAsync(Maps::Database::CMapsDatabase& db, const string& directory)
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

	uint32 CSpatialLoad::ComputeTileBudget(const SMapHeader& header) const
	{
		const uint64 totalTiles = static_cast<uint64>(header.tileCountX) * header.tileCountY;
		uint64 budget = 0;

		auto addBudget = [&](const auto& anchorList) -> bool
		{
			for (const auto& anchor : anchorList)
			{
				const uint64 side = static_cast<uint64>(anchor.radius) * 2 + 1;
				budget += side * side;
				if (budget >= totalTiles)
					return true;
			}
			return false;
		};

		if (addBudget(m_registeredDynamicAnchors)) return static_cast<uint32>(totalTiles);
		if (addBudget(m_registeredStaticAnchors)) return static_cast<uint32>(totalTiles);

		return static_cast<uint32>(budget);
	}

	void CSpatialLoad::AllocatePools(const Maps::Database::CMapsDatabase& db)
	{
		m_streamingStates.clear();
		uint32 totalBudget = 0;
		uint32 maxMapBudget = 0;

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

			auto& state = m_streamingStates.back();
			for (const auto& dyn : m_registeredDynamicAnchors)
				state.anchorProcessor.RegisterDynamicAnchor(dyn.pAnchor, dyn.radius);
			for (const auto& stat : m_registeredStaticAnchors)
				state.anchorProcessor.RegisterPointAnchor(stat.id, stat.pos, stat.radius);

			totalBudget += tileBudget;
			maxMapBudget = std::max(maxMapBudget, tileBudget);
		}

		m_scratchDecrements.reserve(maxMapBudget);
		m_scratchIncrements.reserve(maxMapBudget);

		m_loadingPool.resize(totalBudget);
		m_freeLoadingSlots.reserve(totalBudget);
		m_pendingDispatches.reserve(totalBudget);
		m_runningJobs.reserve(totalBudget);

		for (uint32 i = totalBudget; i > 0; --i)
			m_freeLoadingSlots.push_back(i - 1);
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
		m_registeredDynamicAnchors.emplace_back(pAnchor, radius);
		for (auto& state : m_streamingStates)
			state.anchorProcessor.RegisterDynamicAnchor(pAnchor, radius);
	}

	void CSpatialLoad::UnregisterDynamicAnchor(const Streaming::IMapAnchor* pAnchor)
	{
		for (size_t i = 0; i < m_registeredDynamicAnchors.size(); ++i)
		{
			if (m_registeredDynamicAnchors[i].pAnchor == pAnchor)
			{
				m_registeredDynamicAnchors[i] = m_registeredDynamicAnchors.back();
				m_registeredDynamicAnchors.pop_back();
				break;
			}
		}

		for (auto& state : m_streamingStates)
			state.anchorProcessor.UnregisterDynamicAnchor(pAnchor);
	}

	Streaming::TStaticAnchorID CSpatialLoad::RegisterPointAnchor(Vec3 anchorPos, uint16 radius)
	{
		const auto id = m_nextStaticAnchorID++;
		m_registeredStaticAnchors.emplace_back(id, anchorPos, radius);

		for (auto& state : m_streamingStates)
			state.anchorProcessor.RegisterPointAnchor(id, anchorPos, radius);

		return id;
	}

	void CSpatialLoad::UnregisterPointAnchor(Streaming::TStaticAnchorID anchorID)
	{
		for (size_t i = 0; i < m_registeredStaticAnchors.size(); ++i)
		{
			if (m_registeredStaticAnchors[i].id == anchorID)
			{
				m_registeredStaticAnchors[i] = m_registeredStaticAnchors.back();
				m_registeredStaticAnchors.pop_back();
				break;
			}
		}

		for (auto& state : m_streamingStates)
			state.anchorProcessor.UnregisterPointAnchor(anchorID);
	}

	void CSpatialLoad::UpdatePointAnchor(Streaming::TStaticAnchorID anchorID, Vec3 pos)
	{
		for (auto& anchor : m_registeredStaticAnchors)
		{
			if (anchor.id == anchorID)
			{
				anchor.pos = pos;
				break;
			}
		}

		for (auto& state : m_streamingStates)
			state.anchorProcessor.UpdateStaticAnchor(anchorID, pos);
	}

	void CSpatialLoad::PreUpdate()
	{
		for (size_t i = 0; i < m_runningJobs.size();)
		{
			const uint32 poolIdx = m_runningJobs[i];
			SPendingTileLoad& req = m_loadingPool[poolIdx];

			if (req.jobState.IsRunning())
			{
				++i;
				continue;
			}

			if (req.succeeded && !req.abandoned)
				req.pState->pMap->CommitTile(req.tileIndex, req.reservedSlot);
			else
				req.pState->pMap->ReleaseTileSlotWithoutCommit(req.reservedSlot);

			req.pState->activeJobs.Remove(req.tileIndex);
			m_freeLoadingSlots.push_back(poolIdx);
			m_runningJobs[i] = m_runningJobs.back();
			m_runningJobs.pop_back();
		}
	}

	void CSpatialLoad::PostUpdate(Maps::Database::CMapsDatabase& db)
	{
		for (auto& state : m_streamingStates)
			FlushStreamingState(state);

		if (!m_pendingDispatches.empty())
			DispatchPendingRequests();
	}

	void CSpatialLoad::ProcessDeferred(SMapStreamingState& state)
	{
		for (size_t i = 0; i < state.deferredIncrements.size();)
		{
			if (IncrementTileRef(state.deferredIncrements[i], state))
			{
				state.deferredIncrements[i] = state.deferredIncrements.back();
				state.deferredIncrements.pop_back();
			}
			else ++i;
		}
	}

	void CSpatialLoad::FlushStreamingState(SMapStreamingState& state)
	{
		m_scratchIncrements.clear();
		m_scratchDecrements.clear();

		ProcessDeferred(state);

		state.anchorProcessor.CalculateTileDiffs(m_scratchIncrements, m_scratchDecrements);

		for (uint32 tileIndex : m_scratchIncrements)
			if (!IncrementTileRef(tileIndex, state))
				state.deferredIncrements.push_back(tileIndex);

		for (uint32 tileIndex : m_scratchDecrements)
			DecrementTileRef(tileIndex, state);

		state.pMap->FlushPendingMaintenance();
		state.tileRefCounts.FlushRebuild();
		state.activeJobs.FlushRebuild();
	}

	void CSpatialLoad::DispatchPendingRequests()
	{
		for (uint32 poolIdx : m_pendingDispatches)
		{
			SPendingTileLoad* pReq = &m_loadingPool[poolIdx];

			if (pReq->abandoned)
			{
				pReq->pState->pMap->ReleaseTileSlotWithoutCommit(pReq->reservedSlot);
				m_freeLoadingSlots.push_back(poolIdx);
				continue;
			}
			m_runningJobs.push_back(poolIdx);

			gEnv->pJobManager->AddLambdaJob("LoadSpatialTile", [pReq]()
			{
				const STileEntry& entry = pReq->pState->pMap->GetTileEntry(pReq->tileIndex);
				pReq->succeeded = Maps::CMapFileReader::ReadTileRaw(pReq->pState->pMap->GetFilePath(), entry, pReq->pBuffer);
			}, JobManager::eRegularPriority, &pReq->jobState);
		}
		m_pendingDispatches.clear();
	}

	bool CSpatialLoad::IncrementTileRef(uint32 tileIndex, SMapStreamingState& state)
	{
		uint32* pRefCount = state.tileRefCounts.GetValuePtr(tileIndex);
		if (pRefCount)
		{
			(*pRefCount)++;
			return true;
		}
		return QueueTileLoad(tileIndex, state);
	}

	void CSpatialLoad::DecrementTileRef(uint32 tileIndex, SMapStreamingState& state)
	{
		uint32* pRefCount = state.tileRefCounts.GetValuePtr(tileIndex);

		if (pRefCount)
		{
			if (*pRefCount > 1)
				(*pRefCount)--;
			else
				ReleaseOrCancelTile(tileIndex, state);
		}
	}

	bool CSpatialLoad::QueueTileLoad(uint32 tileIndex, SMapStreamingState& state)
	{
		uint8* pBuffer = nullptr;
		uint16 slot = state.pMap->ReserveTileSlot(tileIndex, &pBuffer);
		if (slot == Maps::kInvalidSlot)
			return false;

		state.tileRefCounts.Insert(tileIndex, 1);

		CRY_ASSERT_MESSAGE(!m_freeLoadingSlots.empty(), "[JDKLevelMaps] Out of loading slots. Budget calculation failed.");
		const uint32 poolSlot = m_freeLoadingSlots.back();
		m_freeLoadingSlots.pop_back();

		m_loadingPool[poolSlot].Init(&state, tileIndex, slot, pBuffer);

		state.activeJobs.Insert(tileIndex, poolSlot);
		m_pendingDispatches.push_back(poolSlot);
		return true;
	}

	void CSpatialLoad::ReleaseOrCancelTile(uint32 tileIndex, SMapStreamingState& state)
	{
		state.tileRefCounts.Remove(tileIndex);

		if (state.pMap->IsTileLoaded(tileIndex))
			state.pMap->ReleaseTile(tileIndex);
		else
		{
			const uint32 poolIdx = state.activeJobs.Find(tileIndex);
			if (poolIdx != Core::Containers::kInvalidValue)
			{
				m_loadingPool[poolIdx].abandoned = true;
				state.activeJobs.Remove(tileIndex);
			}
		}
	}
}