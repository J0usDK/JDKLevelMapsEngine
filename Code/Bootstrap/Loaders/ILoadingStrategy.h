#pragma once
#include <CryString/CryString.h>

namespace JDKLevelMaps::Maps::Database
{
	class CMapsDatabase;
}

namespace JDKLevelMaps::Bootstrap
{
	class ILoadingStrategy
	{
	public:
		virtual ~ILoadingStrategy() = default;

		virtual void Initialize(Maps::Database::CMapsDatabase& db, const string& directory) = 0;
		virtual void UnloadAll(Maps::Database::CMapsDatabase& db) = 0;
	};
}