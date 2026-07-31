#pragma once
#include <vector>
#include <optional>

#include "Shared/MapHeader.h"

namespace JDKLevelMaps::Maps
{
	constexpr uint32 kEmptyTileOffset = 0xFFFFFFFF;

	class CMapFileReader final
	{
	public:
		CMapFileReader() = default;
		~CMapFileReader();

		bool Open(const char* filePath);
		void Close();

		const SMapHeader& GetHeader() const;
		SMapHeader TakeHeader();
		const std::vector<STileEntry>& GetDirectory() const;
		std::vector<STileEntry> TakeDirectory();
		size_t GetTotalPayloadSize() const;

		bool ReadTile(uint32 tileIndex, uint8* pDestBuffer);
		static bool ReadTileRaw(const char* filePath, const STileEntry& entry, uint8* pDestBuffer);

	private:
		bool ReadHeader();

		// Always returns false
		bool ThrowError(bool closeFile, const char* format, ...);

	private:
		FILE* m_pFile = nullptr;
		SMapHeader m_header;
		std::vector<STileEntry> m_directory;
		size_t m_totalPayloadSize = 0;
	};

	std::vector<string> GetFiles(const string& directory, const string& extensionFilter);
}