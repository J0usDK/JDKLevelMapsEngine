#include "StdAfx.h"
#include "BootstrapManager.h"

#include "Runtime/Database/MapsDatabase.h"

void JDKLevelMaps::Bootstrap::CBootstrapManager::SetLoadingDirectory(const string& directory)
{
	m_loadingDirectory = !directory.empty() ? directory : string(gEnv->p3DEngine->GetLevelFilePath("JDKLevelMaps"));

	if (m_loadingDirectory.back() != '/')
		m_loadingDirectory += '/';
}

void JDKLevelMaps::Bootstrap::CBootstrapManager::SetLoadingStrategy(std::unique_ptr<ILoadingStrategy> strategy)
{
	m_loadingStrategy = std::move(strategy);
}

void JDKLevelMaps::Bootstrap::CBootstrapManager::LoadMaps(Maps::Database::CMapsDatabase& db)
{
	if (!m_loadingStrategy)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "[JDKLevelMaps] No loading strategy set");
		return;
	}

	m_loadingStrategy->Initialize(db, m_loadingDirectory);
}