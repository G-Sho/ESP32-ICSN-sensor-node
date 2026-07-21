#pragma once

#include <cstring>
#include <set>
#include <string>

template <size_t MaxItems, size_t MaxChars> class FixedNodeIdSet {
private:
  char ids[MaxItems][MaxChars + 1];
  size_t count;

  int findIndex(const std::string& id) const {
    const std::string normalized = normalize(id);
    for (size_t i = 0; i < count; ++i) {
      if (normalized == ids[i]) {
        return static_cast<int>(i);
      }
    }
    return -1;
  }

  static std::string normalize(const std::string& id) {
    if (id.size() <= MaxChars) {
      return id;
    }
    return id.substr(0, MaxChars);
  }

public:
  FixedNodeIdSet() : count(0) {
    clear();
  }

  explicit FixedNodeIdSet(const std::set<std::string>& src) : count(0) {
    clear();
    insertFromStdSet(src);
  }

  void clear() {
    for (size_t i = 0; i < MaxItems; ++i) {
      ids[i][0] = '\0';
    }
    count = 0;
  }

  size_t size() const {
    return count;
  }

  bool insert(const std::string& id) {
    const std::string normalized = normalize(id);
    if (normalized.empty()) {
      return false;
    }

    if (findIndex(normalized) != -1) {
      return true;
    }

    if (count >= MaxItems) {
      return false;
    }

    strncpy(ids[count], normalized.c_str(), MaxChars);
    ids[count][MaxChars] = '\0';
    ++count;
    return true;
  }

  size_t insertFromStdSet(const std::set<std::string>& src) {
    size_t dropped = 0;
    for (const auto& id : src) {
      if (!insert(id)) {
        ++dropped;
      }
    }
    return dropped;
  }

  size_t insertFromOther(const FixedNodeIdSet& other) {
    size_t dropped = 0;
    for (size_t i = 0; i < other.count; ++i) {
      if (!insert(other.ids[i])) {
        ++dropped;
      }
    }
    return dropped;
  }

  std::set<std::string> toStdSet() const {
    std::set<std::string> out;
    for (size_t i = 0; i < count; ++i) {
      out.insert(std::string(ids[i]));
    }
    return out;
  }
};
