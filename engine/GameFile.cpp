#include <fstream>
#include <sstream>

#include "GameFile.h"

// ---- Property finding on Object ----

GameFile::Property * GameFile::Object::FindProperty(std::string_view a_propertyName)
{
	if (m_json == nullptr || !m_json->is_object())
	{
		return nullptr;
	}

	std::string key(a_propertyName);
	auto it = m_json->find(key);
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
	m_propertyCache.emplace_back(&(*it), key);
	return &m_propertyCache.back();
}

GameFile::Object * GameFile::Object::FindObject(std::string_view a_objectName)
{
	if (m_json == nullptr || !m_json->is_object())
	{
		return nullptr;
	}

	std::string key(a_objectName);
	auto it = m_json->find(key);
	if (it == m_json->end())
	{
		return nullptr;
	}

	// Must be an object or array of objects
	if (!it->is_object() && !it->is_array())
	{
		return nullptr;
	}

	m_childObjectCache.push_back(std::make_unique<Object>(&(*it), key));
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

bool GameFile::Load(std::string_view a_filePath)
{
	Unload();
	std::string path(a_filePath);
	std::ifstream file(path);
	if (!file.is_open())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Could not open game file resource at path %.*s", (int)a_filePath.size(), a_filePath.data());
		return false;
	}

	try
	{
		m_root = nlohmann::json::parse(file, nullptr, true, true);
	}
	catch (const nlohmann::json::parse_error & e)
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "JSON parse error in %.*s: %s", (int)a_filePath.size(), a_filePath.data(), e.what());
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

bool GameFile::Write(std::string_view a_filePath)
{
	std::string path(a_filePath);
	std::ofstream fileOutput(path);
	if (!fileOutput.is_open())
	{
		return false;
	}
	fileOutput << m_root.dump(2) << std::endl;
	fileOutput.close();
	return true;
}

// ---- Top-level accessors ----

const std::string & GameFile::GetString(std::string_view a_object, std::string_view a_property) const
{
	static const std::string s_empty;
	std::string objKey(a_object);
	auto objIt = m_root.find(objKey);
	if (objIt != m_root.end() && objIt->is_object())
	{
		std::string propKey(a_property);
		auto propIt = objIt->find(propKey);
		if (propIt != objIt->end() && propIt->is_string())
		{
			m_stringCache = propIt->get<std::string>();
			return m_stringCache;
		}
	}
	return s_empty;
}

int GameFile::GetInt(std::string_view a_object, std::string_view a_property) const
{
	std::string objKey(a_object);
	auto objIt = m_root.find(objKey);
	if (objIt != m_root.end() && objIt->is_object())
	{
		std::string propKey(a_property);
		auto propIt = objIt->find(propKey);
		if (propIt != objIt->end())
		{
			if (propIt->is_number()) return propIt->get<int>();
			if (propIt->is_string()) return atoi(propIt->get<std::string>().c_str());
		}
	}
	return -1;
}

float GameFile::GetFloat(std::string_view a_object, std::string_view a_property) const
{
	std::string objKey(a_object);
	auto objIt = m_root.find(objKey);
	if (objIt != m_root.end() && objIt->is_object())
	{
		std::string propKey(a_property);
		auto propIt = objIt->find(propKey);
		if (propIt != objIt->end())
		{
			if (propIt->is_number()) return propIt->get<float>();
			if (propIt->is_string()) return (float)atof(propIt->get<std::string>().c_str());
		}
	}
	return 0.0f;
}

bool GameFile::GetBool(std::string_view a_object, std::string_view a_property) const
{
	std::string objKey(a_object);
	auto objIt = m_root.find(objKey);
	if (objIt != m_root.end() && objIt->is_object())
	{
		std::string propKey(a_property);
		auto propIt = objIt->find(propKey);
		if (propIt != objIt->end())
		{
			if (propIt->is_boolean()) return propIt->get<bool>();
			if (propIt->is_string()) return propIt->get<std::string>().find("true") != std::string::npos;
		}
	}
	return false;
}

bool GameFile::GetVector(std::string_view a_object, std::string_view a_property, Vector & a_vec_OUT) const
{
	std::string objKey(a_object);
	auto objIt = m_root.find(objKey);
	if (objIt != m_root.end() && objIt->is_object())
	{
		std::string propKey(a_property);
		auto propIt = objIt->find(propKey);
		if (propIt != objIt->end() && propIt->is_array() && propIt->size() >= 3)
		{
			a_vec_OUT = Vector((*propIt)[0].get<float>(), (*propIt)[1].get<float>(), (*propIt)[2].get<float>());
			return true;
		}
	}
	return false;
}

bool GameFile::GetVector2(std::string_view a_object, std::string_view a_property, Vector2 & a_vec_OUT) const
{
	std::string objKey(a_object);
	auto objIt = m_root.find(objKey);
	if (objIt != m_root.end() && objIt->is_object())
	{
		std::string propKey(a_property);
		auto propIt = objIt->find(propKey);
		if (propIt != objIt->end() && propIt->is_array() && propIt->size() >= 2)
		{
			a_vec_OUT = Vector2((*propIt)[0].get<float>(), (*propIt)[1].get<float>());
			return true;
		}
	}
	return false;
}

// ---- Object/Property construction ----

GameFile::Object * GameFile::AddObject(std::string_view a_objectName, Object * a_parent)
{
	std::string key(a_objectName);
	if (a_parent == nullptr)
	{
		// Top-level object
		m_root[key] = nlohmann::json::object();
		m_objectCache.emplace_back(&m_root[key], key);
		return &m_objectCache.back();
	}
	else
	{
		nlohmann::json * parentJson = a_parent->m_json;

		auto it = parentJson->find(key);
		if (it != parentJson->end())
		{
			if (it->is_array())
			{
				it->push_back(nlohmann::json::object());
				nlohmann::json & added = it->back();
				m_objectCache.emplace_back(&added, key);
				return &m_objectCache.back();
			}
			else if (it->is_object())
			{
				nlohmann::json existing = *it;
				*it = nlohmann::json::array();
				it->push_back(existing);
				it->push_back(nlohmann::json::object());
				nlohmann::json & added = it->back();
				m_objectCache.emplace_back(&added, key);
				return &m_objectCache.back();
			}
		}

		(*parentJson)[key] = nlohmann::json::object();
		m_objectCache.emplace_back(&(*parentJson)[key], key);
		return &m_objectCache.back();
	}
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, std::string_view a_propertyName, std::string_view a_value)
{
	std::string key(a_propertyName);
	(*a_parentObject->m_json)[key] = std::string(a_value);
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[key], key);
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, std::string_view a_propertyName, float a_value)
{
	std::string key(a_propertyName);
	(*a_parentObject->m_json)[key] = a_value;
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[key], key);
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, std::string_view a_propertyName, int a_value)
{
	std::string key(a_propertyName);
	(*a_parentObject->m_json)[key] = a_value;
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[key], key);
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, std::string_view a_propertyName, bool a_value)
{
	std::string key(a_propertyName);
	(*a_parentObject->m_json)[key] = a_value;
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[key], key);
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, std::string_view a_propertyName, const Vector & a_value)
{
	std::string key(a_propertyName);
	(*a_parentObject->m_json)[key] = { a_value.GetX(), a_value.GetY(), a_value.GetZ() };
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[key], key);
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, std::string_view a_propertyName, const Vector2 & a_value)
{
	std::string key(a_propertyName);
	(*a_parentObject->m_json)[key] = { a_value.GetX(), a_value.GetY() };
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[key], key);
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, std::string_view a_propertyName, const Quaternion & a_value)
{
	std::string key(a_propertyName);
	(*a_parentObject->m_json)[key] = { a_value.GetX(), a_value.GetY(), a_value.GetZ(), a_value.GetW() };
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[key], key);
	return &m_propertyCache.back();
}

GameFile::Property * GameFile::AddProperty(Object * a_parentObject, std::string_view a_propertyName, const Colour & a_value)
{
	std::string key(a_propertyName);
	(*a_parentObject->m_json)[key] = { a_value.GetR(), a_value.GetG(), a_value.GetB(), a_value.GetA() };
	m_propertyCache.emplace_back(&(*a_parentObject->m_json)[key], key);
	return &m_propertyCache.back();
}

GameFile::Object * GameFile::FindObject(std::string_view a_name) const
{
	std::string key(a_name);
	auto it = m_root.find(key);
	if (it != m_root.end() && it->is_object())
	{
		m_objectCache.emplace_back(&(*it), key);
		return &m_objectCache.back();
	}
	return nullptr;
}
