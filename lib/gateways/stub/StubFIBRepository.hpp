#ifndef INCLUDED_STUB_FIB_REPOSITY_hpp_
#define INCLUDED_STUB_FIB_REPOSITY_hpp_

#include "interface/FIBRepository.hpp"
#include <map>
#include <queue>

class StubFIBRepository : public FIBRepository
{
private:
    std::map<std::string, std::vector<std::string>> m_fib;
    std::queue<std::string> m_fibManagement;
    int const m_maxSize = 100;

public:
    void save(FIBPair fibPair) override
    {
        auto it = m_fib.find(fibPair.getContentName().getValue());
        if (it != m_fib.end())
            return;

        // FIBのサイズが100を超えないように消す
        while (m_maxSize <= m_fibManagement.size())
        {
            auto it = m_fib.find(m_fibManagement.front());
            if (it != m_fib.end())
                m_fib.erase(m_fibManagement.front());
            m_fibManagement.pop();
        }
        
        m_fib[fibPair.getContentName().getValue()] = fibPair.getDestinationId().getValue();
        m_fibManagement.push(fibPair.getContentName().getValue());
    };

    void remove(ContentName contentName) override
    {
        auto it = m_fib.find(contentName.getValue());
        if (it != m_fib.end())
            m_fib.erase(contentName.getValue());
    };

    bool find(ContentName contentName) override
    {
        auto it = m_fib.find(contentName.getValue());
        return (it != m_fib.end());
    };

    DestinationId get(ContentName contentName) override
    {
        auto it = m_fib.find(contentName.getValue());
        if (it == m_fib.end())
            return DestinationId({"NULL"});
        else
            return DestinationId(m_fib[contentName.getValue()]);
    };
};

#endif // INCLUDED_STUB_FIB_REPOSITY_h_