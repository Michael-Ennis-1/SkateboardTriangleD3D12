#pragma once
#include "Skateboard/Renderer/Pipeline.h"
#include "Platform/DirectX12/Memory/D3DMemoryAllocator.h"
#include "Platform/DirectX12/Model/D3DMeshletModel.h"

namespace Skateboard
{
	class D3DMeshletPipeline : public MeshletPipeline
	{
	public:
		explicit D3DMeshletPipeline(const std::wstring& debugName, const MeshletPipelineDesc& desc);
		DISABLE_COPY_AND_MOVE(D3DMeshletPipeline);

		void BindMeshPipeline(MeshletModel* model = nullptr) final override;
		void SetInputLayout(const BufferLayout& layout) override;
		void SetDefaultInputLayout() override;

		void SetResource(GPUResource* buffer, uint32_t shaderRegister, uint32_t registerSpace) override;
		void Unbind() final override;

		void Release() override;

		const MeshletPipelineDesc& GetDesc() const final override { return m_Desc; }

	private:
		void LoadMeshShader(const wchar_t* filename, void** blob) final override;

		void BuildRootSignature();

		void BuildMeshletPSO();

		void BindAllResources(ID3D12GraphicsCommandList6* pCommandList);

	private:
		Microsoft::WRL::ComPtr<IDxcBlob>	m_AmplificationShader,
											m_MeshShader		 ,
											m_PixelShader		 ;

		Microsoft::WRL::ComPtr<ID3D12RootSignature>	m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState>	m_PipelineStateObject;

		uint8_t* p_DataBegin;

		D3DDescriptorHandle m_DescriptorTableRootHandle{};

	};
}


