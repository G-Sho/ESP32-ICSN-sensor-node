#pragma once

class Time
{
private:
  uint32_t time;

public:
  Time(uint32_t t) : time(t) {}
  uint32_t getValue() const { return time; };
};