#pragma once
#include <array>
#include <vector>
#include <stdint.h>
#include "D3D.h"
#include "Skateboard/Core.h"
#include "Skateboard/SizedPtr.h"

struct ShaderRecord
{
	ShaderRecord() = delete;
	ShaderRecord(SizedPtr pShaderIdentifier) : ShaderIdentifier(pShaderIdentifier), LocalRootArguments({ nullptr, 0 }) {}
	ShaderRecord(SizedPtr pShaderIdentifier, SizedPtr pLocalRootArguments) : ShaderIdentifier(pShaderIdentifier), LocalRootArguments(pLocalRootArguments) {}

	SizedPtr ShaderIdentifier;
	SizedPtr LocalRootArguments;
};

struct ShaderTable
{
	ShaderTable() :
		a_RecordAndShaderTableSizes{ std::pair<uint32_t, uint32_t>(0u, 0u) },
		a_ShaderTableStartAddresses{ 0u }
	{
	}

	void UpdateShaderRecord(uint8_t recordIndex, uint8_t argumentIndex, UINT64 argument)
	{
		// Cull the record on the GPU memory
		uint8_t* data;
		m_ShaderTableStorage->Map(0, nullptr, reinterpret_cast<void**>(&data));
		data += v_RecordBufferStartInBytes[recordIndex] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + argumentIndex * sizeof(UINT64);
		memcpy(data, &argument, sizeof(UINT64));
		m_ShaderTableStorage->Unmap(0, nullptr);
	}

	void GenerateShaderTableBuffer(ID3D12Device5* pDevice)
	{
		// Minimun requirements
		SKTBD_CORE_ASSERT(!m_ShaderTableStorage.Get(), "The Shader table buffer was already created. Preventing double creation..");
		SKTBD_CORE_ASSERT(v_RaygenShaderRecord.size() == 1u, "No ray generation shader found. Have you added a Ray Generation shader record?");

		// Calculate the maximum width
		a_RecordAndShaderTableSizes[0].second = ROUND_UP(a_RecordAndShaderTableSizes[0].first, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		a_RecordAndShaderTableSizes[1].second = ROUND_UP(a_RecordAndShaderTableSizes[1].first * static_cast<uint32_t>(v_MissShaderRecords.size()), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		a_RecordAndShaderTableSizes[2].second = ROUND_UP(a_RecordAndShaderTableSizes[2].first * static_cast<uint32_t>(v_HitGroupShaderRecords.size()), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		a_RecordAndShaderTableSizes[3].second = ROUND_UP(a_RecordAndShaderTableSizes[3].first * static_cast<uint32_t>(v_CallableShaderRecords.size()), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);


		//Determine the buffer size
		const uint32_t buffSize = a_RecordAndShaderTableSizes[0].second + a_RecordAndShaderTableSizes[1].second + a_RecordAndShaderTableSizes[2].second + a_RecordAndShaderTableSizes[3].second;

		// Create the bufer
		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Alignment = 0;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.Width = buffSize;
		bufferDesc.Height = 1;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.MipLevels = 1;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;

		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 0;
		heapProperties.VisibleNodeMask = 0;

		D3D_CHECK_FAILURE(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_ShaderTableStorage.GetAddressOf())));

		// Copy the shader tables into the buffer
		uint8_t* data;
		m_ShaderTableStorage->Map(0, nullptr, reinterpret_cast<void**>(&data));

		CopyShaderTableTo(data, v_RaygenShaderRecord, a_RecordAndShaderTableSizes[0].first);
		CopyShaderTableTo(data, v_MissShaderRecords, a_RecordAndShaderTableSizes[1].first);
		CopyShaderTableTo(data, v_HitGroupShaderRecords, a_RecordAndShaderTableSizes[2].first);
		CopyShaderTableTo(data, v_CallableShaderRecords, a_RecordAndShaderTableSizes[3].first);

		m_ShaderTableStorage->Unmap(0, nullptr);

		// Assign the start addresses
		// Note: if no record was supplied for a given table, then we need to give an NULL address to the dispatch
		a_ShaderTableStartAddresses = {
			m_ShaderTableStorage->GetGPUVirtualAddress(),
			v_MissShaderRecords.empty() ? 0u : m_ShaderTableStorage->GetGPUVirtualAddress() + a_RecordAndShaderTableSizes[0].second,
			v_HitGroupShaderRecords.empty() ? 0u : m_ShaderTableStorage->GetGPUVirtualAddress() + a_RecordAndShaderTableSizes[0].second + a_RecordAndShaderTableSizes[1].second,
			v_CallableShaderRecords.empty() ? 0u : m_ShaderTableStorage->GetGPUVirtualAddress() + a_RecordAndShaderTableSizes[0].second + a_RecordAndShaderTableSizes[1].second + a_RecordAndShaderTableSizes[2].second
		};

		// The pointers contained in this vector are going out of scope after this operation
		v_RaygenShaderRecord.clear();
		v_MissShaderRecords.clear();
		v_HitGroupShaderRecords.clear();
		v_CallableShaderRecords.clear();
	}

	void SetRayGenShader(const ShaderRecord& raygen)
	{
		SKTBD_CORE_ASSERT(v_RaygenShaderRecord.size() == 0u && !m_ShaderTableStorage.Get(), "No previous record must exist, there can only be one raygen shader");
		SKTBD_CORE_ASSERT(raygen.ShaderIdentifier.Ptr != nullptr, "The input shader record does not contain a shader identifier!");
		SKTBD_CORE_ASSERT(raygen.ShaderIdentifier.Size + raygen.LocalRootArguments.Size < D3D12_RAYTRACING_MAX_SHADER_RECORD_STRIDE,
			"The input shader record is too large to fit in the table. Consider reducing your local root arguments.");

		// Determine size and store
		a_RecordAndShaderTableSizes[0].first = raygen.ShaderIdentifier.Size + raygen.LocalRootArguments.Size;
		a_RecordAndShaderTableSizes[0].first = ROUND_UP(a_RecordAndShaderTableSizes[0].first, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

		v_RaygenShaderRecord.push_back(raygen);
		v_RecordBufferStartInBytes.push_back(0u);	// The raygen record will always be the first record in the final buffer
	}
	void AddMissShader(const ShaderRecord& miss)
	{
		SKTBD_CORE_ASSERT(!m_ShaderTableStorage.Get(), "The shader table buffer is already created. Have you called GenerateShaderTableBuffer() before adding records?");
		SKTBD_CORE_ASSERT(miss.ShaderIdentifier.Ptr != nullptr, "The input shader record does not contain a shader identifier!");
		SKTBD_CORE_ASSERT(miss.ShaderIdentifier.Size + miss.LocalRootArguments.Size < D3D12_RAYTRACING_MAX_SHADER_RECORD_STRIDE,
			"The input shader record is too large to fit in the table. Consider reducing your local root arguments.");

		// Determine size and store
		uint32_t recordSize = miss.ShaderIdentifier.Size + miss.LocalRootArguments.Size;
		if (a_RecordAndShaderTableSizes[1].first < recordSize)
			a_RecordAndShaderTableSizes[1].first = ROUND_UP(recordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);	// Align correctly though

		v_MissShaderRecords.push_back(miss);
	}
	void AddHitGroupShaders(const ShaderRecord& hitgrp)
	{
		SKTBD_CORE_ASSERT(!m_ShaderTableStorage.Get(), "The shader table buffer is already created. Have you called GenerateShaderTableBuffer() before adding records?");
		SKTBD_CORE_ASSERT(hitgrp.ShaderIdentifier.Ptr != nullptr, "The input shader record does not contain a shader identifier!");
		SKTBD_CORE_ASSERT(hitgrp.ShaderIdentifier.Size + hitgrp.LocalRootArguments.Size < D3D12_RAYTRACING_MAX_SHADER_RECORD_STRIDE,
			"The input shader record is too large to fit in the table. Consider reducing your local root arguments.");

		// Determine size and store
		uint32_t recordSize = hitgrp.ShaderIdentifier.Size + hitgrp.LocalRootArguments.Size;
		if (a_RecordAndShaderTableSizes[2].first < recordSize)
			a_RecordAndShaderTableSizes[2].first = ROUND_UP(recordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

		v_HitGroupShaderRecords.push_back(hitgrp);
	}
	void AddCallableShader(const ShaderRecord& callable)
	{
		SKTBD_CORE_ASSERT(!m_ShaderTableStorage.Get(), "The shader table buffer is already created. Have you called GenerateShaderTableBuffer() before adding records?");
		SKTBD_CORE_ASSERT(callable.ShaderIdentifier.Ptr != nullptr, "The input shader record does not contain a shader identifier!");
		SKTBD_CORE_ASSERT(callable.ShaderIdentifier.Size + callable.LocalRootArguments.Size < D3D12_RAYTRACING_MAX_SHADER_RECORD_STRIDE,
			"The input shader record is too large to fit in the table. Consider reducing your local root arguments.");

		// Determine size and store
		uint32_t recordSize = callable.ShaderIdentifier.Size + callable.LocalRootArguments.Size;
		if (a_RecordAndShaderTableSizes[3].first < recordSize)
			a_RecordAndShaderTableSizes[3].first = ROUND_UP(recordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

		v_CallableShaderRecords.push_back(callable);
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetRaygenRecordStartAddress() const { return a_ShaderTableStartAddresses[0]; }
	D3D12_GPU_VIRTUAL_ADDRESS GetMissRecordStartAddress() const { return a_ShaderTableStartAddresses[1]; }
	D3D12_GPU_VIRTUAL_ADDRESS GetHitgroupRecordStartAddress() const { return a_ShaderTableStartAddresses[2]; }
	D3D12_GPU_VIRTUAL_ADDRESS GetCallableRecordStartAddress() const { return a_ShaderTableStartAddresses[3]; }

	uint32_t GetRaygenShaderTableSize() const { return a_RecordAndShaderTableSizes[0].second; }
	uint32_t GetMissShaderTableStride() const { return a_RecordAndShaderTableSizes[1].first; }
	uint32_t GetMissShaderTableSize() const { return a_RecordAndShaderTableSizes[1].second; }
	uint32_t GetHitGroupShaderTableStride() const { return a_RecordAndShaderTableSizes[2].first; }
	uint32_t GetHitGroupShaderTableSize() const { return a_RecordAndShaderTableSizes[2].second; }
	uint32_t GetCallableShaderTableStride() const { return a_RecordAndShaderTableSizes[3].first; }
	uint32_t GetCallableShaderTableSize() const { return a_RecordAndShaderTableSizes[3].second; }

private:
	inline void CopyShaderTableTo(uint8_t*& dest, const std::vector<ShaderRecord>& records, uint32_t maxRecordSize)
	{
		uint32_t tableSize = static_cast<uint32_t>(records.size());
		for (uint32_t i = 0u; i < tableSize; ++i)
		{
			const ShaderRecord& item = records[i];
			uint8_t* originalPtr = dest;

			// Copy the shader identifier
			memcpy(dest, item.ShaderIdentifier.Ptr, item.ShaderIdentifier.Size);

			// Copy the root arguments
			if (item.LocalRootArguments.Ptr)
				memcpy(dest + item.ShaderIdentifier.Size, item.LocalRootArguments.Ptr, item.LocalRootArguments.Size);

			// Progress to the next record
			dest += maxRecordSize;

			// After the last record the table must be aligned for the next record
			if (i == tableSize - 1u)
				dest += ROUND_UP(maxRecordSize * tableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) - maxRecordSize * tableSize;

			// Determine how many bytes have progressed for this record
			uint32_t previousBytes = v_RecordBufferStartInBytes.back();
			v_RecordBufferStartInBytes.emplace_back(static_cast<uint32_t>(dest - originalPtr) + previousBytes);
		}
	}

private:
	std::vector<ShaderRecord>						v_RaygenShaderRecord,	// There is only one raygen record, but we'll keep it in a vector for the simplicity of CopyShaderTableTo() (yes, lazy)
													v_MissShaderRecords,
													v_HitGroupShaderRecords,
													v_CallableShaderRecords;
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_ShaderTableStorage;
	std::array<std::pair<uint32_t, uint32_t>, 4>	a_RecordAndShaderTableSizes;
	std::array<D3D12_GPU_VIRTUAL_ADDRESS, 4>		a_ShaderTableStartAddresses;
	std::vector<uint32_t>							v_RecordBufferStartInBytes;
};

