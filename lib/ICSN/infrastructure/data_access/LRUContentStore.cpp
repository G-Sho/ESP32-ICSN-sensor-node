#include "LRUContentStore.hpp"
#include "../../config/Config.hpp"

void LRUContentStore::save(const CSPair &csPair) {
    const std::string &name = csPair.getContentName().getValue();
    const std::string &content = csPair.getContent().getValue();
    
    cache.put(name, content);
}

void LRUContentStore::remove(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    cache.remove(name);
}

bool LRUContentStore::find(const ContentName &contentName) {
    return cache.contains(contentName.getValue());
}

Content LRUContentStore::get(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    std::string content;
    
    if (cache.get(name, content)) {
        return Content(content);
    }
    
    return Content::Null();
}
