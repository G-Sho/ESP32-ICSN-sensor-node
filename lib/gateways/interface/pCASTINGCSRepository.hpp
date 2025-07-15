#pragma once

#include "interface/CSRepository.hpp"
#include <unordered_map>
#include <list>
#include <set>
#include <string>
#include <iostream>
#include <random>
#include <painlessmesh.h>

constexpr size_t MAX_CS_TABLE_SIZE = 20;
#define CACHE_ENTRY_TTL_US 1000000000.0 // 1秒間（1,000,000 μs）

class pCASTINGCSRepository : public CSRepository
{
private:
    std::list<std::tuple<std::string, std::string, double>> Q;                                               // {key, value}
    std::unordered_map<std::string, std::list<std::tuple<std::string, std::string, double>>::iterator> iter; // <key, iterator>
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
            double timeStamp = std::get<2>(*it);
            double FR = 1.0 - ((now - timeStamp) / CACHE_ENTRY_TTL_US);
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
        mesh = meshPtr;
    };

    void save(const CSPair &pcsPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    Content get(const ContentName &contentName) override;

    void printCache() const
    {
        Serial.printf("=== Cache Contents ===\n");
        int index = 0;

        double now = mesh ? mesh->getNodeTime() : 0.0;

        for (const auto &entry : Q)
        {
            const std::string &key = std::get<0>(entry);
            const std::string &value = std::get<1>(entry);
            double timestamp = std::get<2>(entry);
            double FR = 1.0 - ((now - timestamp) / CACHE_ENTRY_TTL_US);
            const char *status = (FR <= 0.0) ? "Expired" : "Valid";

            Serial.printf("[%d] Key: %s, Value: %s, Timestamp: %.2f, Freshness: %.2f, Status: %s\n",
                          index++, key.c_str(), value.c_str(), timestamp, FR, status);
        }

        Serial.printf("======================\n\n");
    }
};
