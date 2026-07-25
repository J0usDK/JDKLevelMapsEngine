#include "StdAfx.h"
#include "Plugin.h"

#include <CrySchematyc/Env/IEnvRegistry.h>
#include <CrySchematyc/Env/EnvPackage.h>
#include <CrySchematyc/Utils/SharedString.h>

#include "Bootstrap/BootstrapManager.h"
#include "Bootstrap/Loaders/FullLoad.h"
#include "Runtime/Database/MapsDatabase.h"

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

CJDKLevelMapsEngine::~CJDKLevelMapsEngine()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
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
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "[JDKLevelMaps] SpatialStream mode not implemented yet");
		return;
	}

	g_state->pBootstrap->SetLoadingDirectory(directory);
	g_state->pBootstrap->SetLoadingStrategy(std::move(pLoadingStrategy));

	g_state->bInitialized = true;
}

void CJDKLevelMapsEngine::Shutdown()
{
	if (!g_state->bInitialized) return;

	UnloadAll();
	g_state.reset();
}

void CJDKLevelMapsEngine::LoadAll()
{
	if (!g_state || !g_state->bInitialized) return;
	g_state->pBootstrap->LoadMaps(*g_state->pDatabase);
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