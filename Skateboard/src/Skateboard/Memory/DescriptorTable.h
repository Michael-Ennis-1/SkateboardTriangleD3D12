#pragma once
#include "Skateboard/Renderer/InternalFormats.h"

namespace Skateboard
{
	class DescriptorTable
	{
	public:
		virtual ~DescriptorTable() {}

		virtual uint32_t GetDescriptorCount() const = 0;

		void SetTableType(ShaderDescriptorTableType_ type) { m_Type = type; }
		virtual ShaderDescriptorTableType_ GetTableType() const { return m_Type; }

	protected:
		ShaderDescriptorTableType_ m_Type;
	};
}