#ifndef INCLUDED_STUB_PIT_REPOSITORY_hpp_
#define INCLUDED_STUB_PIT_REPOSITORY_hpp_

#include "interface/PITRepository.hpp"
#include <map>
#include <queue>

class StubPITRepository : public PITRepository
{
private:
    std::map<String, String> m_pit;
    std::queue<String> m_pitManagement;
    int const m_maxSize = 100;

public:
    void save(PIT pit) override
    {
        auto it = m_pit.find(pit.getContentName().getValue());
        if (it != m_pit.end())
            return;

        // PITのサイズが100を超えないように消す
        while (m_maxSize <= m_pitManagement.size())
        {
            auto it = m_pit.find(m_pitManagement.front());
            if (it != m_pit.end()) // if this node knows the forwarding address
                m_pit.erase(m_pitManagement.front());
            m_pitManagement.pop();
        }

        m_pit[pit.getContentName().getValue()] = pit.getNodeId().getValue();
        m_pitManagement.push(pit.getContentName().getValue());
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

    NodeId get(ContentName contentName) override
    {
        auto it = m_pit.find(contentName.getValue());
        if (it != m_pit.end())
            return NodeId(m_pit[contentName.getValue()]);
        else
            return NodeId("NULL");
    };
};

#endif // INCLUDED_STUB_PIT_REPOSITY_hpp_