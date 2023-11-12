#ifndef BLOCK_HPP
#define BLOCK_HPP
#include "Arduino.h"
#include "io.hpp"
#include "input.hpp"

class Block {
  public:
    Block(Io *io, uint8_t blockNo, bool isInbound);
    void tick(long time);
    void startTicker();
    void activateBlock();
    void doBlock();
    void setTickerBlock(Block *tickerBlock);

  private:
    const uint8_t c_tickerMask = 0x10;
    const uint8_t c_activateMask = 0x10;
    const uint8_t c_blockMask = 0x8;
    const uint8_t c_LEDred = 0x20;
    const uint8_t c_LEDwhite = 0x40;
    const uint16_t c_portStateOffset = 0x10;

    const long    c_tickerDurationMillis = 15000;

    Io *m_io;
    Block *m_tickerBlock;
    uint8_t m_blockNo;
    bool m_isInbound;
  
    uint8_t m_pioValue = 0;
    bool m_isRedState = false;
    bool m_tickerDoStart = false;
    long m_tickerStarted = 0;
    long m_tickerUntil = 0;

    bool m_blockDoStart = false;
    long m_blockUntil = 0;

    long m_nextInput = 0;
    Input *m_acIn;
    Input *m_dcIn;

    void initState();
    void saveState();
    void checkBlockState(long time);
    void checkTickerStarted(long time);
    void advanceTicker(long time);
    void checkInput(long time);
    void updateStateLed();
    void updateLocks();
};

#endif