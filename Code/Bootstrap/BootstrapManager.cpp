#include "StdAfx.h"
#include "BootstrapManager.h"

#include "Runtime/Database/MapsDatabase.h"

namespace JDKLevelMaps::Bootstrap
{
	void CBootstrapManager::SetLoadingDirectory(const string& directory)
	{
		m_loadingDirectory = !directory.empty() ? directory : string(gEnv->p3DEngine->GetLevelFilePath("JDKLevelMaps"));

		if (m_loadingDirectory.back() != '/')
			m_loadingDirectory += '/';
	}

	void CBootstrapManager::SetLoadingStrategy(std::unique_ptr<ILoadingStrategy> strategy)
	{
		m_loadingStrategy = std::move(strategy);
	}

	void CBootstrapManager::RegisterDynamicAnchor(const Streaming::IMapAnchor* pAnchor, uint16 radius)
	{
		m_loadingStrategy->RegisterDynamicAnchor(pAnchor, radius);
	}

	void CBootstrapManager::UnregisterDynamicAnchor(const Streaming::IMapAnchor* pAnchor)
	{
		m_loadingStrategy->UnregisterDynamicAnchor(pAnchor);
	}

	Streaming::TStaticAnchorID CBootstrapManager::RegisterPointAnchor(Vec3 anchorPos, uint16 radius)
	{
		return m_loadingStrategy->RegisterPointAnchor(anchorPos, radius);
	}

	void CBootstrapManager::UnregisterPointAnchor(Streaming::TStaticAnchorID id)
	{
		m_loadingStrategy->UnregisterPointAnchor(id);
	}

	void CBootstrapManager::UpdatePointAnchor(Streaming::TStaticAnchorID id, Vec3 pos)
	{
		m_loadingStrategy->UpdatePointAnchor(id, pos);
	}

	void CBootstrapManager::LoadMaps(Maps::Database::CMapsDatabase& db)
	{
		if (!m_loadingStrategy)
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "[JDKLevelMaps] No loading strategy set");
			return;
		}

		m_loadingStrategy->Initialize(db, m_loadingDirectory);
	}

	void CBootstrapManager::PreUpdate()
	{
		m_loadingStrategy->PreUpdate();
	}

	void CBootstrapManager::PostUpdate(Maps::Database::CMapsDatabase& db)
	{
		m_loadingStrategy->PostUpdate(db);
	}
}