#pragma once
#include "CryCore/BaseTypes.h"

namespace JDKLevelMaps::Streaming
{
	using TStaticAnchorID = uint32;

	class IMapAnchor
	{
	public:
		virtual ~IMapAnchor() = default;

		virtual void GetPosition(float& outX, float& outY) const = 0;
	};
}