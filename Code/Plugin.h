// Copyright 2016-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <CrySystem/ICryPlugin.h>
#include <CryGame/IGameFramework.h>
#include <CryEntitySystem/IEntityClass.h>

#include "Shared/IJDKLevelMapsPlugin.h"
#include "Shared/ILevelMap.h"

class CJDKLevelMapsEngine : public JDKLevelMaps::IJDKLevelMapsPlugin, public ISystemEventListener
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

	virtual void Init(JDKLevelMaps::ELoadingMode mode, const string& directory) override;
	virtual void Shutdown() override;
	
	virtual void LoadAll() override;
	virtual void UnloadAll() override;

	virtual const JDKLevelMaps::Maps::ILevelMap* GetMap(JDKLevelMaps::EMapType type) const override;
};