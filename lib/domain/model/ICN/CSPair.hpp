#ifndef INCLUDED_CS_hpp_
#define INCLUDED_CS_hpp_

#include "model/message/ContentName.hpp"
#include "model/message/Content.hpp"
#include "model/message/Time.hpp"

class CSPair
{
private:
  ContentName contentName;
  Content content;

public:
  CSPair(const ContentName &contentName, const Content &content)
      : contentName(contentName), content(content) {}

  ContentName getContentName() const { return contentName; };

  Content getContent() const { return content; };
};

#endif // INCLUDED_CS_hpp_