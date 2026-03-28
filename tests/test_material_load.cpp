// Test harness for Material MTL file loading
// Isolates the LoadData parsing logic from Model.h to test without the full engine
//
// Build: bazel build //tests:test_material_load
// Run:   bazel-bin/tests/test_material_load.exe

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <filesystem>

#include "../engine/StringUtils.h"

// Replicate the Material::LoadData parsing logic exactly as in Model.h
struct TestMaterial
{
	std::string m_name;
	float m_ambientR = 0, m_ambientG = 0, m_ambientB = 0;
	float m_diffuseR = 0, m_diffuseG = 0, m_diffuseB = 0;
	float m_specularR = 0, m_specularG = 0, m_specularB = 0;
	int m_shininess = 0;
	std::string m_diffuseTexName;
	std::string m_normalTexName;
	std::string m_specularTexName;
	bool m_found = false;

	bool LoadFromFile(std::string_view a_materialFileName, std::string_view a_materialName)
	{
		std::string path(a_materialFileName);
		std::ifstream file(path);
		return LoadData(file, a_materialName);
	}

	// Exact copy of Material::LoadData from Model.h with texture loads replaced by string captures
	template <typename TInputData>
	bool LoadData(TInputData & a_input, std::string_view a_materialName)
	{
		char line[512];
		memset(&line, 0, sizeof(char) * 512);

		unsigned int lineCount = 0;
		bool foundTargetMaterial = false;

		if (a_input.is_open())
		{
			while (a_input.good())
			{
				a_input.getline(line, 512);
				lineCount++;
				std::string strLine(line);

				// Comment
				if (strLine.find('#') != std::string::npos)
				{
					continue;
				}
				// Material library declaration
				else if (strLine.find("newmtl") != std::string::npos)
				{
					if (foundTargetMaterial)
					{
						a_input.close();
						m_found = true;
						return true;
					}

					char tempName[256];
					tempName[0] = '\0';
					sscanf(line, "newmtl %s", tempName);
					m_name = tempName;

					foundTargetMaterial = m_name.find(a_materialName) != std::string::npos;
					continue;
				}
				else if (foundTargetMaterial)
				{
					float matX, matY, matZ = 0.0f;

					if (strLine.find("map_Kd") != std::string::npos)
					{
						char tempMatName[512];
						memset(&tempMatName, 0, sizeof(char) * 512);
						if (strLine.find("\\") == std::string::npos)
						{
							sscanf(line, "map_Kd %s", tempMatName);
						}
						else
						{
							std::string extracted = StringUtils::ExtractFileNameFromPath(line);
							sscanf(extracted.c_str(), "%s", tempMatName);
						}
						m_diffuseTexName = tempMatName;
					}
					else if (strLine.find("map_Bump") != std::string::npos)
					{
						char tempMatName[512];
						memset(&tempMatName, 0, sizeof(char) * 512);
						if (strLine.find("\\") == std::string::npos)
						{
							sscanf(line, "map_Bump %s", tempMatName);
						}
						else
						{
							std::string extracted = StringUtils::ExtractFileNameFromPath(line);
							sscanf(extracted.c_str(), "%s", tempMatName);
						}
						m_normalTexName = tempMatName;
					}
					else if (strLine.find("map_Ks") != std::string::npos)
					{
						char tempMatName[512];
						memset(&tempMatName, 0, sizeof(char) * 512);
						if (strLine.find("\\") == std::string::npos)
						{
							sscanf(line, "map_Ks %s", tempMatName);
						}
						else
						{
							std::string extracted = StringUtils::ExtractFileNameFromPath(line);
							sscanf(extracted.c_str(), "%s", tempMatName);
						}
						m_specularTexName = tempMatName;
					}
					else if (strLine.find("Ka") != std::string::npos)
					{
						sscanf(line, "Ka %f %f %f", &matX, &matY, &matZ);
						m_ambientR = matX; m_ambientG = matY; m_ambientB = matZ;
					}
					else if (strLine.find("Kd") != std::string::npos)
					{
						sscanf(line, "Kd %f %f %f", &matX, &matY, &matZ);
						m_diffuseR = matX; m_diffuseG = matY; m_diffuseB = matZ;
					}
					else if (strLine.find("Ks") != std::string::npos)
					{
						sscanf(line, "Ks %f %f %f", &matX, &matY, &matZ);
						m_specularR = matX; m_specularG = matY; m_specularB = matZ;
					}
					else if (strLine.find("Ns") != std::string::npos)
					{
						float shininess = 0.0f;
						sscanf(line, "Ns %f", &shininess);
						m_shininess = (int)shininess;
					}
				}
			}
			a_input.close();

			if (foundTargetMaterial)
			{
				m_found = true;
				return true;
			}
		}
		return false;
	}
};

void PrintMaterial(const TestMaterial & mat, const char * label)
{
	printf("--- %s ---\n", label);
	printf("  found:    %s\n", mat.m_found ? "YES" : "NO");
	printf("  name:     '%s'\n", mat.m_name.c_str());
	printf("  ambient:  %.3f %.3f %.3f\n", mat.m_ambientR, mat.m_ambientG, mat.m_ambientB);
	printf("  diffuse:  %.3f %.3f %.3f\n", mat.m_diffuseR, mat.m_diffuseG, mat.m_diffuseB);
	printf("  specular: %.3f %.3f %.3f\n", mat.m_specularR, mat.m_specularG, mat.m_specularB);
	printf("  shiny:    %d\n", mat.m_shininess);
	printf("  diffTex:  '%s'\n", mat.m_diffuseTexName.c_str());
	printf("  normTex:  '%s'\n", mat.m_normalTexName.c_str());
	printf("  specTex:  '%s'\n", mat.m_specularTexName.c_str());
}

struct TestCase
{
	const char * mtlFile;
	const char * materialName;
	const char * label;

	// Expected values (nullptr = don't check)
	bool expectFound;
	const char * expectName;
	const char * expectDiffTex;
	int expectShininess;
};

// Simulate the full pipeline: OBJ file → extract mtllib → build material path → load material
// This mirrors what Model::LoadData does
bool TestObjToMaterialPipeline(const char * objPath, const char * expectedMatName, TestMaterial & mat_OUT)
{
	std::string path(objPath);
	std::ifstream objFile(path);
	if (!objFile.is_open())
	{
		printf("  ERROR: Could not open OBJ file: %s\n", objPath);
		return false;
	}

	// Step 1: TrimFileNameFromPath to get directory (same as Model::Load line 20)
	std::string modelPath = StringUtils::TrimFileNameFromPath(objPath);
	printf("  modelPath (from Load): '%s'\n", modelPath.c_str());

	// Step 2: Inside LoadData, TrimFileNameFromPath is called AGAIN on modelPath (line 339)
	std::string materialFilePath = StringUtils::TrimFileNameFromPath(modelPath);
	printf("  materialFilePath (after double trim): '%s'\n", materialFilePath.c_str());

	// Step 3: Parse OBJ to find mtllib and usemtl
	char line[512];
	std::string mtlLibName;
	std::string usemtlName;

	while (objFile.good())
	{
		objFile.getline(line, 512);
		std::string strLine(line);

		if (strLine.find('#') != std::string::npos)
			continue;

		if (strLine.find("mtllib") != std::string::npos)
		{
			char tempName[256];
			tempName[0] = '\0';
			sscanf(line, "mtllib %s", tempName);
			mtlLibName = tempName;

			// This mirrors Model.h line 381
			materialFilePath = StringUtils::AppendString(materialFilePath, mtlLibName);
			printf("  mtllib: '%s'\n", mtlLibName.c_str());
			printf("  materialFilePath (after append): '%s'\n", materialFilePath.c_str());
		}

		if (strLine.find("usemtl") != std::string::npos && usemtlName.empty())
		{
			char tempName[256];
			tempName[0] = '\0';
			sscanf(line, "usemtl %s", tempName);
			usemtlName = tempName;
			printf("  usemtl: '%s'\n", usemtlName.c_str());
		}
	}
	objFile.close();

	if (materialFilePath.empty() || usemtlName.empty())
	{
		printf("  ERROR: Could not find mtllib or usemtl in OBJ\n");
		return false;
	}

	// Step 4: Load the material (same as Model.h line 430)
	printf("  Loading material '%s' from '%s'\n", usemtlName.c_str(), materialFilePath.c_str());
	bool result = mat_OUT.LoadFromFile(materialFilePath, usemtlName);
	printf("  Load result: %s\n", result ? "OK" : "FAILED");
	return result;
}

int main()
{
	const char * meshDir = "c:/Projects/Galagus/mesh/";

	TestCase tests[] = {
		// spaceFighter.mtl has: newmtl None, Ns 500, Ka/Kd/Ks 0.8, no textures
		{ "spaceFighter.mtl", "None", "spaceFighter/None", true, "None", "", 500 },

		// enemyBee.mtl has: newmtl enemyBeeMat_enemyBee.tga, map_Kd tex\enemyBee.tga
		{ "enemyBee.mtl", "enemyBeeMat", "enemyBee/enemyBeeMat", true, "enemyBeeMat_enemyBee.tga", "enemyBee.tga", 96 },

		// star.mtl has two materials: starMat (map_Kd tex\\star.tga) and starMat_star.tga (map_Kd tex\star.tga)
		{ "star.mtl", "starMat", "star/starMat (first)", true, "starMat", "star.tga", 96 },
		{ "star.mtl", "starMat_star", "star/starMat_star (second)", true, "starMat_star.tga", "star.tga", 96 },

		// logo.mtl
		{ "logo.mtl", "logo", "logo", true, nullptr, nullptr, -1 },

		// missile.mtl
		{ "missile.mtl", "missile", "missile", true, nullptr, nullptr, -1 },
	};

	int passed = 0;
	int failed = 0;
	int total = sizeof(tests) / sizeof(tests[0]);

	for (auto & t : tests)
	{
		std::string fullPath = std::string(meshDir) + t.mtlFile;
		TestMaterial mat;
		mat.LoadFromFile(fullPath, t.materialName);
		PrintMaterial(mat, t.label);

		bool ok = true;

		if (mat.m_found != t.expectFound)
		{
			printf("  FAIL: expected found=%s, got %s\n", t.expectFound ? "YES" : "NO", mat.m_found ? "YES" : "NO");
			ok = false;
		}
		if (t.expectName != nullptr && mat.m_name != t.expectName)
		{
			printf("  FAIL: expected name='%s', got '%s'\n", t.expectName, mat.m_name.c_str());
			ok = false;
		}
		if (t.expectDiffTex != nullptr && mat.m_diffuseTexName != t.expectDiffTex)
		{
			printf("  FAIL: expected diffTex='%s', got '%s'\n", t.expectDiffTex, mat.m_diffuseTexName.c_str());
			ok = false;
		}
		if (t.expectShininess >= 0 && mat.m_shininess != t.expectShininess)
		{
			printf("  FAIL: expected shininess=%d, got %d\n", t.expectShininess, mat.m_shininess);
			ok = false;
		}

		if (ok)
		{
			printf("  PASS\n");
			passed++;
		}
		else
		{
			failed++;
		}
		printf("\n");
	}

	printf("=== Isolated parsing: %d/%d passed, %d failed ===\n\n", passed, total, failed);

	// Pipeline tests: simulate full OBJ → MTL flow
	printf("=== Pipeline Tests (OBJ -> MTL) ===\n\n");
	const char * objFiles[] = {
		"c:/Projects/Galagus/mesh/enemyBee.obj",
		"c:/Projects/Galagus/mesh/star.obj",
		"c:/Projects/Galagus/mesh/spaceFighter.obj",
		"c:/Projects/Galagus/mesh/logo.obj",
		"c:/Projects/Galagus/mesh/missile.obj",
	};

	int pipelinePassed = 0;
	int pipelineTotal = sizeof(objFiles) / sizeof(objFiles[0]);

	for (auto * objFile : objFiles)
	{
		printf("--- Pipeline: %s ---\n", objFile);
		TestMaterial mat;
		if (TestObjToMaterialPipeline(objFile, nullptr, mat))
		{
			PrintMaterial(mat, "  Result");
			if (mat.m_found)
			{
				printf("  PASS\n");
				pipelinePassed++;
			}
			else
			{
				printf("  FAIL: material not found\n");
			}
		}
		else
		{
			printf("  FAIL: pipeline returned false\n");
		}
		printf("\n");
	}

	printf("=== Pipeline: %d/%d passed ===\n", pipelinePassed, pipelineTotal);
	return (failed > 0 || pipelinePassed < pipelineTotal) ? 1 : 0;
}
