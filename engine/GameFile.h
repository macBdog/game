#ifndef _ENGINE_GAME_FILE_
#define _ENGINE_GAME_FILE_
#pragma once

#include <deque>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "../core/Colour.h"
#include "../core/Vector.h"
#include "../core/Quaternion.h"

#include "DataPack.h"
#include "Log.h"
#include "StringUtils.h"
#include "json.hpp"

struct DataPackEntry;

///\brief GameFile is a general purpose configuration and data file wrapper
///		  backed by nlohmann/json. Data files are stored in standard JSON format.
class GameFile
{
public:

	///\brief A Property wraps a reference to a JSON value and provides typed accessors
	struct Property
	{
		Property() : m_json(nullptr) { }
		Property(nlohmann::json * a_json, const std::string & a_name)
			: m_json(a_json)
			, m_name(a_name) { }

		inline const std::string & GetString() const
		{
			if (m_json && m_json->is_string())
			{
				m_stringCache = m_json->get<std::string>();
				return m_stringCache;
			}
			static const std::string s_empty;
			return s_empty;
		}

		inline int GetInt() const
		{
			if (m_json)
			{
				if (m_json->is_number()) return m_json->get<int>();
				if (m_json->is_string()) return atoi(m_json->get<std::string>().c_str());
			}
			return -1;
		}

		inline float GetFloat() const
		{
			if (m_json)
			{
				if (m_json->is_number()) return m_json->get<float>();
				if (m_json->is_string()) return (float)atof(m_json->get<std::string>().c_str());
			}
			return 0.0f;
		}

		inline bool GetBool() const
		{
			if (m_json)
			{
				if (m_json->is_boolean()) return m_json->get<bool>();
				if (m_json->is_string()) return m_json->get<std::string>().find("true") != std::string::npos;
			}
			return false;
		}

		inline Colour GetColour() const
		{
			if (m_json && m_json->is_array() && m_json->size() >= 4)
			{
				return Colour(
					(*m_json)[0].get<float>(),
					(*m_json)[1].get<float>(),
					(*m_json)[2].get<float>(),
					(*m_json)[3].get<float>());
			}
			return sc_colourWhite;
		}

		inline Vector GetVector() const
		{
			if (m_json && m_json->is_array() && m_json->size() >= 3)
			{
				return Vector(
					(*m_json)[0].get<float>(),
					(*m_json)[1].get<float>(),
					(*m_json)[2].get<float>());
			}
			return Vector::Zero();
		}

		inline Quaternion GetQuaternion() const
		{
			if (m_json && m_json->is_array() && m_json->size() >= 4)
			{
				return Quaternion(
					(*m_json)[0].get<float>(),
					(*m_json)[1].get<float>(),
					(*m_json)[2].get<float>(),
					(*m_json)[3].get<float>());
			}
			return Quaternion();
		}

		inline Vector2 GetVector2() const
		{
			if (m_json && m_json->is_array() && m_json->size() >= 2)
			{
				return Vector2(
					(*m_json)[0].get<float>(),
					(*m_json)[1].get<float>());
			}
			return Vector2::Vector2Zero();
		}

		nlohmann::json * m_json;
		std::string m_name;
		mutable std::string m_stringCache;	///< Keeps string alive for GetString() return
	};

	///\brief An Object wraps a reference to a JSON object node
	struct Object
	{
		Object() : m_json(nullptr) { }
		Object(nlohmann::json * a_json, const std::string & a_name)
			: m_json(a_json)
			, m_name(a_name) { }

		///\brief Find a property (non-object, non-array-of-objects value) by name
		Property * FindProperty(std::string_view a_propertyName);

		///\brief Find a child object by name
		Object * FindObject(std::string_view a_objectName);

		///\brief Get child objects - for arrays of objects (like lights, gameObjects, widgets)
		std::vector<Object *> & GetChildObjects();

		///\brief Get the name of this object
		const std::string & GetName() const { return m_name; }

		nlohmann::json * m_json;
		std::string m_name;

		// Caches for returned pointers (so they remain valid)
		std::deque<Property> m_propertyCache;
		std::vector<std::unique_ptr<Object>> m_childObjectCache;
		std::vector<Object *> m_childPtrCache;
	};

	GameFile() {}
	GameFile(std::string_view a_filePath) { Load(a_filePath); }
	GameFile(DataPackEntry * a_packData) { Load(a_packData); }
	~GameFile() { Unload(); }

	///\brief Load the game file and parse it into data
	bool Load(std::string_view a_filePath);
	bool Load(DataPackEntry * a_packData);

	void Unload();
	inline bool IsLoaded() const { return !m_root.empty(); }

	///\brief Write data from memory to file
	bool Write(std::string_view a_filePath);

	///\brief Accessors to the gamefile property data
	const std::string & GetString(std::string_view a_object, std::string_view a_property) const;
	int GetInt(std::string_view a_object, std::string_view a_property) const;
	float GetFloat(std::string_view a_object, std::string_view a_property) const;
	bool GetBool(std::string_view a_object, std::string_view a_property) const;
	bool GetVector(std::string_view a_object, std::string_view a_property, Vector & a_vec_OUT) const;
	bool GetVector2(std::string_view a_object, std::string_view a_property, Vector2 & a_vec_OUT) const;

	///\brief Add an object that has properties
	Object * AddObject(std::string_view a_objectName, Object * a_parent = nullptr);

	///\brief Add a property with a parent object
	Property * AddProperty(Object * a_parentObject, std::string_view a_propertyName, std::string_view a_value);
	Property * AddProperty(Object * a_parentObject, std::string_view a_propertyName, float a_value);
	Property * AddProperty(Object * a_parentObject, std::string_view a_propertyName, int a_value);
	Property * AddProperty(Object * a_parentObject, std::string_view a_propertyName, bool a_value);
	Property * AddProperty(Object * a_parentObject, std::string_view a_propertyName, const Vector & a_value);
	Property * AddProperty(Object * a_parentObject, std::string_view a_propertyName, const Vector2 & a_value);
	Property * AddProperty(Object * a_parentObject, std::string_view a_propertyName, const Quaternion & a_value);
	Property * AddProperty(Object * a_parentObject, std::string_view a_propertyName, const Colour & a_value);

	///\brief Helper function to find an object by name
	Object * FindObject(std::string_view a_name) const;

private:

	mutable nlohmann::json m_root;						///< The root JSON document

	// Caches for Object wrappers so pointers remain valid
	// Using deque so emplace_back never invalidates existing pointers
	mutable std::deque<Object> m_objectCache;
	mutable std::deque<Property> m_propertyCache;
	mutable std::string m_stringCache;
};

#endif // _ENGINE_GAME_FILE
