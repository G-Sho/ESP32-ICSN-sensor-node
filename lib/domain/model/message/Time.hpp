#ifndef INCLUDED_TIME_hpp_
#define INCLUDED_TIME_hpp_

class Time
{
private:
  uint32_t value;

public:
  Time(uint32_t value)
  {
    // Write the rules

    this->value = value;
  };

  static Time Null()
  {
    return Time({});
  }

  uint32_t getValue() const
  {
    return value;
  };
};

#endif // INCLUDED_TIME_hpp_