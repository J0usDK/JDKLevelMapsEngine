#include "StdAfx.h"
#include "Plugin.h"

#include <CrySchematyc/Env/IEnvRegistry.h>
#include <CrySchematyc/Env/EnvPackage.h>
#include <CrySchematyc/Utils/SharedString.h>

#include "Bootstrap/BootstrapManager.h"
#include "Bootstrap/Loaders/FullLoad.h"
#include "Bootstrap/Loaders/SpatialLoad.h"
#include "Runtime/Database/MapsDatabase.h"

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

CJDKLevelMapsEngine::~CJDKLevelMapsEngine()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
	if (gEnv->pGameFramework)
		gEnv->pGameFramework->UnregisterListener(this);

	if (gEnv->pSchematyc)
		gEnv->pSchematyc->GetEnvRegistry().DeregisterPackage(CJDKLevelMapsEngine::GetCID());
}

bool CJDKLevelMapsEngine::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this,"CJDKLevelMapsEngine");
	return true;
}

void CJDKLevelMapsEngine::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
		case ESYSTEM_EVENT_GAME_FRAMEWORK_INIT_DONE:
		{
			if (gEnv->pGameFramework)
				gEnv->pGameFramework->RegisterListener(this, "CJDKLevelMapsEngine", FRAMEWORKLISTENERPRIORITY_DEFAULT);
		}
		break;

		case ESYSTEM_EVENT_REGISTER_SCHEMATYC_ENV:
		{
			auto staticAutoRegisterLambda = [](Schematyc::IEnvRegistrar& registrar)
			{
				Detail::CStaticAutoRegistrar<Schematyc::IEnvRegistrar&>::InvokeStaticCallbacks(registrar);
			};

			if (gEnv->pSchematyc)
			{
				gEnv->pSchematyc->GetEnvRegistry().RegisterPackage(
					stl::make_unique<Schematyc::CEnvPackage>(
						CJDKLevelMapsEngine::GetCID(),
						"EntityComponents",
						"Crytek GmbH",
						"Components",
						staticAutoRegisterLambda
						)
				);
			}
		}
		break;
	}
}

struct SSystemState
{
	std::unique_ptr<JDKLevelMaps::Bootstrap::CBootstrapManager> pBootstrap;
	std::unique_ptr<JDKLevelMaps::Maps::Database::CMapsDatabase> pDatabase;

	bool bInitialized = false;
};
std::unique_ptr<SSystemState> g_state = nullptr;

void CJDKLevelMapsEngine::OnPostUpdate(float fDeltaTime)
{
	if (!g_state || !g_state->bInitialized) return;
	g_state->pBootstrap->PreUpdate();
}

void CJDKLevelMapsEngine::OnActionEvent(const SActionEvent& event)
{
	switch (event.m_event)
	{
		case EActionEvent::eAE_earlyPreUpdate:
			if (!g_state || !g_state->bInitialized) return;
			g_state->pBootstrap->PostUpdate(*g_state->pDatabase);
			break;
		default:
			break;
	}
}

void CJDKLevelMapsEngine::Init(JDKLevelMaps::ELoadingMode mode, const string& directory)
{
	g_state = std::make_unique<SSystemState>();
	g_state->pBootstrap = std::make_unique<JDKLevelMaps::Bootstrap::CBootstrapManager>();
	g_state->pDatabase = std::make_unique<JDKLevelMaps::Maps::Database::CMapsDatabase>();

	std::unique_ptr<JDKLevelMaps::Bootstrap::ILoadingStrategy> pLoadingStrategy;
	switch (mode)
	{
	case JDKLevelMaps::ELoadingMode::FullLoad:
		pLoadingStrategy = std::make_unique<JDKLevelMaps::Bootstrap::CFullLoad>();
		break;
	case JDKLevelMaps::ELoadingMode::SpatialStream:
		pLoadingStrategy = std::make_unique<JDKLevelMaps::Bootstrap::CSpatialLoad>();
		break;
	}

	g_state->pBootstrap->SetLoadingDirectory(directory);
	g_state->pBootstrap->SetLoadingStrategy(std::move(pLoadingStrategy));
}

void CJDKLevelMapsEngine::RegisterDynamicAnchor(const JDKLevelMaps::Streaming::IMapAnchor* pAnchor, uint16 radius)
{
	if (!g_state || !g_state->pBootstrap || !g_state->pDatabase) return;
	g_state->pBootstrap->RegisterDynamicAnchor(pAnchor, radius);
}

void CJDKLevelMapsEngine::UnregisterDynamicAnchor(const JDKLevelMaps::Streaming::IMapAnchor* pAnchor)
{
	if (!g_state || !g_state->pBootstrap || !g_state->pDatabase) return;
	g_state->pBootstrap->UnregisterDynamicAnchor(pAnchor);
}

JDKLevelMaps::Streaming::TStaticAnchorID CJDKLevelMapsEngine::RegisterPointAnchor(Vec3 anchorPos, uint16 radius)
{
	if (!g_state || !g_state->pBootstrap || !g_state->pDatabase) return -1;
	return g_state->pBootstrap->RegisterPointAnchor(anchorPos, radius);
}

void CJDKLevelMapsEngine::UnregisterPointAnchor(JDKLevelMaps::Streaming::TStaticAnchorID id)
{
	if (!g_state || !g_state->pBootstrap || !g_state->pDatabase) return;
	g_state->pBootstrap->UnregisterPointAnchor(id);
}

void CJDKLevelMapsEngine::UpdatePointAnchor(JDKLevelMaps::Streaming::TStaticAnchorID id, Vec3 pos)
{
	if (!g_state || !g_state->pBootstrap || !g_state->pDatabase) return;
	g_state->pBootstrap->UpdatePointAnchor(id, pos);
}

void CJDKLevelMapsEngine::FinishInit()
{
	if (!g_state || !g_state->pBootstrap || !g_state->pDatabase) return;
	g_state->pBootstrap->LoadMaps(*g_state->pDatabase);
	g_state->bInitialized = true;
}

void CJDKLevelMapsEngine::Shutdown()
{
	if (!g_state || !g_state->bInitialized) return;

	UnloadAll();
	g_state.reset();
}

void CJDKLevelMapsEngine::UnloadAll()
{
	if (!g_state || !g_state->bInitialized) return;
	g_state->pDatabase->UnregisterAll();
}

const JDKLevelMaps::Maps::ILevelMap* CJDKLevelMapsEngine::GetMap(JDKLevelMaps::EMapType type) const
{
	if (!g_state || !g_state->bInitialized) return nullptr;
	return g_state->pDatabase->GetMap(type);
}

CRYREGISTER_SINGLETON_CLASS(CJDKLevelMapsEngine)