#pragma once

class HopCount
{
private:
  int cnt;

public:
  HopCount(int cnt) : cnt(cnt) {}
  int getValue() const { return cnt; };
  void increment() { cnt++; };
};