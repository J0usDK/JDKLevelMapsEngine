#pragma once
#include "CryCore/BaseTypes.h"

namespace JDKLevelMaps
{
	enum class EMapType : uint16;
}

namespace JDKLevelMaps::Maps
{
	class ILevelMap
	{
	public:
		virtual ~ILevelMap() = default;

		virtual EMapType GetType() const = 0;
		virtual bool IsValid() const = 0;

		virtual size_t GetMemoryUsage() const = 0;
	};
}