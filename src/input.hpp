#ifndef INPUT_HPP
#define INPUT_HPP
#include "Arduino.h"

class Input {
  public:
    Input(uint8_t mask, int triggerCount);
    bool checkTrigger(long time, uint8_t actVal);
    bool isChanging();
    void lock();
    void unlock();

  private:
    long c_idleDuration = 2000;
    uint8_t m_mask;

    int m_changeCounter = 0;
    long m_lastChange = 0;
    long m_unlockAt = 0;
    int m_triggerCount = 0;
    bool m_lastState = true;
    bool m_isChanging = false;
    bool m_isLocked = false;
};

#endif