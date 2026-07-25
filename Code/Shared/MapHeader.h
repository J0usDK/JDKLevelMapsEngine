#pragma once
#include <CryCore/BaseTypes.h>

namespace JDKLevelMaps
{
	constexpr uint16 kLayerMapVersion = 2;
	constexpr uint32 kLayerMapMagic = 'JDKM';

	enum class EMapType : uint16
	{
		VegetationDensity = 0
	};

#pragma pack(push, 1)
	struct STileEntry
	{
		uint64 fileOffset = 0;
		uint32 byteSize = 0;
		uint32 flags = 0;
	};
#pragma pack(pop)

	static_assert(sizeof(STileEntry) == 16,
		"The size of STileEntry has been changed. Keep the in-game reader up to date.");

#pragma pack(push, 1)
	struct SMapHeader
	{
		uint32 magic = kLayerMapMagic;
		uint16 version = kLayerMapVersion;
		EMapType mapType = EMapType::VegetationDensity;
		int32 gridWidth = 0;
		int32 gridHeight = 0;
		float cellSize = 0.0f;	//meters per cell
		float originX = 0.0f;
		float originY = 0.0f;

		uint32 tileSize = 0;	//size in cells
		uint32 tileCountX = 0;
		uint32 tileCountY = 0;
	};
#pragma pack(pop)

	static_assert(sizeof(SMapHeader) == 40,
		"The size of SMapHeader has been changed. Keep the in-game reader up to date.");
}