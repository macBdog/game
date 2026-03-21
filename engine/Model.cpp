#include <iostream>
#include <fstream>

#include "Log.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Model.h"

bool Model::Load(std::string_view a_modelFilePath, ModelDataPool & a_modelData)
{
	// Early out for no file case
	if (a_modelFilePath.empty())
	{
		return false;
	}

	// Set name and pass path into load so the material is loaded adjacent to the model
	m_name = StringUtils::ExtractFileNameFromPath(a_modelFilePath);
	std::string modelPath = StringUtils::TrimFileNameFromPath(a_modelFilePath);

	std::ifstream file(std::string(a_modelFilePath), std::ios::binary);
	return LoadData<std::ifstream>(file, a_modelData, modelPath);
}

bool Model::Load(DataPackEntry * a_packedModel, ModelDataPool & a_modelDataPool, DataPack * a_dataPack)
{
	// Set name from pack entry
	m_name = StringUtils::ExtractFileNameFromPath(a_packedModel->GetPath());
	std::string modelPath = StringUtils::TrimFileNameFromPath(a_packedModel->GetPath());

	return LoadData<DataPackEntry>(*a_packedModel, a_modelDataPool, modelPath, a_dataPack);
}

bool Model::Unload()
{
	ObjectNode * curObjectNode = m_objects.GetHead();
	while (curObjectNode != nullptr)
	{
		// Deallocate memory allocated during mode load
		Object * curObject = curObjectNode->GetData();

		free(curObject->GetVertices());
		free(curObject->GetNormals());
		free(curObject->GetUvs());

		curObject->SetVertices(nullptr);
		curObject->SetNormals(nullptr);
		curObject->SetUvs(nullptr);

		// Deallocate memory for the object storage in the model list
		ObjectNode * next = curObjectNode->GetNext();
		m_objects.Remove(curObjectNode);
		delete curObject;
		delete curObjectNode;
		curObjectNode = next;
	}
	return true;
}

bool Material::Load(std::string_view a_materialFileName, std::string_view a_materialName)
{
	// Early out for no file case
	if (a_materialFileName.empty())
	{
		return false;
	}

	std::ifstream file(std::string(a_materialFileName));
	return LoadData<std::ifstream>(file, a_materialName);
}

bool Material::Load(DataPackEntry * a_packedMaterial, std::string_view a_materialName)
{
	if (a_packedMaterial != nullptr)
	{
		return LoadData<DataPackEntry>(*a_packedMaterial, a_materialName);
	}
	return false;
}
