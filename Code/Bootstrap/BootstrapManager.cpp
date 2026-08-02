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

	void CBootstrapManager::RegisterDynamicAnchor(EMapType targetMap, const Streaming::IMapAnchor* pAnchor, uint16 radius)
	{
		m_loadingStrategy->RegisterDynamicAnchor(targetMap, pAnchor, radius);
	}

	void CBootstrapManager::UnregisterDynamicAnchor(EMapType targetMap,  const Streaming::IMapAnchor* pAnchor)
	{
		m_loadingStrategy->UnregisterDynamicAnchor(targetMap, pAnchor);
	}

	Streaming::TStaticAnchorID CBootstrapManager::RegisterPointAnchor(EMapType targetMap, Vec3 anchorPos, uint16 radius)
	{
		return m_loadingStrategy->RegisterPointAnchor(targetMap, anchorPos, radius);
	}

	void CBootstrapManager::UnregisterPointAnchor(EMapType targetMap, Streaming::TStaticAnchorID id)
	{
		m_loadingStrategy->UnregisterPointAnchor(targetMap, id);
	}

	void CBootstrapManager::UpdatePointAnchor(EMapType targetMap, Streaming::TStaticAnchorID id, Vec3 pos)
	{
		m_loadingStrategy->UpdatePointAnchor(targetMap, id, pos);
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