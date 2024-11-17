#ifndef INCLUDED_CS_hpp_
#define INCLUDED_CS_hpp_

#include "model\message\ContentName.hpp"
#include "model\message\Content.hpp"

class CSPair
{
private:
  ContentName contentName;
  Content content;

public:
  CSPair(const ContentName& contentName, const Content& content)
  : contentName(contentName), content(content) {}

  ContentName getContentName() { return contentName; };

  Content getContent() { return content; };
};

#endif //INCLUDED_CS_hpp_