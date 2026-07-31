#include "StdAfx.h"
#include "MapReader.h"

#include <limits>

#include "Shared/MapHeader.h"


namespace JDKLevelMaps::Maps
{
	CMapFileReader::~CMapFileReader()
	{
		Close();
	}

	bool CMapFileReader::Open(const char* filePath)
	{
		m_pFile = gEnv->pCryPak->FOpen(filePath, "rb");
		if (!m_pFile)
			return ThrowError(false, "[JDKLevelMaps] File %s is not found", filePath);

		if (!ReadHeader())
			return false;

		return true;
	}

	void CMapFileReader::Close()
	{
		m_directory.clear();
		if (!m_pFile)
			return;

		gEnv->pCryPak->FClose(m_pFile);
		m_totalPayloadSize = 0;
		m_pFile = nullptr;
	}

	bool CMapFileReader::ReadHeader()
	{
		if (gEnv->pCryPak->FReadRaw(&m_header, sizeof(SMapHeader), 1, m_pFile) != 1 || m_header.magic != kLayerMapMagic || m_header.version != kLayerMapVersion)
			return ThrowError(true, "[JDKLevelMaps] Map file header is corrupted");

		const size_t directorySize = static_cast<size_t>(m_header.tileCountX) * m_header.tileCountY;
		m_directory.resize(directorySize);

		if (gEnv->pCryPak->FReadRaw(m_directory.data(), sizeof(STileEntry), directorySize, m_pFile) != directorySize)
			return ThrowError(true, "[JDKLevelMaps] Map file directory is corrupted");

		for (const auto& entry : m_directory)
			if (entry.flags != 1 && entry.byteSize > 0)
				m_totalPayloadSize += entry.byteSize;

		return true;
	}

	bool CMapFileReader::ReadTile(uint32 tileIndex, uint8* pDestBuffer)
	{
		if (!m_pFile || tileIndex >= m_directory.size() || !pDestBuffer)
			return ThrowError(false, "[JDKLevelMaps] File is not opened or incorrect tile index");

		const STileEntry& entry = m_directory[tileIndex];
		if (entry.flags == 1 || entry.byteSize == 0 || entry.fileOffset == 0)
			return true;

		CRY_ASSERT_MESSAGE(entry.fileOffset <= static_cast<uint64>(std::numeric_limits<long>::max()),
			"[JDKLevelMaps] Tile offset exceeds 2GB limit! Map is too large for 32-bit FSeek.");

		gEnv->pCryPak->FSeek(m_pFile, static_cast<long>(entry.fileOffset), SEEK_SET);
		if (gEnv->pCryPak->FReadRaw(pDestBuffer, 1, entry.byteSize, m_pFile) != entry.byteSize)
			return ThrowError(false, "[JDKLevelMaps] Disk I/O error while reading tile %u", tileIndex);

		return true;
	}

	bool CMapFileReader::ReadTileRaw(const char* filePath, const STileEntry& entry, uint8* pDestBuffer)
	{
		if (entry.flags == 1 || entry.byteSize == 0 || entry.fileOffset == 0)
			return true;

		FILE* pFile = gEnv->pCryPak->FOpen(filePath, "rb");
		if (!pFile)
			return false;

		CRY_ASSERT_MESSAGE(entry.fileOffset <= static_cast<uint64>(std::numeric_limits<long>::max()),
			"[JDKLevelMaps] Tile offset exceeds 2GB limit! Map is too large for 32-bit FSeek.");

		gEnv->pCryPak->FSeek(pFile, static_cast<long>(entry.fileOffset), SEEK_SET);
		const bool ok = gEnv->pCryPak->FReadRaw(pDestBuffer, 1, entry.byteSize, pFile) == entry.byteSize;

		gEnv->pCryPak->FClose(pFile);
		return ok;
	}

	bool CMapFileReader::ThrowError(bool closeFile, const char* format, ...)
	{
		if (closeFile)
			Close();

		char buffer[512];
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);

		CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, "%s", buffer);

		return false;
	}

	const SMapHeader& CMapFileReader::GetHeader() const { return m_header; }
	SMapHeader CMapFileReader::TakeHeader() { return std::move(m_header); }
	const std::vector<STileEntry>& CMapFileReader::GetDirectory() const { return m_directory; }
	std::vector<STileEntry> CMapFileReader::TakeDirectory() { return std::move(m_directory); }
	size_t CMapFileReader::GetTotalPayloadSize() const { return m_totalPayloadSize; }


	std::vector<string> GetFiles(const string& directory, const string& extensionFilter)
	{
		std::vector<string> filePaths;
		CryFixedStringT<MAX_PATH> searchPath(directory.c_str());
		searchPath += extensionFilter;

		_finddata_t fd;
		intptr_t handle = gEnv->pCryPak->FindFirst(searchPath.c_str(), &fd);

		if (handle == -1)
			return filePaths;

		CryFixedStringT<MAX_PATH> pathBuffer;
		do
		{
			if (!(fd.attrib & _A_SUBDIR))
			{
				pathBuffer.assign(directory.c_str());
				pathBuffer.append(fd.name);
				filePaths.emplace_back(pathBuffer);
			}
		} while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

		gEnv->pCryPak->FindClose(handle);
		return filePaths;
	}
}