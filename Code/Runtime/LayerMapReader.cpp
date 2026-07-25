#include "StdAfx.h"
#include "LayerMapReader.h"

std::optional<JDKLevelMaps::SLoadedLayerMap> JDKLevelMaps::LoadLayerMap(const char* filePath)
{
	FILE* const pFile = gEnv->pCryPak->FOpen(filePath, "rb");
	if (!pFile)
		return std::nullopt;

	SLayerMapHeader header;
	const bool headerOk = gEnv->pCryPak->FReadRaw(&header, sizeof(header), 1, pFile) == 1;

	if (!headerOk || header.magic != kLayerMapMagic || header.version != kLayerMapVersion || header.gridWidth <= 0 || header.gridHeight <= 0)
	{
		gEnv->pCryPak->FClose(pFile);
		CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, "[JDKLevelMaps] Map file is corrupted (header)");
		return std::nullopt;
	}

	const size_t fileSize = gEnv->pCryPak->FGetSize(pFile);
	if (fileSize <= sizeof(header))
	{
		gEnv->pCryPak->FClose(pFile);
		CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, "[JDKLevelMaps] Map file is corrupted: map's data is missed");
		return std::nullopt;
	}

	std::vector<uint8> data(fileSize - sizeof(header));
	const bool dataOk = gEnv->pCryPak->FReadRaw(data.data(), 1, data.size(), pFile) == data.size();
	gEnv->pCryPak->FClose(pFile);

	if (!dataOk)
	{
		CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, "[JDKLevelMaps] Map file is corrupted: map's data is corrupted");
		return std::nullopt;
	}

	SLoadedLayerMap result;
	result.header = header;
	result.data = std::move(data);
	return result;
}