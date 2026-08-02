#include "StdAfx.h"
#include "AnchorProcessor.h"

namespace JDKLevelMaps::Streaming
{
	CAnchorProcessor::CAnchorProcessor(const SMapHeader& header, float tileWorldSizeInv) : m_header(header), m_tileWorldSizeInv(tileWorldSizeInv)
	{

	}

	void CAnchorProcessor::RegisterDynamicAnchor(const IMapAnchor* pAnchor, uint16 radius)
	{
		if (!pAnchor) return;
		m_dynamicAnchors.push_back({ pAnchor, radius, INT32_MIN, INT32_MIN });
	}

	void CAnchorProcessor::UnregisterDynamicAnchor(const IMapAnchor* pAnchor)
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

	void CAnchorProcessor::RegisterPointAnchor(TStaticAnchorID id, Vec3 anchorPos, uint16 radius)
	{
		m_staticAnchors.emplace_back(id, std::move(anchorPos), radius, INT32_MIN, INT32_MIN);
	}
	void CAnchorProcessor::UnregisterPointAnchor(TStaticAnchorID id)
	{
		for (auto& anchor : m_staticAnchors)
		{
			if (anchor.id == id)
			{
				anchor.bPendingRemoval = true;
				break;
			}
		}
	}

	void CAnchorProcessor::UpdateStaticAnchor(TStaticAnchorID id, Vec3 pos)
	{
		for (auto& anchor : m_staticAnchors)
		{
			if (anchor.id == id)
			{
				anchor.pos = std::move(pos);
				break;
			}
		}
	}

	void CAnchorProcessor::CalculateTileDiffs(std::vector<uint32>& outIncrements, std::vector<uint32>& outDecrements)
	{
		// Remove pending anchors first to avoid updating objects scheduled for deletion this frame
		ProcessRemovals(m_dynamicAnchors, outDecrements);
		UpdateAnchors(m_dynamicAnchors, outIncrements, outDecrements);

		ProcessRemovals(m_staticAnchors, outDecrements);
		UpdateAnchors(m_staticAnchors, outIncrements, outDecrements);
	}

	uint32 CAnchorProcessor::ComputeTileBudget() const
	{
		const uint64 totalTiles = static_cast<uint64>(m_header.tileCountX) * m_header.tileCountY;
		uint64 budget = 0;

		auto addBudget = [&](const auto& anchorList) -> bool
		{
			for (const auto& anchor : anchorList)
			{
				const uint64 side = static_cast<uint64>(anchor.radius) * 2 + 1;
				budget += side * side;
				if (budget >= totalTiles) return true;
			}
			return false;
		};

		if (addBudget(m_dynamicAnchors)) return static_cast<uint32>(totalTiles);
		if (addBudget(m_staticAnchors)) return static_cast<uint32>(totalTiles);

		return static_cast<uint32>(budget);
	}

	template<typename TAnchor>
	void CAnchorProcessor::ProcessRemovals(std::vector<TAnchor>& anchors, std::vector<uint32>& outDecrements)
	{
		for (size_t i = 0; i < anchors.size();)
		{
			auto& anchor = anchors[i];
			if (anchor.bPendingRemoval)
			{
				if (anchor.lastTileX != INT32_MIN)
				{
					const STileRect oldRect = ClipRect(ComputeRect(anchor.lastTileX, anchor.lastTileY, anchor.radius), m_header);
					AppendDiffTiles(oldRect, STileRect::Empty(), m_header.tileCountX, outDecrements);
				}

				anchors[i] = std::move(anchors.back());
				anchors.pop_back();
			}
			else ++i;
		}
	}

	template<typename TAnchor>
	void CAnchorProcessor::UpdateAnchors(std::vector<TAnchor>& anchors, std::vector<uint32>& outIncrements, std::vector<uint32>& outDecrements)
	{
		for (auto& anchor : anchors)
		{
			float worldX, worldY;
			anchor.GetWorldPos(worldX, worldY);

			int32 currentTileX, currentTileY;
			WorldToTile(worldX, worldY, m_header, m_tileWorldSizeInv, currentTileX, currentTileY);

			if (currentTileX != anchor.lastTileX || currentTileY != anchor.lastTileY)
			{
				const STileRect newRect = ClipRect(ComputeRect(currentTileX, currentTileY, anchor.radius), m_header);
				const STileRect oldRect = anchor.lastTileX != INT32_MIN
					? ClipRect(ComputeRect(anchor.lastTileX, anchor.lastTileY, anchor.radius), m_header)
					: STileRect::Empty();

				if (anchor.lastTileX != INT32_MIN)
					AppendDiffTiles(oldRect, newRect, m_header.tileCountX, outDecrements);
				AppendDiffTiles(newRect, oldRect, m_header.tileCountX, outIncrements);

				anchor.lastTileX = currentTileX;
				anchor.lastTileY = currentTileY;
			}
		}
	}

	void CAnchorProcessor::AppendDiffTiles(const STileRect& a, const STileRect& b, uint32 mapWidth, std::vector<uint32>& outList)
	{
		const int32 ox0 = std::max(a.minX, b.minX), ox1 = std::min(a.maxX, b.maxX);
		const int32 oy0 = std::max(a.minY, b.minY), oy1 = std::min(a.maxY, b.maxY);
		const bool hasOverlap = ox0 <= ox1 && oy0 <= oy1;

		const int32 areaA = (a.maxX - a.minX + 1) * (a.maxY - a.minY + 1);
		const int32 areaOverlap = hasOverlap ? ((ox1 - ox0 + 1) * (oy1 - oy0 + 1)) : 0;
		const int32 diffArea = areaA - areaOverlap;

		if (diffArea <= 0)
			return;

		const size_t oldSize = outList.size();
		outList.resize(oldSize + diffArea);

		uint32* pDest = outList.data() + oldSize;

		if (!hasOverlap)
		{
			for (int32 y = a.minY; y <= a.maxY; ++y)
			{
				const uint32 baseIdx = static_cast<uint32>(y) * mapWidth;
				for (int32 x = a.minX; x <= a.maxX; ++x)
					*pDest++ = baseIdx + static_cast<uint32>(x);
			}
			return;
		}

		// Top
		for (int32 y = a.minY; y < oy0; ++y)
		{
			uint32 currentIdx = static_cast<uint32>(y) * mapWidth + static_cast<uint32>(a.minX);
			for (int32 x = a.minX; x <= a.maxX; ++x)
				*pDest++ = currentIdx++;
		}

		//	Middle (Left && Right)
		for (int32 y = oy0; y <= oy1; ++y)
		{
			const uint32 rowBase = static_cast<uint32>(y) * mapWidth;

			uint32 leftIdx = rowBase + static_cast<uint32>(a.minX);
			for (int32 x = a.minX; x < ox0; ++x)
				*pDest++ = leftIdx++;

			uint32 rightIdx = rowBase + static_cast<uint32>(ox1 + 1);
			for (int32 x = ox1 + 1; x <= a.maxX; ++x)
				*pDest++ = rightIdx++;
		}

		for (int32 y = oy1 + 1; y <= a.maxY; ++y)
		{
			uint32 currentIdx = static_cast<uint32>(y) * mapWidth + static_cast<uint32>(a.minX);
			for (int32 x = a.minX; x <= a.maxX; ++x)
				*pDest++ = currentIdx++;
		}
	}
}