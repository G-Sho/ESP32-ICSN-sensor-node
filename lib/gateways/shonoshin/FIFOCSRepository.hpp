#ifndef INCLUDED_FIFO_CS_REPOSITORY_hpp_
#define INCLUDED_FIFO_CS_REPOSITORY_hpp_

#include "interface/CSRepository.hpp"
#include <map>
#include <queue>

#define MAX_CS_TABLE_SIZE 20

class FIFOCSRepository : public CSRepository
{
private:
    std::map<std::string, std::string> m_cs;
    std::queue<std::string> m_csManagement;

public:
    void save(CSPair csPair) override
    {
        auto it = m_cs.find(csPair.getContentName().getValue());
        if (it != m_cs.end())
            return;

        // CSのサイズがmaxSizeを超えないように消す
        while (MAX_CS_TABLE_SIZE <= m_csManagement.size())
        {
            auto it = m_cs.find(m_csManagement.front());
            if (it != m_cs.end())
                m_cs.erase(m_csManagement.front());
            m_csManagement.pop();
        }

        m_cs[csPair.getContentName().getValue()] = csPair.getContent().getValue();
        m_csManagement.push(csPair.getContentName().getValue());
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

#endif // INCLUDED_FIFO_CS_REPOSITORY_hpp_