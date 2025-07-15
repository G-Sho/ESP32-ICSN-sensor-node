#pragma once

#include "interface/PITRepository.hpp"
#include <unordered_map>
#include <list>
#include <set>
#include <string>
#include <iostream>

constexpr size_t MAX_PIT_TABLE_SIZE = 20;

class LRUPITRepository : public PITRepository {
private:
    std::list<std::pair<std::string, std::set<std::string>>> Q;
    std::unordered_map<std::string, std::list<std::pair<std::string, std::set<std::string>>>::iterator> iter;

public:
    void save(const PITPair &pitPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    DestinationId get(const ContentName &contentName) override;
};
