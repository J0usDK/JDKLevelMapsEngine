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

		m_streamer.DispatchPendingRequests();
	}

	void CSpatialLoad::UnloadAll(Maps::Database::CMapsDatabase& db)
	{
		m_streamingStates.clear();
		m_stateLookup.fill(nullptr);

		m_pendingDynamicAnchors.clear();
		m_pendingStaticAnchors.clear();

		m_streamer.Reset();

		db.UnregisterAll();
	}

	CSpatialLoad::SMapStreamingState* CSpatialLoad::FindStreamingState(EMapType targetMap)
	{
		CRY_ASSERT_MESSAGE(static_cast<size_t>(targetMap) < static_cast<size_t>(EMapType::Count), "Invalid EMapType passed to SpatialLoad!");
		return m_stateLookup[static_cast<size_t>(targetMap)];
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

	void CSpatialLoad::AllocatePools(const Maps::Database::CMapsDatabase& db)
	{
		uint32 totalBudget = 0;
		uint32 maxMapBudget = 0;

		InitializeMapStates(db, totalBudget, maxMapBudget);

		std::vector<SEarlyDynamic>().swap(m_pendingDynamicAnchors);
		std::vector<SEarlyStatic>().swap(m_pendingStaticAnchors);

		AllocateGlobalBuffers(totalBudget, maxMapBudget);
	}

	void CSpatialLoad::InitializeMapStates(const Maps::Database::CMapsDatabase& db, uint32& outTotalBudget, uint32& outMaxBudget)
	{
		m_streamingStates.clear();
		m_streamingStates.reserve(db.GetMaps().size());

		for (const auto& pMap : db.GetMaps())
		{
			if (!pMap || !pMap->IsValid()) continue;
			Maps::CBaseSpatialMap* pSpatialMap = pMap->AsSpatialMap();
			if (!pSpatialMap) continue;

			const EMapType currentType = pSpatialMap->GetType();

			m_streamingStates.emplace_back(pSpatialMap);
			auto& state = m_streamingStates.back();
			m_stateLookup[static_cast<size_t>(currentType)] = &state;

			for (const auto& dyn : m_pendingDynamicAnchors)
				if (dyn.targetMap == currentType)
					state.anchorProcessor.RegisterDynamicAnchor(dyn.pAnchor, dyn.radius);
			for (const auto& stat : m_pendingStaticAnchors)
				if (stat.targetMap == currentType)
					state.anchorProcessor.RegisterPointAnchor(stat.id, stat.pos, stat.radius);

			const uint32 tileBudget = state.anchorProcessor.ComputeTileBudget();
			state.InitTables(tileBudget);
			pSpatialMap->SetMaximalCapacity(tileBudget, pSpatialMap->GetTileByteSize());

			outTotalBudget += tileBudget;
			outMaxBudget = std::max(outMaxBudget, tileBudget);
		}
	}

	void CSpatialLoad::AllocateGlobalBuffers(uint32 totalBudget, uint32 maxBudget)
	{
		m_scratchDecrements.reserve(maxBudget);
		m_scratchIncrements.reserve(maxBudget);

		m_streamer.Initialize(totalBudget);
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

	void CSpatialLoad::RegisterDynamicAnchor(EMapType targetMap, const Streaming::IMapAnchor* pAnchor, uint16 radius)
	{
		if (!pAnchor) return;

		if (auto* pState = FindStreamingState(targetMap))
		{
			pState->anchorProcessor.RegisterDynamicAnchor(pAnchor, radius);
			return;
		}

		m_pendingDynamicAnchors.emplace_back(targetMap, pAnchor, radius);
	}

	void CSpatialLoad::UnregisterDynamicAnchor(EMapType targetMap, const Streaming::IMapAnchor* pAnchor)
	{
		if (!pAnchor) return;

		if (auto* pState = FindStreamingState(targetMap))
		{
			pState->anchorProcessor.UnregisterDynamicAnchor(pAnchor);
			return;
		}

		for (size_t i = 0; i < m_pendingDynamicAnchors.size(); ++i)
		{
			if (m_pendingDynamicAnchors[i].targetMap == targetMap && m_pendingDynamicAnchors[i].pAnchor == pAnchor)
			{
				m_pendingDynamicAnchors[i] = m_pendingDynamicAnchors.back();
				m_pendingDynamicAnchors.pop_back();
				return;
			}
		}
	}

	Streaming::TStaticAnchorID CSpatialLoad::RegisterPointAnchor(EMapType targetMap, Vec3 anchorPos, uint16 radius)
	{
		const auto id = m_nextStaticAnchorID++;
		if (auto* pState = FindStreamingState(targetMap))
		{
			pState->anchorProcessor.RegisterPointAnchor(id, anchorPos, radius);
			return id;
		}

		m_pendingStaticAnchors.emplace_back(targetMap, id, anchorPos, radius);
		return id;
	}

	void CSpatialLoad::UnregisterPointAnchor(EMapType targetMap, Streaming::TStaticAnchorID anchorID)
	{
		if (auto* pState = FindStreamingState(targetMap))
		{
			pState->anchorProcessor.UnregisterPointAnchor(anchorID);
			return;
		}

		for (size_t i = 0; i < m_pendingStaticAnchors.size(); ++i)
		{
			if (m_pendingStaticAnchors[i].targetMap == targetMap && m_pendingStaticAnchors[i].id == anchorID)
			{
				m_pendingStaticAnchors[i] = m_pendingStaticAnchors.back();
				m_pendingStaticAnchors.pop_back();
				return;
			}
		}
	}

	void CSpatialLoad::UpdatePointAnchor(EMapType targetMap, Streaming::TStaticAnchorID anchorID, Vec3 pos)
	{
		if (auto* pState = FindStreamingState(targetMap))
		{
			pState->anchorProcessor.UpdateStaticAnchor(anchorID, pos);
			return;
		}

		for (auto& anchor : m_pendingStaticAnchors)
		{
			if (anchor.targetMap == targetMap && anchor.id == anchorID)
			{
				anchor.pos = pos;
				return;
			}
		}
	}

	void CSpatialLoad::PreUpdate()
	{
		m_streamer.ProcessCompletedRequests([this](EMapType mapType, uint32 tileIndex)
		{
			if (auto* pState = FindStreamingState(mapType))
				pState->activeJobs.Remove(tileIndex);
		});
	}

	void CSpatialLoad::PostUpdate(Maps::Database::CMapsDatabase& db)
	{
		for (auto& state : m_streamingStates)
			FlushStreamingState(state);

		m_streamer.DispatchPendingRequests();
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

		const uint32 poolSlot = m_streamer.QueueLoad(state.pMap, tileIndex, slot, pBuffer);
		state.activeJobs.Insert(tileIndex, poolSlot);

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
				m_streamer.CancelLoad(poolIdx);
				state.activeJobs.Remove(tileIndex);
			}
		}
	}
}