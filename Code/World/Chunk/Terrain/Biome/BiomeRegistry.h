#pragma once

#include "BiomeConfig.h"

#include <string>
#include <unordered_map>
#include <vector>

class BiomeRegistry
{
public:
    void LoadAll(const std::string &directory);

    const BiomeConfig *GetById(const std::string &id) const;

    const std::unordered_map<std::string, BiomeConfig> &GetAll() const { return m_Biomes; }
    const std::vector<std::string> &GetAllSorted() const { return m_SortedIds; }

private:
    std::unordered_map<std::string, BiomeConfig> m_Biomes;
    std::vector<std::string> m_SortedIds;
};