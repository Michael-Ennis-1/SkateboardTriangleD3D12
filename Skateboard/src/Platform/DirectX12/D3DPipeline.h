#pragma once
#include "Skateboard/Renderer/Pipeline.h"
#include "D3D.h"
#include "Memory/D3DMemoryAllocator.h"
#include "ShaderTable.h"

namespace Skateboard
{
	static void LoadShader(const wchar_t* filename, void** blob);

	class D3DRasterizationPipeline : public RasterizationPipeline
	{
	public:
		D3DRasterizationPipeline(const std::wstring& debugName, const RasterizationPipelineDesc& desc);

		virtual void Bind(uint32_t geoID) final override;

		void Release() override;

	private:
		void LoadAllShaders();
		void BuildRootSignature();
		void BuildPSO();

	private:
		Microsoft::WRL::ComPtr<IDxcBlob>			m_VertexShader,
													m_HullShader,
													m_DomainShader,
													m_GeoShader,
													m_PixelShader,
													m_ComputeShader;
		Microsoft::WRL::ComPtr<ID3D12RootSignature>	m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState>	m_PipelineStateObject;
		D3DDescriptorHandle							m_DescriptorTableRootHandle;
		D3D_PRIMITIVE_TOPOLOGY						m_PrimitiveTopology;
	};

	class D3DComputePipeline : public ComputePipeline
	{
	public:
		D3DComputePipeline(const std::wstring& debugName, const ComputePipelineDesc& desc);

		virtual void Bind() final override;

		void Release() override;
	private:
		void LoadAllShaders();
		void BuildRootSignature();
		void BuildPSO();

	private:
		Microsoft::WRL::ComPtr<IDxcBlob>			m_ComputeShader;
		Microsoft::WRL::ComPtr<ID3D12RootSignature>	m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState>	m_PipelineStateObject;
		D3DDescriptorHandle							m_DescriptorTableRootHandle;
	};

	class D3DRaytracingPipeline : public RaytracingPipeline
	{
	public:
		D3DRaytracingPipeline(const std::wstring& debugName, const RaytracingPipelineDesc& desc);

		virtual void Bind() final override;
		virtual void ResizeDispatchAndOutputUAV(uint32_t newWidth, uint32_t newHeight, uint32_t newDepth = 1u) final override;

		void Release() override;
	private:
		void BuildRootSignature();
		void BuildSO();
		void BuildShaderTable();

	private:
		Microsoft::WRL::ComPtr<IDxcBlob>			m_ShaderDXIL;
		Microsoft::WRL::ComPtr<ID3D12RootSignature>	m_GlobalRootSignature,
													m_LocalRaygenRootSignature,
													m_LocalHitGroupRootSignature,
													m_LocalMissRootSignature,
													m_LocalCallableRootSignature;
		Microsoft::WRL::ComPtr<ID3D12StateObject>	m_StateObject;
		D3DDescriptorHandle							m_GlobalDescriptorTableHandle,
													m_LocalRaygenDescriptorTableHandle,
													m_LocalHitGroupDescriptorTableHandle,
													m_LocalMissDescriptorTableHandle,
													m_LocalCallableDescriptorTableHandle;
		ShaderTable									m_ShaderTable;
		std::vector<class D3DUploadBuffer*>			v_GlobalConstantBuffers,
													v_LocalRayGenConstantBuffers,
													v_LocalHitGroupConstantBuffers,
													v_LocalMissConstantBuffers,
													v_LocalCallableConstantBuffers;
		std::vector<class D3DDescriptorTable*>		v_GlobalDescriptorTables,
													v_LocalRayGenDescriptorTables,
													v_LocalHitGroupDescriptorTables,
													v_LocalMissDescriptorTables,
													v_LocalCallableDescriptorTables;
	};
}
