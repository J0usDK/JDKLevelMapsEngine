#include "StdAfx.h"
#include "TileStreamer.h"

#include "Bootstrap/Reader/MapReader.h"
#include "Runtime/Maps/Base/BaseSpatialMap.h"

namespace JDKLevelMaps::Streaming
{
	void CTileStreamer::Initialize(uint32 totalBudget)
	{
		m_pendingLoads.resize(totalBudget);
		m_freeLoadingSlots.reserve(totalBudget);
		m_pendingDispatches.reserve(totalBudget);
		m_runningJobs.reserve(totalBudget);

		for (uint32 i = totalBudget; i > 0; --i)
			m_freeLoadingSlots.push_back(i - 1);
	}

	void CTileStreamer::Reset()
	{
		m_pendingLoads.clear();
		m_freeLoadingSlots.clear();
		m_pendingDispatches.clear();
		m_runningJobs.clear();
	}

	uint32 CTileStreamer::QueueLoad(Maps::CBaseSpatialMap* pMap, uint32 tileIndex, uint16 reservedSlot, uint8* pBuffer)
	{
		CRY_ASSERT_MESSAGE(!m_freeLoadingSlots.empty(), "[JDKLevelMaps] No free loading slots available for tile load request.");

		const uint32 poolSlot = m_freeLoadingSlots.back();
		m_freeLoadingSlots.pop_back();

		m_pendingLoads[poolSlot].Init(pMap, tileIndex, reservedSlot, pBuffer);
		m_pendingDispatches.push_back(poolSlot);

		return poolSlot;
	}

	void CTileStreamer::CancelLoad(uint32 poolIdx)
	{
		if (poolIdx < m_pendingLoads.size())
			m_pendingLoads[poolIdx].abandoned = true;
	}

	void CTileStreamer::DispatchPendingRequests()
	{
		for (uint32 poolIdx : m_pendingDispatches)
		{
			SPendingTileLoad* pReq = &m_pendingLoads[poolIdx];

			if (pReq->abandoned)
			{
				pReq->pMap->ReleaseTileSlotWithoutCommit(pReq->reservedSlot);
				m_freeLoadingSlots.push_back(poolIdx);
				continue;
			}
			m_runningJobs.push_back(poolIdx);

			gEnv->pJobManager->AddLambdaJob("LoadSpatialTile", [pReq]()
			{
				const STileEntry& entry = pReq->pMap->GetTileEntry(pReq->tileIndex);
				pReq->succeeded = Maps::CMapFileReader::ReadTileRaw(pReq->pMap->GetFilePath(), entry, pReq->pBuffer);

			}, JobManager::eRegularPriority, &pReq->jobState);
		}
		m_pendingDispatches.clear();
	}
}