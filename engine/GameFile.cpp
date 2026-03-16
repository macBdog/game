#include <fstream>
#include <sstream>

#include "GameFile.h"

// ---- Property finding on Object ----

GameFile::Property * GameFile::Object::FindProperty(const char * a_propertyName)
{
	if (m_json == nullptr || !m_json->is_object())
	{
		return nullptr;
	}

	auto it = m_json->find(a_propertyName);
	if (it == m_json->end())
	{
		return nullptr;
	}

	// Don't return objects or arrays-of-objects as properties
	if (it->is_object())
	{
		return nullptr;
	}
	if (it->is_array() && it->size() > 0 && (*it)[0].is_object())
	{
		return nullptr;
	}

	// Cache the property wrapper so the pointer stays valid
	m_propertyCache.emplace_back(&(*it), std::string(a_propertyName));
	return &m_propertyCache.back();
}

GameFile::Object * GameFile::Object::FindObject(const char * a_objectName)
{
	if (m_json == nullptr || !m_json->is_object())
	{
		return nullptr;
	}

	auto it = m_json->find(a_objectName);
	if (it == m_json->end())
	{
		return nullptr;
	}

	// Must be an object or array of objects
	if (!it->is_object() && !it->is_array())
	{
		return nullptr;
	}

	m_childObjectCache.push_back(std::make_unique<Object>(&(*it), std::string(a_objectName)));
	return m_childObjectCache.back().get();
}

std::vector<GameFile::Object *> & GameFile::Object::GetChildObjects()
{
	m_childPtrCache.clear();

	if (m_json == nullptr)
	{
		return m_childPtrCache;
	}

	if (m_json->is_array())
	{
		// Array of objects - each element is a child
		for (auto & element : *m_json)
		{
			if (element.is_object())
			{
				m_childObjectCache.push_back(std::make_unique<Object>(&element, m_name));
				m_childPtrCache.push_back(m_childObjectCache.back().get());
			}
		}
	}
	else if (m_json->is_object())
	{
		// Object with named children - each key-value pair where value is object
		for (auto it = m_json->begin(); it != m_json->end(); ++it)
		{
			if (it->is_object())
			{
				m_childObjectCache.push_back(std::make_unique<Object>(&(*it), it.key()));
				m_childPtrCache.push_back(m_childObjectCache.back().get());
			}
			else if (it->is_array() && it->size() > 0 && (*it)[0].is_object())
			{
				// Array of objects under a key
				for (auto & element : *it)
				{
					if (element.is_object())
					{
						m_childObjectCache.push_back(std::make_unique<Object>(&element, it.key()));
						m_childPtrCache.push_back(m_childObjectCache.back().get());
					}
				}
			}
		}
	}

	return m_childPtrCache;
}

// ---- GameFile Loading ----

bool GameFile::Load(const char * a_filePath)
{
	Unload();
	std::ifstream file(a_filePath);
	if (!file.is_open())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Could not open game file resource at path %s", a_filePath);
		return false;
	}

	try
	{
		m_root = nlohmann::json::parse(file, nullptr, true, true);
	}
	catch (const nlohmann::json::parse_error & e)
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "JSON parse error in %s: %s", a_filePath, e.what());
		return false;
	}
	return true;
}

bool GameFile::Load(DataPackEntry * a_packData)
{
	Unload();
	if (a_packData == nullptr || a_packData->m_data == nullptr || a_packData->m_size == 0)
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Could not open game datapack file resource.");
		return false;
	}

	try
	{
		std::string jsonStr(a_packData->m_data, a_packData->m_size);
		m_root = nlohmann::json::parse(jsonStr, nullptr, true, true);
	}
	catch (const nlohmann::json::parse_error & e)
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "JSON parse error in datapack entry: %s", e.what());
		return false;
	}
	return true;
}

void GameFile::Unload()
{
	m_root.clear();
	m_objectCache.clear();
	m_propertyCache.clear();
}

bool GameFile::Write(const char * a_filePath)
{
	std::ofstream fileOutput(a_filePath);
	if (!fileOutput.is_open())
	{
		return false;
	}
	fileOutput << m_root.dump(2) << std::endl;
	fileOutput.close();
	return true;
}

// ---- Top-level accessors ----

const char * GameFile::GetString(const char * a_object, const char * a_property) const
{
	auto objIt = m_root.find(a_object);
	if (objIt != m_root.end() && objIt->is_object())
	{
		auto propIt = objIt->find(a_property);
		if (propIt != objIt->end() && propIt->is_string())
		{
			m_stringCache = propIt->get<std::string>();
			return m_stringCache.c_str();
		}
	}
	return "";
}

int GameFile::GetInt(const char * a_object, const char * a_property) const
{
	auto objIt = m_root.find(a_object);
	if (objIt != m_root.end() && objIt->is_object())
	{
		auto propIt = objIt->find(a_property);
		if (propIt != objIt->end())
		{
			if (propIt->is_number()) return propIt->get<int>();
			if (propIt->is_string()) return atoi(propIt->get<std::string>().c_str());
		}
	}
	return -1;
}

float GameFile::GetFloat(const char * a_object, const char * a_property) const
{
	auto objIt = m_root.find(a_object);
	if (objIt != m_root.end() && objIt->is_object())
	{
		auto propIt = objIt->find(a_property);
		if (propIt != objIt->end())
		{
			if (propIt->is_number()) return propIt->get<float>();
			if (propIt->is_string()) return (float)atof(propIt->get<std::string>().c_str());
		}
	}
	return 0.0f;
}

bool GameFile::GetBool(const char * a_object, const char * a_property) const
{
	auto objIt = m_root.find(a_object);
	if (objIt != m_root.end() && objIt->is_object())
	{
		auto propIt = objIt->find(a_property);
		if (propIt != objIt->end())
		{
			if (propIt->is_boolean()) return propIt->get<bool>();
			if (propIt->is_string()) return propIt->get<std::string>().find("true") != std::string::npos;
		}
	}
	return false;
}

bool GameFile::GetVector(const char * a_object, const char * a_property, Vector & a_vec_OUT) const
{
	auto objIt = m_root.find(a_object);
	if (objIt != m_root.end() && objIt->is_object())
	{
		auto propIt = objIt->find(a_property);
		if (propIt != objIt->end() && propIt->is_array() && propIt->size() >= 3)
		{
			a_vec_OUT = Vector((*propIt)[0].get<float>(), (*propIt)[1].get<float>(), (*propIt)[2].get<float>());
			return true;
		}
	}
	return false;
}

bool GameFile::GetVector2(const char * a_object, const char * a_property, Vector2 & a_vec_OUT) const
{
	auto objIt = m_root.find(a_object);
	if (objIt != m_root.end() && objIt->is_object())
	{
		auto propIt = objIt->find(a_property);
		if (propIt != objIt->end() && propIt->is_array() && propIt->size() >= 2)
		{
			a_vec_OUT = Vector2((*propIt)[0].get<float>(), (*propIt)[1].get<float>());
			return true;
		}
	}
	return false;
}

// ---- Object/Property construction ----

GameFile::Object * GameFile::AddObject(const char * a_objectName, Object * a_parent)
{
	if (a_parent == nullptr)
	{
		// Top-level object
		m_root[a_objectName] = nlohmann::json::object();
		m_objectCache.emplace_back(&m_root[a_objectName], std::string(a_objectName));
		return &m_objectCache.back();
	}
	else
	{
		// Check if there is already an array key for child objects of this type
		// For repeated object names (like multiple gameObjects), use an array
		nlohmann::json * parentJson = a_parent->m_json;

		auto it = parentJson->find(a_objectName);
		if (it != parentJson->end())
		{
			// Key exists - if it's an array, append; if object, convert to array
			if (it->is_array())
			{
				it->push_back(nlohmann::json::object());
				nlohmann::json & added = it->back();
				m_objectCache.emplace_back(&added, std::string(a_objectName));
				return &m_objectCache.back();
			}
			else if (it->is_object())
			{
				// Convert single object to array
				nlohmann::json existing = *it;
				*it = nlohmann::json::array();
				it->push_back(existing);
				it->push_back(nlohmann::json::object());
				nlohmann::json & added = it->back();
				m_objectCache.emplace_back(&added, std::string(a_objectName));
				return &m_objectCache.back();
			}
		}

		// New key
		(*parentJson)[a_objectName] = nlohmann::json::object();
		m_objectCache.emplace_back(&(*parentJson)[a_objectName], std::string(a_objectName));
		return &m_objectCache.back();
	}
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, const char * a_propertyName, const char * a_value)
{
	(*a_parentObject->m_json)[a_propertyName] = std::string(a_value);
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[a_propertyName], std::string(a_propertyName));
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, const char * a_propertyName, float a_value)
{
	(*a_parentObject->m_json)[a_propertyName] = a_value;
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[a_propertyName], std::string(a_propertyName));
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, const char * a_propertyName, int a_value)
{
	(*a_parentObject->m_json)[a_propertyName] = a_value;
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[a_propertyName], std::string(a_propertyName));
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, const char * a_propertyName, bool a_value)
{
	(*a_parentObject->m_json)[a_propertyName] = a_value;
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[a_propertyName], std::string(a_propertyName));
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, const char * a_propertyName, const Vector & a_value)
{
	(*a_parentObject->m_json)[a_propertyName] = { a_value.GetX(), a_value.GetY(), a_value.GetZ() };
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[a_propertyName], std::string(a_propertyName));
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, const char * a_propertyName, const Vector2 & a_value)
{
	(*a_parentObject->m_json)[a_propertyName] = { a_value.GetX(), a_value.GetY() };
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[a_propertyName], std::string(a_propertyName));
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, const char * a_propertyName, const Quaternion & a_value)
{
	(*a_parentObject->m_json)[a_propertyName] = { a_value.GetX(), a_value.GetY(), a_value.GetZ(), a_value.GetW() };
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[a_propertyName], std::string(a_propertyName));
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, const char * a_propertyName, const Colour & a_value)
{
	(*a_parentObject->m_json)[a_propertyName] = { a_value.GetR(), a_value.GetG(), a_value.GetB(), a_value.GetA() };
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[a_propertyName], std::string(a_propertyName));
	return &m_propertyCache.back();
}

GameFile::Object * GameFile::FindObject(const char * a_name) const
{
	auto it = m_root.find(a_name);
	if (it != m_root.end() && it->is_object())
	{
		m_objectCache.emplace_back(&(*it), std::string(a_name));
		return &m_objectCache.back();
	}
	return nullptr;
}
