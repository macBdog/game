#include "StringUtils.h"

#include <algorithm>
#include <cstring>
#include <filesystem>

std::string StringUtils::ExtractField(std::string_view a_buffer, std::string_view a_delim, unsigned int a_fieldIndex)
{
    if (a_buffer.empty() || a_buffer.find(a_delim) == std::string_view::npos)
    {
        return {};
    }

    // Skip leading delimiters
    while (a_buffer.substr(0, a_delim.size()) == a_delim)
    {
        a_buffer.remove_prefix(a_delim.size());
    }

    // Walk through fields
    size_t start = 0;
    size_t end = std::string_view::npos;

    for (unsigned int i = 0; i <= a_fieldIndex; ++i)
    {
        if (i > 0)
        {
            start = end + a_delim.size();
            if (start >= a_buffer.size())
            {
                return {};
            }
        }
        end = a_buffer.find(a_delim, start);
        if (end == std::string_view::npos)
        {
            if (i < a_fieldIndex)
            {
                return {};
            }
            end = a_buffer.size();
        }
    }

    if (start >= a_buffer.size() || start == end)
    {
        return {};
    }

    return TrimString(a_buffer.substr(start, end - start), true);
}

std::string StringUtils::ExtractPropertyName(std::string_view a_buffer, std::string_view a_delim)
{
    if (a_buffer.empty())
    {
        return {};
    }

    size_t delimPos = a_buffer.find(a_delim);
    if (delimPos == std::string_view::npos)
    {
        return {};
    }

    return TrimString(a_buffer.substr(0, delimPos));
}

std::string StringUtils::ExtractValue(std::string_view a_buffer, std::string_view a_delim)
{
    if (a_buffer.empty())
    {
        return {};
    }

    size_t delimPos = a_buffer.find(a_delim);
    if (delimPos == std::string_view::npos)
    {
        return {};
    }

    return TrimString(a_buffer.substr(delimPos + a_delim.size()));
}

std::string_view StringUtils::ExtractFileNameFromPath(std::string_view a_buffer)
{
    std::filesystem::path file_path(a_buffer);
    return file_path.filename().string();
}

std::string StringUtils::TrimFileNameFromPath(std::string_view a_path)
{
    if (a_path.empty())
    {
        return {};
    }

    size_t lastSlash = a_path.find_last_of("/\\");
    if (lastSlash == std::string_view::npos)
    {
        return {};
    }

    return std::string(a_path.substr(0, lastSlash + 1));
}

std::string StringUtils::TrimString(std::string_view a_buffer, bool a_trimQuotes)
{
    if (a_buffer.empty())
    {
        return {};
    }

    std::string result;
    result.reserve(a_buffer.size());

    for (char c : a_buffer)
    {
        // Remove whitespace characters
        if (c == '\n' || c == ' ' || c == '\t' || c == ';')
        {
            continue;
        }
        // Remove quotes if requested
        if (a_trimQuotes && c == '"')
        {
            continue;
        }
        result += c;
    }

    return result;
}

std::string StringUtils::ReadLine(FILE *a_filePointer)
{
    char buf[s_maxCharsPerLine];

    do {
        if (fgets(buf, s_maxCharsPerLine, a_filePointer) == nullptr)
        {
            return {};
        }

        if (feof(a_filePointer))
        {
            return {};
        }
    } while (buf[0] == '/' || buf[0] == '\n' || buf[0] == '\r');

    // Remove trailing carriage return
    char * cr = strchr(buf, '\r');
    if (cr)
    {
        *cr = '\0';
    }

    return std::string(buf);
}

unsigned char StringUtils::ConvertToLower(unsigned char a_char)
{
    if (a_char >= 'A' && a_char <= 'Z')
    {
        return a_char + ('a' - 'A');
    }
    return a_char;
}

unsigned int StringUtils::CountCharacters(std::string_view a_string, char a_searchChar)
{
    unsigned int count = 0;
    for (char c : a_string)
    {
        if (c == a_searchChar)
        {
            ++count;
        }
    }
    return count;
}

std::string StringUtils::PrependString(std::string_view a_buffer, std::string_view a_prefix)
{
    std::string result;
    result.reserve(a_prefix.size() + a_buffer.size());
    result.append(a_prefix);
    result.append(a_buffer);
    return result;
}

std::string StringUtils::AppendString(std::string_view a_buffer, std::string_view a_suffix)
{
    std::string result;
    result.reserve(a_buffer.size() + a_suffix.size());
    result.append(a_buffer);
    result.append(a_suffix);
    return result;
}

std::string_view StringUtils::BoolToString(bool a_input)
{
    return a_input ? "true" : "false";
}
