// Copyright 2016-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <CrySystem/ICryPlugin.h>
#include <CryGame/IGameFramework.h>
#include <CryEntitySystem/IEntityClass.h>

#include "Shared/IJDKLevelMapsPlugin.h"
#include "Shared/ILevelMap.h"

namespace JDKLevelMaps::Streaming
{
	class IMapAnchor;
	using TStaticAnchorID = uint32;
}

class CJDKLevelMapsEngine : public JDKLevelMaps::IJDKLevelMapsPlugin, public ISystemEventListener, public IGameFrameworkListener
{
public:
	CRYGENERATE_SINGLETONCLASS_GUID(CJDKLevelMapsEngine, "JDKLevelMapsEngine", "2711a23d-3848-4cdd-a95b-e9d88ffa23b0"_cry_guid)

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(Cry::IEnginePlugin)
		CRYINTERFACE_ADD(JDKLevelMaps::IJDKLevelMapsPlugin)
	CRYINTERFACE_END()

	virtual ~CJDKLevelMapsEngine();
	
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

	virtual void OnPostUpdate(float fDeltaTime) override;
	virtual void OnPreRender() override {}
	virtual void OnSaveGame(ISaveGame* pSaveGame) override {}
	virtual void OnLoadGame(ILoadGame* pLoadGame) override {}
	virtual void OnLevelEnd(const char* nextLevel) override {}
	virtual void OnActionEvent(const SActionEvent& event) override;

	virtual void Init(JDKLevelMaps::ELoadingMode mode, const string& directory) override;
	virtual void FinishInit();
	virtual void Shutdown() override;

	virtual void RegisterDynamicAnchor(JDKLevelMaps::EMapType targetMap, const JDKLevelMaps::Streaming::IMapAnchor* pAnchor, uint16 radius) override;
	virtual void UnregisterDynamicAnchor(JDKLevelMaps::EMapType targetMap, const JDKLevelMaps::Streaming::IMapAnchor* pAnchor) override;

	virtual JDKLevelMaps::Streaming::TStaticAnchorID RegisterPointAnchor(JDKLevelMaps::EMapType targetMap, Vec3 anchorPos, uint16 radius) override;
	virtual void UnregisterPointAnchor(JDKLevelMaps::EMapType targetMap, JDKLevelMaps::Streaming::TStaticAnchorID id) override;
	virtual void UpdatePointAnchor(JDKLevelMaps::EMapType targetMap, JDKLevelMaps::Streaming::TStaticAnchorID id, Vec3 pos) override;
	
	virtual void UnloadAll() override;

	virtual const JDKLevelMaps::Maps::ILevelMap* GetMap(JDKLevelMaps::EMapType type) const override;
};