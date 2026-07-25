#pragma once
#include "Bootstrap/Loaders/ILoadingStrategy.h"

namespace JDKLevelMaps::Maps::Database
{
	class CMapsDatabase;
}

namespace JDKLevelMaps::Bootstrap
{
	class CBootstrapManager final
	{
	public:
		void SetLoadingDirectory(const string& directory);
		void SetLoadingStrategy(std::unique_ptr<ILoadingStrategy> strategy);

		void LoadMaps(Maps::Database::CMapsDatabase& db);

	private:
		string m_loadingDirectory = "";
		std::unique_ptr<ILoadingStrategy> m_loadingStrategy = nullptr;
	};
}