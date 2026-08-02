#pragma once
#include <vector>
#include <CryMath/Cry_Math.h>

#include "Shared/MapHeader.h"
#include "Shared/IMapAnchor.h"

namespace JDKLevelMaps::Streaming
{
	using TStaticAnchorID = uint32;

	class CAnchorProcessor final
	{
	public:
		CAnchorProcessor(const SMapHeader& header, float tileWorldSizeInv);

		void RegisterDynamicAnchor(const IMapAnchor* pAnchor, uint16 radius);
		void UnregisterDynamicAnchor(const IMapAnchor* pAnchor);

		void RegisterPointAnchor(TStaticAnchorID id, Vec3 anchorPos, uint16 radius);
		void UnregisterPointAnchor(TStaticAnchorID id);
		void UpdateStaticAnchor(TStaticAnchorID id, Vec3 pos);

		void CalculateTileDiffs(std::vector<uint32>& outIncrements, std::vector<uint32>& outDecrements);
		uint32 ComputeTileBudget() const;

	private:
		struct SAnchorBase
		{
			int32 lastTileX = INT32_MIN;
			int32 lastTileY = INT32_MIN;
			uint16 radius = 0;
			bool bPendingRemoval = false;

			SAnchorBase() = default;
			SAnchorBase(uint16 r, int32 ltX, int32 ltY) : radius(r), lastTileX(ltX), lastTileY(ltY) {}
		};

		struct SDynamicAnchor : public SAnchorBase
		{
			const Streaming::IMapAnchor* pAnchor = nullptr;

			SDynamicAnchor() {}
			SDynamicAnchor(const Streaming::IMapAnchor* pAnchor, uint16 r, int32 ltX, int32 ltY)
				: SAnchorBase(r, ltX, ltY), pAnchor(pAnchor) {
			}

			inline void GetWorldPos(float& x, float& y) const { pAnchor->GetPosition(x, y); }
		};

		struct SStaticAnchor : public SAnchorBase
		{
			Streaming::TStaticAnchorID id = 0;
			Vec3 pos;

			SStaticAnchor() {}
			SStaticAnchor(Streaming::TStaticAnchorID id, Vec3&& pos, uint16 r, int32 ltX, int32 ltY)
				: SAnchorBase(r, ltX, ltY), id(id), pos(std::move(pos)) {
			}

			inline void GetWorldPos(float& x, float& y) const { x = pos.x; y = pos.y; }
		};

		struct STileRect
		{
			int32 minX = INT32_MIN, maxX = INT32_MIN, minY = INT32_MIN, maxY = INT32_MIN;

			constexpr STileRect() {}
			constexpr STileRect(int32 minX, int32 maxX, int32 minY, int32 maxY) : minX(minX), maxX(maxX), minY(minY), maxY(maxY) {}

			static constexpr STileRect Empty() { return { 0, -1, 0, -1 }; }
		};

	private:
		template<typename TAnchor>
		void ProcessRemovals(std::vector<TAnchor>& anchors, std::vector<uint32>& outDecrements);

		template<typename TAnchor>
		void UpdateAnchors(std::vector<TAnchor>& anchors, std::vector<uint32>& outIncrements, std::vector<uint32>& outDecrements);

		void AppendDiffTiles(const STileRect& a, const STileRect& b, uint32 mapWidth, std::vector<uint32>& outList);

		constexpr static STileRect ComputeRect(int32 tileX, int32 tileY, uint16 radius)
			{ return { tileX - radius, tileX + radius, tileY - radius, tileY + radius }; }

		constexpr static STileRect ClipRect(const STileRect& r, const SMapHeader& header)
		{
			const int32 maxTileX = static_cast<int32>(header.tileCountX);
			const int32 maxTileY = static_cast<int32>(header.tileCountY);

			if (r.maxX < 0 || r.minX >= maxTileX || r.maxY < 0 || r.minY >= maxTileY)
				return STileRect::Empty();

			return { std::max(0, r.minX), std::min(maxTileX - 1, r.maxX), std::max(0, r.minY), std::min(maxTileY - 1, r.maxY) };
		}

		inline static void WorldToTile(float worldX, float worldY, const SMapHeader& header, float tileWorldSizeInv, int32& outTileX, int32& outTileY)
		{
			outTileX = static_cast<int32>((worldX - header.originX) * tileWorldSizeInv);
			outTileY = static_cast<int32>((worldY - header.originY) * tileWorldSizeInv);
		}
	
	private:
		SMapHeader m_header;
		float m_tileWorldSizeInv;

		std::vector<SDynamicAnchor> m_dynamicAnchors;
		std::vector<SStaticAnchor> m_staticAnchors;
	};
}