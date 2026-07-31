#pragma once
#include <CryThreading/IJobManager.h>

#include "ILoadingStrategy.h"
#include "Runtime/Maps/Base/BaseSpatialMap.h"
#include "Core/Containers/OpenAddressTable.h"
#include "Core/AnchorProcessor.h"

namespace JDKLevelMaps::Maps
{
	class ILevelMap;
	class CBaseSpatialMap;
	struct SLoadedMap;
}

namespace JDKLevelMaps::Streaming
{
	class IMapAnchor;
}

namespace JDKLevelMaps::Bootstrap
{
	class CSpatialLoad final : public ILoadingStrategy
	{
	public:
		CSpatialLoad() = default;
		~CSpatialLoad() = default;

		void Initialize(Maps::Database::CMapsDatabase& db, const string& directory) override;
		void UnloadAll(Maps::Database::CMapsDatabase& db) override;

		// Registers anchor for objects or entities that can change their position
		void RegisterDynamicAnchor(const Streaming::IMapAnchor* pAnchor, uint16 radius) override;
		void UnregisterDynamicAnchor(const Streaming::IMapAnchor* pAnchor) override;

		// Registers anchor for objects or entities that have constant position
		Streaming::TStaticAnchorID RegisterPointAnchor(Vec3 anchorPos, uint16 radius) override;
		void UnregisterPointAnchor(Streaming::TStaticAnchorID anchorId) override;
		void UpdatePointAnchor(Streaming::TStaticAnchorID anchorId, Vec3 pos) override;

		void PreUpdate() override;
		void PostUpdate(Maps::Database::CMapsDatabase& db) override;

	private:
		struct SMapStreamingState
		{
			Maps::CBaseSpatialMap* pMap = nullptr;
			Core::Containers::COpenAddressTable tileRefCounts;
			Core::Containers::COpenAddressTable activeJobs;
			std::vector<uint32> deferredIncrements;

			Streaming::CAnchorProcessor anchorProcessor;

			explicit SMapStreamingState(Maps::CBaseSpatialMap* map, uint32 capacity) : pMap(map), anchorProcessor(map->GetHeader(), map->GetTileWorldSizeInv())
			{
				tileRefCounts.Initialize(capacity);
				activeJobs.Initialize(capacity);
			}
		};

		struct SPendingTileLoad
		{
			uint8* pBuffer = nullptr;
			SMapStreamingState* pState = nullptr;
			JobManager::SJobState jobState;

			uint32 tileIndex = 0;
			uint16 reservedSlot = 0;

			bool succeeded = false;
			bool abandoned = false;

			void Init(SMapStreamingState* pState, uint32 idx, uint16 slot, uint8* pBuf)
				{ this->pState = pState; tileIndex = idx; reservedSlot = slot; pBuffer = pBuf; succeeded = false; abandoned = false; }
		};

		struct SRegisteredDynamic
		{
			const Streaming::IMapAnchor* pAnchor;
			uint16 radius;

			SRegisteredDynamic(const Streaming::IMapAnchor* pAnchor, uint16 r) : pAnchor(pAnchor), radius(r) {}
		};

		struct SRegisteredStatic
		{
			Streaming::TStaticAnchorID id;
			Vec3 pos;
			uint16 radius;

			SRegisteredStatic(Streaming::TStaticAnchorID id, Vec3 pos, uint16 r) : id(id), pos(pos), radius(r) {}
		};

	private:
		void LoadMapsAsync(Maps::Database::CMapsDatabase& db, const string& directory);
		uint32 ComputeTileBudget(const SMapHeader& header) const;
		void AllocatePools(const Maps::Database::CMapsDatabase& db);

		std::unique_ptr<Maps::ILevelMap> LoadMapInternal(const string& filePath);
		std::unique_ptr<Maps::ILevelMap> TryConstructMap(SMapHeader&& header, const string& filePath) const;

		void ProcessDeferred(SMapStreamingState& state);
		void FlushStreamingState(SMapStreamingState& state);
		void DispatchPendingRequests();

		bool IncrementTileRef(uint32 tileIndex, SMapStreamingState& map);
		void DecrementTileRef(uint32 tileIndex, SMapStreamingState& map);

		bool QueueTileLoad(uint32 tileIndex, SMapStreamingState& state);
		void ReleaseOrCancelTile(uint32 tileIndex, SMapStreamingState& state);

	private:
		Streaming::TStaticAnchorID m_nextStaticAnchorID = 0;
		std::vector<SRegisteredDynamic> m_registeredDynamicAnchors;
		std::vector<SRegisteredStatic> m_registeredStaticAnchors;

		std::vector<SMapStreamingState> m_streamingStates;

		std::vector<SPendingTileLoad> m_loadingPool;
		std::vector<uint32> m_freeLoadingSlots;
		std::vector<uint32> m_pendingDispatches;
		std::vector<uint32> m_runningJobs;

		std::vector<uint32> m_scratchIncrements;
		std::vector<uint32> m_scratchDecrements;
	};

}