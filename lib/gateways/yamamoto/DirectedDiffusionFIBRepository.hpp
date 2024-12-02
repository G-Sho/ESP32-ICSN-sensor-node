#ifndef INCLUDED_DIRECTEDDIFFUSION_FIB_REPOSITORY_hpp_
#define INCLUDED_DIRECTEDDIFFUSION_FIB_REPOSITORY_hpp_

#include "interface/FIBRepository.hpp"
#include <map>       //std::map
#include <algorithm> // std::min
#include <utility>
#include <queue>
#include <ctime>
#include <random> //random

#define THRESHOLD 5
#define MAX_PRIORITY 100
// 優先度の最大値

class DirectedDiffusionFIBRepository : public FIBRepository
{
private:
    // FIB本体はstring，doublepairのvectorを含むMAP
    std::map<std::string, std::set<std::pair<std::string, double>>> a_fib;
    std::queue<std::string> a_fibManagement;
    int const m_maxSize = 100;

    // content IDと一致した配列を引数とし，複数の文字列型配列を戻り値とした関数．
    std::set<std::string> pickupFibElement(std::set<std::pair<std::string, double>> dest)
    {
        double max = 0.0;
        for (auto &x : dest)
        {
            if (x.second > max)
                max = x.second;
        }
        double sum = 0.0;
        for (auto &x : dest)
        {
            sum += x.second;
        }
        // 優先度の平均を求める
        double average = sum / dest.size();

        // 戻り値にするノードの数を端数切り捨てで求める．全体に対する割合は平均を最大で割った値の逆数
        int nodeNum = static_cast<unsigned int>(dest.size() * (average / max));
        if (nodeNum < 1)
            nodeNum = 1;

        std::mt19937 rd(static_cast<unsigned int>(std::time(nullptr)));
        std::uniform_int_distribution<double> dist(0.0, 1.0);

        // 戻り値用のfibリスト
        std::set<std::pair<std::string, double>> fib_tmp = dest;
        std::set<std::string> f_fib;

        // 他に渡すFIBのリストを作成．計算量が増加しそうなので要改善
        while ((f_fib.size() < nodeNum) || (fib_tmp.size() > 0))
        {
            for (auto it = fib_tmp.begin(); it != fib_tmp.end(); ++it)
            {
                // auto num =
                double prob = it->second / max;
                // 送信確率は10％以上
                if (prob <= 0.1)
                {
                    prob = 0.1;
                }
                if (dist(rd) < prob)
                {
                    f_fib.insert(it->first);
                    fib_tmp.erase(it);
                    break;
                }
                fib_tmp.erase(it);
            }
        }
        // 複数選んだFIBを返す
        return f_fib;
    };

    // FIBの優先度が最大数に到達したら該当Content IDの優先度を全てスケールダウンする
    void scaleDownPriority(const &std::string, ) // contentName contentName){
        for (auto it = a_fib[contentName.getValue()].begin(); it != a_fib[contentName.getValue()].end(); ++it)
    {
        *it.second /= (MAX_PRIORITY * 10);
        // スケールダウン時に優先度の低いFIBは削除
        if (it.second < 0.1)
            a_fib[contentName.getValue()].erase(it);
    }

public:
    void saveForDynamic(FIBPair fibPair, std::string faceId, int hopCount) override
    {
        double priority = 0;
        auto it = a_fib.find(fibPair.getContentName().getValue());
        if (it != a_fib.end())
        {
            for (auto &x : a_fib[fibPair.getContentName().getValue()])
            {
                if (x.first == faceId)
                {
                    x.second += (1.0 / static_cast<double>(hopCount));
                    if (x.second > MAX_PRIORITY)
                        scaleDownPriority(fibPair.getContentName());
                    return;
                }
            }
            return;
        }

        // FIBのサイズが100を超えないように消す
        while (m_maxSize <= a_fibManagement.size())
        {
            auto it = a_fib.find(a_fibManagement.front());
            if (it != a_fib.end())
                a_fib.erase(a_fibManagement.front());
            a_fibManagement.pop();
        }
        a_fib[fibPair.getContentName().getValue()] = fibPair.getDestinationId().getValue();
        for (auto &x : a_fib[fibPair.getContentName().getValue()])
        {
            x.second = (1.0 / static_cast<double>(hopCount));
            // 優先度の初期値：1.0
        }
        a_fibManagement.push(fibPair.getContentName().getValue());
    };

    void remove(ContentName contentName) override
    {
        auto it = a_fib.find(contentName.getValue());
        if (it != a_fib.end())
            a_fib.erase(contentName.getValue());
    };

    bool find(ContentName contentName) override
    {
        auto it = a_fib.find(contentName.getValue());
        return (it != a_fib.end());
    };

    // contentNameに一致するノードID（Face）を複数取得して返す
    DestinationId get(ContentName contentName) override
    {
        auto it = a_fib.find(contentName.getValue());
        if (it == a_fib.end())
        {
            return DestinationId({"NULL"});
        }
        else
        {
            // 動作を分けた
            return DestinationId(pickupFibElement(a_fib[contentName.getValue()]));
        }
    };
};

#endif // INCLUDED_DIRECTEDDIFFUSION_FIB_REPOSITY_h_