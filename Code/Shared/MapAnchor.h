#pragma once
#include "CryMath/Cry_Math.h"

namespace JDKLevelMaps::Streaming
{
	using TStaticAnchorId = uint32;
	constexpr TStaticAnchorId kInvalidStaticAnchor = -1;

	class IMapAnchor
	{
	public:
		virtual ~IMapAnchor() = default;
		virtual const Vec3& GetAnchorPosition() const = 0;
	};
}