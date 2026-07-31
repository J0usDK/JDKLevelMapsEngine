#pragma once
#include <CryThreading/IJobManager.h>

#include "ILoadingStrategy.h"
#include "Core/Containers/OpenAddressTable.h"

namespace JDKLevelMaps
{
	struct SMapHeader;
}

namespace JDKLevelMaps::Maps
{
	class ILevelMap;
	class CBaseSpatialMap;
	struct SLoadedMap;
}

namespace JDKLevelMaps::Streaming
{
	class IMapAnchor;
	using TStaticAnchorID = uint32;
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

			std::vector<std::pair<int32, int32>> deferredIncrements;

			explicit SMapStreamingState(Maps::CBaseSpatialMap* map, uint32 capacity) : pMap(map)
			{
				tileRefCounts.Initialize(capacity);
			}
		};

		struct SDynamicAnchor
		{
			const Streaming::IMapAnchor* pAnchor = nullptr;
			uint16 radius = 0;

			int32 lastTileX = INT32_MIN;
			int32 lastTileY = INT32_MIN;
			
			bool bPendingRemoval = false;

			SDynamicAnchor() {}
			SDynamicAnchor(const Streaming::IMapAnchor* pAnchor, uint16 r, int32 ltX, int32 ltY)
				: pAnchor(pAnchor), radius(r), lastTileX(ltX), lastTileY(ltY) { }
		};
		struct SStaticAnchor
		{
			Streaming::TStaticAnchorID id = 0;
			Vec3 pos;
			uint16 radius = 0;

			int32 lastTileX = INT32_MIN;
			int32 lastTileY = INT32_MIN;

			bool bPendingRemoval = false;

			SStaticAnchor() {}
			SStaticAnchor(Streaming::TStaticAnchorID id, Vec3&& pos, uint16 r, int32 ltX, int32 ltY)
				: id(id), pos(std::move(pos)), radius(r), lastTileX(ltX), lastTileY(ltY) {}
		};

		struct STileRect
		{ 
			int32 minX, maxX, minY, maxY; 

			STileRect() {}
			STileRect(int32 minX, int32 maxX, int32 minY, int32 maxY) : minX(minX), maxX(maxX), minY(minY), maxY(maxY) {}
		};

		struct SPendingTileLoad
		{
			JobManager::SJobState jobState;

			uint32 tileIndex = 0;
			uint16 reservedSlot = 0;
			uint8* pBuffer = nullptr;

			bool started = false;
			bool succeeded = false;
			bool abandoned = false;

			Maps::CBaseSpatialMap* pTargetMap = nullptr;

			SPendingTileLoad() { }
			SPendingTileLoad(uint32 idx, uint16 slot, uint8* pBuf, bool started, bool succeeded, bool abandoned, Maps::CBaseSpatialMap* pMap)
				: tileIndex(idx), reservedSlot(slot), pBuffer(pBuf), started(started), succeeded(succeeded), abandoned(abandoned), pTargetMap(pMap) { }
		};

	private:
		uint32 ComputeTileBudget(const SMapHeader& header) const;

		std::unique_ptr<Maps::ILevelMap> LoadMapInternal(const string& filePath);
		std::unique_ptr<Maps::ILevelMap> TryConstructMap(SMapHeader&& header, const string& filePath) const;

		void ProcessDynamicAnchors(SMapStreamingState& state);
		void ProcessStaticAnchors(SMapStreamingState& state);

		void DispatchPendingRequests();

		void WorldToTile(float worldX, float worldY, const SMapHeader& header, int32& outTileX, int32& outTileY) const;
		STileRect ComputeRect(int32 tileX, int32 tileY, uint16 radius);

		bool IncrementTileRef(int32 tileX, int32 tileY, SMapStreamingState& map);
		void DecrementTileRef(int32 tileX, int32 tileY, SMapStreamingState& map);

		void MarkRequestAbandoned(uint32 tileIndex, Maps::CBaseSpatialMap* pMap);

		template<typename Func>
		void ForEachDiffTile(const STileRect& a, const STileRect& b, Func&& callback);

	private:
		std::vector<SDynamicAnchor> m_dynamicAnchors;
		std::vector<SStaticAnchor> m_staticAnchors;

		Streaming::TStaticAnchorID m_nextStaticAnchorId = 0;

		std::vector<SMapStreamingState> m_streamingStates;

		std::vector<std::unique_ptr<SPendingTileLoad>> m_pendingLoadings;

		std::vector<std::pair<int32, int32>> m_scratchIncrements;
		std::vector<std::pair<int32, int32>> m_scratchDecrements;
	};

}