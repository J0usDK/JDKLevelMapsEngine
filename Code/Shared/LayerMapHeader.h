#pragma once
#include <CryCore/BaseTypes.h>

namespace JDKLevelMaps
{
	constexpr uint16 kLayerMapVersion = 1;
	constexpr uint32 kLayerMapMagic = 0x50414D4C; // 'LMAP'

	enum class ELayerMapType : uint16
	{
		VegetationDensity = 0
	};

#pragma pack(push, 1)
	struct SLayerMapHeader
	{
		uint32 magic = kLayerMapMagic;
		uint16 version = kLayerMapVersion;
		ELayerMapType mapType = ELayerMapType::VegetationDensity;
		int32 gridWidth = 0;
		int32 gridHeight = 0;
		float cellSize = 0.0f;	//meters per cell
		float originX = 0.0f;
		float originY = 0.0f;
	};
#pragma pack(pop)

	static_assert(sizeof(SLayerMapHeader) == 28,
		"The size of SLayerMapHeader has been changed. Keep the in-game reader up to date.");
}