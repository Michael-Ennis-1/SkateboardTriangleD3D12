#include "sktbdpch.h"
#include "MeshEngine.h"
#include "Skateboard/Renderer/Model/Model.h"

namespace Skateboard
{
	

	MeshletModel* MeshletBank::AddMeshletModel(const std::wstring_view& name, MeshletModel* model)
	{

		if(m_MeshletModels.contains(name))
		{
			SKTBD_APP_WARN("A model with that name already exists!");
			return m_MeshletModels[name].get();
		}

		std::shared_ptr<MeshletModel> pModel(model);

		m_MeshletModels.emplace(name, std::move(model));
		
		return m_MeshletModels[name].get();
	}

	MeshletModel* MeshletBank::AddMeshletModel(const std::wstring_view& name, std::unique_ptr<MeshletModel>& model)
	{
		if (m_MeshletModels.contains(name))
		{
			SKTBD_APP_WARN("A model with that name already exists!");
			return m_MeshletModels[name].get();
		}

		m_MeshletModels.emplace(name, std::move(model));

		return m_MeshletModels[name].get();
	}

	MeshletModel* MeshletBank::Get(const std::wstring_view& name)
	{
		if (!m_MeshletModels.contains(name))
		{
			SKTBD_APP_WARN("A model with that name does not exist!");
			return nullptr;
		}

		return m_MeshletModels[name].get();
	}

	bool MeshletBank::Contains(const std::wstring_view& name)
	{
		return m_MeshletModels.contains(name);
	}

	bool MeshletBank::ReleaseModel(const std::wstring_view& name)
	{
		if (!m_MeshletModels.contains(name))
		{
			SKTBD_APP_WARN("A model with that name does not exist!");
			return false;
		}

		auto* pModel = m_MeshletModels[name].get();
		pModel->Release();
		
		m_MeshletModels.erase(name);

		return true;
	}

	Model* MeshBank::AddModel(const std::wstring_view& name, Model* model)
	{
		if(m_Models.contains(name))
		{
			SKTBD_APP_WARN("A model with that name already exists!");
			return m_Models[name].get();
		}

		std::shared_ptr<Model> pModel(model);
		m_Models.emplace(name, std::move(model));

		return m_Models[name].get();
	}

	Model* MeshBank::AddModel(const std::wstring_view& name, std::unique_ptr<Model>& model)
	{
		if (m_Models.contains(name))
		{
			SKTBD_APP_WARN("A model with that name already exists!");
			return m_Models[name].get();
		}

		m_Models.emplace(name, std::move(model));

		return m_Models[name].get();
	}

	Model* MeshBank::Get(const std::wstring_view& name)
	{
		if (!m_Models.contains(name))
		{
			SKTBD_APP_WARN("A model with that name does not exist!");
			return nullptr;
		}

		return m_Models[name].get();
	}

	bool MeshBank::Contains(const std::wstring_view& name)
	{
		return m_Models.contains(name);
	}

	bool MeshBank::ReleaseModel(const std::wstring_view& name)
	{

		if (!m_Models.contains(name))
		{
			SKTBD_APP_WARN("A model with that name does not exist!");
			return false;
		}

		auto* pModel = m_Models[name].get();
		pModel->Release();

		m_Models.erase(name);

		return true;
	}
}

