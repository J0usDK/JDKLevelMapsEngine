#pragma once
#include <CryThreading/IJobManager.h>

#include "ILoadingStrategy.h"
#include "Runtime/Maps/Base/BaseSpatialMap.h"
#include "Core/Containers/OpenAddressTable.h"
#include "Core/Streaming/AnchorProcessor.h"
#include "Core/Streaming/TileStreamer.h"

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
		void RegisterDynamicAnchor(EMapType targetMap, const Streaming::IMapAnchor* pAnchor, uint16 radius) override;
		void UnregisterDynamicAnchor(EMapType targetMap, const Streaming::IMapAnchor* pAnchor) override;

		// Registers anchor for objects or entities that have constant position
		Streaming::TStaticAnchorID RegisterPointAnchor(EMapType targetMap, Vec3 anchorPos, uint16 radius) override;
		void UnregisterPointAnchor(EMapType targetMap, Streaming::TStaticAnchorID anchorId) override;
		void UpdatePointAnchor(EMapType targetMap, Streaming::TStaticAnchorID anchorId, Vec3 pos) override;

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

			explicit SMapStreamingState(Maps::CBaseSpatialMap* map) : pMap(map), anchorProcessor(map->GetHeader(), map->GetTileWorldSizeInv()) { }

			void InitTables(uint32 capacity)
			{
				tileRefCounts.Initialize(capacity);
				activeJobs.Initialize(capacity);
			}
		};

		struct SEarlyDynamic
		{
			const Streaming::IMapAnchor* pAnchor;
			uint16 radius;
			EMapType targetMap;

			SEarlyDynamic(EMapType target, const Streaming::IMapAnchor* pAnchor, uint16 r) : targetMap(target), pAnchor(pAnchor), radius(r) {}
		};

		struct SEarlyStatic
		{
			Vec3 pos;
			Streaming::TStaticAnchorID id;
			uint16 radius;
			EMapType targetMap;

			SEarlyStatic(EMapType target, Streaming::TStaticAnchorID id, Vec3 pos, uint16 r) : targetMap(target), id(id), pos(pos), radius(r) {}
		};

	private:
		SMapStreamingState* FindStreamingState(EMapType targetMap);

		void LoadMapsAsync(Maps::Database::CMapsDatabase& db, const string& directory);

		void AllocatePools(const Maps::Database::CMapsDatabase& db);
		void InitializeMapStates(const Maps::Database::CMapsDatabase& db, uint32& outTotalBudget, uint32& outMaxBudget);
		void AllocateGlobalBuffers(uint32 totalBudget, uint32 maxBudget);

		std::unique_ptr<Maps::ILevelMap> LoadMapInternal(const string& filePath);
		std::unique_ptr<Maps::ILevelMap> TryConstructMap(SMapHeader&& header, const string& filePath) const;

		void ProcessDeferred(SMapStreamingState& state);
		void FlushStreamingState(SMapStreamingState& state);

		bool IncrementTileRef(uint32 tileIndex, SMapStreamingState& map);
		void DecrementTileRef(uint32 tileIndex, SMapStreamingState& map);

		bool QueueTileLoad(uint32 tileIndex, SMapStreamingState& state);
		void ReleaseOrCancelTile(uint32 tileIndex, SMapStreamingState& state);

	private:
		Streaming::TStaticAnchorID m_nextStaticAnchorID = 0;
		std::vector<SEarlyDynamic> m_pendingDynamicAnchors;
		std::vector<SEarlyStatic> m_pendingStaticAnchors;

		std::vector<SMapStreamingState> m_streamingStates;
		std::array<SMapStreamingState*, static_cast<size_t>(EMapType::Count)> m_stateLookup{};

		std::vector<uint32> m_scratchIncrements;
		std::vector<uint32> m_scratchDecrements;

		Streaming::CTileStreamer m_streamer;
	};

}