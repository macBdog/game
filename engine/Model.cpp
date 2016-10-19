#include <iostream>
#include <fstream>

#include "Log.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Model.h"

using namespace std;	// For iostream resources

bool Model::Load(const char * a_modelFilePath, ModelDataPool & a_modelData)
{
	// Early out for no file case
	if (a_modelFilePath == NULL)
	{
		return false;
	}

	// Set name and pass path into load so the material is loaded adjacent to the model
	sprintf(m_name, "%s", StringUtils::ExtractFileNameFromPath(a_modelFilePath));
	char modelPath[StringUtils::s_maxCharsPerLine];
	strncpy(modelPath, a_modelFilePath, StringUtils::s_maxCharsPerLine);
	StringUtils::TrimFileNameFromPath(modelPath);

	ifstream file(a_modelFilePath, ios::binary);
	return LoadData<ifstream>(file, a_modelData, modelPath);
}

bool Model::Load(DataPackEntry * a_packedModel, ModelDataPool & a_modelDataPool, DataPack * a_dataPack)
{
	// Set name from pack entry
	sprintf(m_name, "%s", StringUtils::ExtractFileNameFromPath(a_packedModel->m_path));
	char modelPath[StringUtils::s_maxCharsPerLine];
	strncpy(modelPath, a_packedModel->m_path, StringUtils::s_maxCharsPerLine);
	StringUtils::TrimFileNameFromPath(modelPath);

	return LoadData<DataPackEntry>(*a_packedModel, a_modelDataPool, modelPath, a_dataPack);
}

bool Model::Unload()
{
	ObjectNode * curObjectNode = m_objects.GetHead();
	while (curObjectNode != NULL)
	{
		// Deallocate memory allocated during mode load
		Object * curObject = curObjectNode->GetData();

		free(curObject->GetVertices());
		free(curObject->GetNormals());
		free(curObject->GetUvs());

		curObject->SetVertices(NULL);
		curObject->SetNormals(NULL);
		curObject->SetUvs(NULL);

		// Deallocate memory for the object storage in the model list
		ObjectNode * next = curObjectNode->GetNext();
		m_objects.Remove(curObjectNode);
		delete curObject;
		delete curObjectNode;
		curObjectNode = next;
	}
	return true;
}

bool Material::Load(const char * a_materialFileName, const char * a_materialName)
{
	// Early out for no file case
	if (a_materialFileName == NULL)
	{
		return false;
	}

	ifstream file(a_materialFileName);
	return LoadData<ifstream>(file, a_materialName);
}

bool Material::Load(DataPackEntry * a_packedMaterial, const char * a_materialName)
{
	if (a_packedMaterial != NULL)
	{
		return LoadData<DataPackEntry>(*a_packedMaterial, a_materialName);
	}
	return false;
}
