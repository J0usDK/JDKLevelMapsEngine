#pragma once
#include "Runtime/Maps/Base/BaseSpatialMap.h"

namespace JDKLevelMaps::Streaming
{
	template<typename TCallback>
	void CTileStreamer::ProcessCompletedRequests(TCallback&& onRequestCompleted)
	{
		for (size_t i = 0; i < m_runningJobs.size();)
		{
			const uint32 poolIdx = m_runningJobs[i];
			SPendingTileLoad& req = m_pendingLoads[poolIdx];

			if (req.jobState.IsRunning())
			{
				++i;
				continue;
			}

			if (req.succeeded && !req.abandoned)
				req.pMap->CommitTile(req.tileIndex, req.reservedSlot);
			else
				req.pMap->ReleaseTileSlotWithoutCommit(req.reservedSlot);

			onRequestCompleted(req.pMap->GetType(), req.tileIndex);

			m_freeLoadingSlots.push_back(poolIdx);
			m_runningJobs[i] = m_runningJobs.back();
			m_runningJobs.pop_back();
		}
	}
}