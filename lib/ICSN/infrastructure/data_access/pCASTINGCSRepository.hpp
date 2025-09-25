#pragma once

#include "data_access/ICSRepository.hpp"
#include "config/Config.hpp"
#include <unordered_map>
#include <list>
#include <set>
#include <string>
#include <iostream>
#include <random>
#include <painlessmesh.h>

class pCASTINGCSRepository : public ICSRepository
{
private:
    std::list<std::tuple<std::string, std::string, uint32_t>> Q;                                               // {key, value}
    std::unordered_map<std::string, std::list<std::tuple<std::string, std::string, uint32_t>>::iterator> iter; // <key, iterator>
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;
    painlessMesh *mesh = nullptr;

    static std::mt19937::result_type initSeed()
    {
        static std::random_device rd;
        return rd(); // 安全に一度だけ呼ばれる
    }

    double computeFu(const double &EN, const double &OC, const double &FR, const double &w1 = 1.0 / 3, const double &w2 = 1.0 / 3, const double &w3 = 1.0 / 3, const int &n = 1)
    {
        // 鮮度が負（期限切れ）の場合、キャッシュしない
        if (FR <= 0.0)
            return 0.0;

        double term1 = w1 * std::pow(EN, n);
        double term2 = w2 * std::pow(1 - OC, n);
        double term3 = w3 * std::pow(FR, n);

        return term1 + term2 + term3; // Fu を返す
    }

    bool pCASTING(const double &EN, const double &OC, const double &FR)
    {
        double randVal = dis(gen); // 0〜1の一様分布
        return (randVal <= computeFu(EN, OC, FR));
    }

    void removeExpiredEntries()
    {
        auto now = mesh->getNodeTime();
        for (auto it = Q.begin(); it != Q.end();)
        {
            uint32_t timeStamp = std::get<2>(*it);
            double FR = 1.0 - ((double)(now - timeStamp) / (double)systemConfig.cacheEntryTtlUs);
            if (FR <= 0.0)
            {
                iter.erase(std::get<0>(*it));
                it = Q.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

public:
    pCASTINGCSRepository()
        : gen(initSeed()), dis(0.0, 1.0) {}

    void setMesh(painlessMesh *meshPtr)
    {
        if (meshPtr == nullptr)
        {
            Serial.println("Error: Mesh pointer is null.");
            return;
        }
        mesh = meshPtr;
    };

    void save(const CSPair &pcsPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    Content get(const ContentName &contentName) override;

    void printCache() const
    {
        Serial.printf("=== Content Store ===\n");
        int index = 0;

        uint32_t now = mesh ? mesh->getNodeTime() : 0;

        for (const auto &entry : Q)
        {
            const std::string &key = std::get<0>(entry);
            const std::string &value = std::get<1>(entry);
            uint32_t timestamp = std::get<2>(entry);
            double FR = 1.0 - ((double)(now - timestamp) / (double)systemConfig.cacheEntryTtlUs);
            const char *status = (FR <= 0.0) ? "Expired" : "Valid";

            // Print cache entry
            Serial.printf("[%d] Key: %s, Value: %s, Timestamp: %lu, Freshness: %.2f, Status: %s\n",
                          index++, key.c_str(), value.c_str(), timestamp, FR, status);
        }

        Serial.printf("======================\n\n");
    }
};
