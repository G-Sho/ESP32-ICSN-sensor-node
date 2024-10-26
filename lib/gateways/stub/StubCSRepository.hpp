#ifndef INCLUDED_STUB_CS_REPOSITY_hpp_
#define INCLUDED_STUB_CS_REPOSITY_hpp_

#include "model\ICN\CSRepository.hpp"
#include <map>
#include <queue>

class StubCSRepository : public CSRepository
{
private:
    std::map<String, String> m_cs;
    std::queue<String> m_csManagement;
    int const m_maxSize = 100;

public:
    void save(CS cs) override
    {
        auto it = m_cs.find(cs.getContentName().getValue());
        if (it != m_cs.end())
            return;

        // CSのサイズが100を超えないように消す
        while (m_maxSize <= m_csManagement.size())
        {
            auto it = m_cs.find(m_csManagement.front());
            if (it != m_cs.end())
                m_cs.erase(m_csManagement.front());
            m_csManagement.pop();
        }

        m_cs[cs.getContentName().getValue()] = cs.getContent().getValue();
        m_csManagement.push(cs.getContentName().getValue());
    };

    void remove(ContentName contentName) override
    {
        auto it = m_cs.find(contentName.getValue());
        if (it != m_cs.end())
            m_cs.erase(contentName.getValue());
    };

    bool find(ContentName contentName) override
    {
        auto it = m_cs.find(contentName.getValue());
        return (it != m_cs.end());
    };

    Content get(ContentName contentName) override
    {
        auto it = m_cs.find(contentName.getValue());
        if (it == m_cs.end())
            return Content("NULL");
        else
            return Content(m_cs[contentName.getValue()]);
    };
};

#endif // INCLUDED_STUB_CS_REPOSITY_hpp_