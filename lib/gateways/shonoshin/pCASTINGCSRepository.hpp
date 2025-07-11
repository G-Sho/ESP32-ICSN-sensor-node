#ifndef INCLUDED_pCASTING_CS_REPOSITORY_hpp_
#define INCLUDED_pCASTING_CS_REPOSITORY_hpp_

#include "interface/pCASTINGCSRepositoryInterface.hpp"
#include <unordered_map>
#include <random>

#define MAX_CS_TABLE_SIZE 20

class pCASTINGCSRepository : public pCASTINGCSRepositoryInterface
{
private:
    std::list<std::pair<std::string, std::string>> Q;                                               // {key, value}
    std::unordered_map<std::string, std::list<std::pair<std::string, std::string>>::iterator> iter; // <key, iterator>
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;

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

public:
    pCASTINGCSRepository()
        : gen(initSeed()), dis(0.0, 1.0) {}

    void save(const CSPair &csPair, const double &EN, const double &OC, const double &FR) override
    {
        const std::string &name = csPair.getContentName().getValue();

        if (pCASTING(EN, OC, FR))
        {
            if (iter.count(name) != 0)
            {
                Q.erase(iter[name]);
            }
            else if (Q.size() >= MAX_CS_TABLE_SIZE)
            {
                auto it = --Q.end();
                const std::string &k = it->first;
                iter.erase(k);
                Q.pop_back();
            }
            Q.push_front({name, csPair.getContent().getValue()});
            iter[name] = Q.begin();
        }
    };

    void remove(const ContentName &contentName) override
    {
        const std::string &name = contentName.getValue();
        if (iter.count(name) != 0)
        {
            Q.erase(iter[name]);
            iter.erase(name);
        }
    };

    bool find(const ContentName &contentName) override
    {
        return iter.count(contentName.getValue()) != 0;
    };

    Content get(const ContentName &contentName) override
    {
        const std::string &name = contentName.getValue();
        if (iter.count(name) == 0)
            return Content::Null();

        auto it = iter[name];
        const std::string &v = it->second;
        Q.erase(it);
        Q.push_front({name, v});
        iter[name] = Q.begin();
        return Content(v);
    };
};

#endif // INCLUDED_pCASTING_CS_REPOSITORY_hpp_