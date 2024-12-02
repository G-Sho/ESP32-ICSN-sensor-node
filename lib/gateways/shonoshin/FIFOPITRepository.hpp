#ifndef INCLUDED_FIFO_PIT_REPOSITORY_hpp_
#define INCLUDED_FIFO_PIT_REPOSITORY_hpp_

#include "interface/PITRepository.hpp"
#include <map>
#include <queue>

#define MAX_PIT_TABLE_SIZE 20

class FIFOPITRepository : public PITRepository
{
private:
    std::map<std::string, std::set<std::string>> m_pit;
    std::queue<std::string> m_pitManagement;

public:
    void save(PITPair pitPair) override
    {
        auto it = m_pit.find(pitPair.getContentName().getValue());
        if (it != m_pit.end())
            return;

        // PITのサイズが100を超えないように消す
        while (MAX_PIT_TABLE_SIZE <= m_pitManagement.size())
        {
            auto it = m_pit.find(m_pitManagement.front());
            if (it != m_pit.end()) // if this node knows the forwarding address
                m_pit.erase(m_pitManagement.front());
            m_pitManagement.pop();
        }

        m_pit[pitPair.getContentName().getValue()] = pitPair.getDestinationId().getValue();
        m_pitManagement.push(pitPair.getContentName().getValue());
    };

    void remove(ContentName contentName) override
    {
        auto it = m_pit.find(contentName.getValue());
        if (it != m_pit.end())
            m_pit.erase(contentName.getValue());
    };
    
    bool find(ContentName contentName) override
    {
        auto it = m_pit.find(contentName.getValue());
        return (it != m_pit.end());
    };

    DestinationId get(ContentName contentName) override
    {
        auto it = m_pit.find(contentName.getValue());
        if (it != m_pit.end())
            return DestinationId(m_pit[contentName.getValue()]);
        else
            return DestinationId({"NULL"});
    };
};

#endif // INCLUDED_FIFO_PIT_REPOSITY_hpp_