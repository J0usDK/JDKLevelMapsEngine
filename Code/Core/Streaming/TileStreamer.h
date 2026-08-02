#pragma once
#include <CryThreading/IJobManager.h>

namespace JDKLevelMaps::Maps
{
	class CBaseSpatialMap;
}

namespace JDKLevelMaps::Streaming
{
	class CTileStreamer final
	{
	public:
		void Initialize(uint32 totalBudget);
		void Reset();

		uint32 QueueLoad(Maps::CBaseSpatialMap* pMap, uint32 tileIndex, uint16 reservedSlot, uint8* pBuffer);

		void CancelLoad(uint32 poolIdx);
		void DispatchPendingRequests();

		template<typename TCallback>
		void ProcessCompletedRequests(TCallback&& onRequestCompleted);

	private:
		struct SPendingTileLoad
		{
			Maps::CBaseSpatialMap* pMap = nullptr;
			JobManager::SJobState jobState;

			uint8* pBuffer = nullptr;
			uint32 tileIndex = 0;
			uint16 reservedSlot = 0;

			bool succeeded = false;
			bool abandoned = false;

			void Init(Maps::CBaseSpatialMap* map, uint32 idx, uint16 slot, uint8* pBuf)
				{ pMap = map; tileIndex = idx; reservedSlot = slot; pBuffer = pBuf; succeeded = false; abandoned = false; }
		};

	private:
		std::vector<SPendingTileLoad> m_pendingLoads;
		std::vector<uint32> m_freeLoadingSlots;
		std::vector<uint32> m_pendingDispatches;
		std::vector<uint32> m_runningJobs;
	};
}

#include "TileStreamer.inl"