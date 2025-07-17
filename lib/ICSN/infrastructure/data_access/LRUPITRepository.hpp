#pragma once

#include "data_access/IPITRepository.hpp"
#include <unordered_map>
#include <list>
#include <set>
#include <string>
#include <iostream>

class LRUPITRepository : public IPITRepository {
private:
    std::list<std::pair<std::string, std::set<std::string>>> Q;
    std::unordered_map<std::string, std::list<std::pair<std::string, std::set<std::string>>>::iterator> iter;

public:
    void save(const PITPair &pitPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    DestinationId get(const ContentName &contentName) override;
};
