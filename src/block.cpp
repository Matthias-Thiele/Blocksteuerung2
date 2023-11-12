#include "block.hpp"


/**
 * @brief Erzeugt ein neues Block Objekt.
 * 
 * Jeder Block hat bis zu 3 Relais, 4 LEDs und
 * drei potentialfreie Eingänge. Wie diese
 * verwendet werden, hängt von der Verkabelung
 * der Busplatine ab. Nicht jede Blockplatine
 * muss voll bestückt sein.
 * 
 * @param io Hardware Ansteuerung, normalerweise I2C
 * @param blockNo Blocknummer (0..7)
 * @param isInbound Block zum Einfahrts- oder Ausfahrtsgleis
 */
Block::Block(Io *io, uint8_t blockNo, bool isInbound) {
  m_io = io;
  m_blockNo = blockNo;
  m_isInbound = isInbound;
  m_pioValue = 0xff;
  Serial.print("Init "); Serial.println(m_pioValue, HEX);
  m_isRedState = false;
  m_acIn = new Input(0x1, 25);
  m_dcIn = new Input(0x2, 0);
  m_tickerBlock = this;
  initState();
  updateStateLed();
}

/**
 * @brief Initialisiert nach dem Start den letzten Zustand
 * 
 * @param isRed 
 */
void Block::initState() {
  uint8_t lastState = m_io->eeRead(c_portStateOffset + m_blockNo);
  m_isRedState = lastState & 1;
  Serial.print("Init state: "); Serial.println(lastState, HEX);
}

/**
 * @brief Schreibt nach jeder Änderung den aktuellen Zustand
 * in das EEPROM damit dieser nach einem Start wiederhergestellt
 * werden kann.
 * 
 */
void Block::saveState() {
  uint8_t newState = m_isRedState & 1;

  uint8_t lastState = m_io->eeRead(c_portStateOffset + m_blockNo);
  if (newState != lastState) {
    Serial.print("Write new state: "); Serial.println(newState, HEX);
    m_io->eeWrite(c_portStateOffset + m_blockNo, newState);
  }
}

/**
 * @brief Gibt die Blocksteuerung frei.
 * 
 * Wenn der Microcontroller stromlos ist, soll ein
 * Blocken verhindert werden. Das passiert indem
 * die Wechselstrom Blockstrecke unterbrochen wird.
 * 
 * Beim Start wird das Relais aktiviert welches
 * die Blockstrecke unterbricht und schließt somit
 * den Wechselstromkreis.
 * 
 */
void Block::activateBlock() {
  Serial.print("Activate ");
  m_pioValue &= ~c_activateMask;
  m_io->pioWrite(m_blockNo, m_pioValue);
  Serial.print("Activate "); Serial.println(m_pioValue, HEX);
}

/**
 * @brief Startet den Block/ Rückblockvorgang.
 * 
 * Fügt den vorher gestarteten Kurbelinduktor
 * in die Wechselstromschleife dieses Blocks
 * ein.
 * 
 * Der Blockvorgang wird nach der konfigurierten
 * Zeit automatisch beendet, es gibt keine 
 * Stop Funktion.
 * 
 * Ein versuchter Blockvorgang bei falschen
 * Blockzustand wird wie im mechanischen
 * Stellwerk einfach ignoriert.
 */
void Block::doBlock() {
  m_isRedState = !m_isInbound; // DC nach Weiß bei Einfahrt und nach Rot bei Ausfahrt
  m_acIn->lock();
  m_tickerBlock->startTicker();
  m_blockDoStart = true;
}

/**
 * @brief Meldet zurück in welchem Zustand sich der Block befindet
 * 
 * Rot: true
 * Weiss: false
 * 
 * @return boolean 
 */
boolean Block::getState() {
  return m_isRedState;
}

/**
 * @brief Startet den Kurbelinduktor.
 * 
 * Der Ticker simuliert den Wechselstromgenerator
 * welcher zum Blocken oder Rückblocken verwendet
 * wird. Es gibt im System typischerweise nur einen -
 * er wird von allen Blöcken gemeinsam verwendet.
 * 
 * Nach dem Start läuft er die voreingestellte
 * Zeit und beendet sich dann selber. Ein Stop
 * Kommando ist nicht notwendig und existiert
 * deshalb nicht.
 * 
 * Wenn mehrere Blöcke nacheinander und überlappend
 * gestartet werden, läuft der Generator so lange
 * bis der letzte Blockvorgang abgeschlossen ist.
 * 
 * Diese Funktion muss normalerweise nicht extra
 * aufgerufen werden, da sie von der doBlock()
 * Funktion zum Blocken/ Rückblocken automatisch
 * mit aufgerufen wird.
 */
void Block::startTicker() {
  m_tickerDoStart = true;
}

/**
 * @brief Automatische Zeitsteuerung.
 * 
 * Da einige Vorgänge automatisch zeitgesteuert
 * ablaufen, muss die Block funktion zyklisch
 * aus der loop Schleife heraus aufgerufen werden.
 * 
 * @param time aktueller Zeitstempel
 */
void Block::tick(long time) {
  checkInput(time);
  checkBlockState(time);
  checkTickerStarted(time);
  advanceTicker(time);

  m_io->pioWrite(m_blockNo, m_pioValue);
}

/**
 * @brief Meldet an, welche Blockkarte den Ticker enthält.
 * 
 * @param tickerBlock 
 */
void Block::setTickerBlock(Block *tickerBlock) {
  m_tickerBlock = tickerBlock;
}

void Block::updateLocks() {
  // Block abgeben nur bei Weiß, annehmen nur bei Rot
  // wie im mechanischen Stellwerk.
  if (m_isRedState ^ m_isInbound) {
    m_acIn->unlock();
    m_dcIn->lock();
  } else {
    m_acIn->lock();
    m_dcIn->unlock();
  }
}

/**
 * @brief Prüft alle Eingänge und führt bei Betätigung die
 * Blockaktion aus.
 * 
 * @param time 
 */
void Block::checkInput(long time) {
  if (time > m_nextInput) {
    if (m_acIn->isChanging()) {
      // während des Blockvorgangs leuchten beide LEDs.
      m_pioValue &= ~(c_LEDred | c_LEDwhite);
      m_io->pioWrite(m_blockNo, m_pioValue);
    }

    uint8_t actVal = m_io->pioRead(m_blockNo);
    bool triggerBlockLocal = m_acIn->checkTrigger(time, actVal);
    if (triggerBlockLocal) {
      Serial.println("AC Block triggered");
      m_isRedState = m_isInbound; // AC nach Rot bei Einfahrt und nach Weiß bei Ausfahrt
      this->updateStateLed();
      updateLocks();
      saveState();
    }

    bool triggerBlockRemote = m_dcIn->checkTrigger(time, actVal);
    if (triggerBlockRemote) {
      Serial.println("Button triggered");
      this->doBlock();
    }

    m_nextInput = time + 5;
  }
}

/**
 * @brief Start des Wechselstromgenerators.
 * 
 * Wenn der Generator gestartet wird, setzt der
 * Aufruf nur ein Flag. Dieses wird hier beim
 * nächsten Tick geprüft und die Start- und
 * Endezeit eingestellt.
 * 
 * @param time 
 */
void Block::checkTickerStarted(long time) {
  if (m_tickerDoStart) {
    if (m_tickerStarted == 0) {
      m_tickerStarted = time;
    }

    m_tickerUntil = time + c_tickerDurationMillis;
    m_tickerDoStart = false;
  }

}

/**
 * @brief Start eines Block/ Rückblockvorgangs.
 * 
 * Wenn ein Blockvorgang gestartet wird, setzt
 * der Aufruf nur ein Flag. Dieses wird beim
 * nächsten Tick geprüft und dann das Relais
 * geschaltet welches den Wechselstromgenerator
 * in den Stromkreis einfügt.
 * 
 * Wenn die Endezeit erreicht ist, wird des
 * Relais zurückgestellt.
 * 
 * @param time 
 */
void Block::checkBlockState(long time) {
  if (m_blockDoStart) {
    Serial.println("Start ticker.");
    // neuen Blockvorgang starten
    m_pioValue &= ~c_blockMask;

    // während des Blockvorgangs leuchten beide LEDs.
    m_pioValue &= ~(c_LEDred | c_LEDwhite);
    m_io->pioWrite(m_blockNo, m_pioValue);

    m_blockUntil = time + c_tickerDurationMillis;
    m_blockDoStart = false;
  }

  if ((m_blockUntil > 0) && (m_blockUntil < time)) {
    // Ende der Blockzeit erreicht
    Serial.println("Stop ticker.");
    m_isRedState = !m_isInbound;
    updateStateLed();

    m_pioValue |= c_blockMask;
    m_io->pioWrite(m_blockNo, m_pioValue);
    Serial.print("Block end "); Serial.println(m_pioValue, HEX);

    updateLocks();
    saveState();
    m_blockUntil = 0;
  }
}

/**
 * @brief Aktualisiert die LED Anzeige zum Blockzustand.
 * 
 */
void Block::updateStateLed() {
  if (m_isRedState) {
    m_pioValue |= c_LEDwhite; 
    m_pioValue &= ~c_LEDred;
  } else {
    m_pioValue |= c_LEDred;
    m_pioValue &= ~c_LEDwhite;
  }
  Serial.print("LED "); Serial.println(m_pioValue, HEX);
}

/**
 * @brief Wechselspannung erzeugen.
 * 
 * Wenn das Ende der Blockzeit erreicht ist, wird
 * die Wechselspannungserzeugung abgeschaltet.
 * 
 * Andernfalls wird in Abhängigkeit der vergangenen
 * Zeit die Polarität umgeschaltet.
 * 
 * @param time 
 */
void Block::advanceTicker(long time) {
  if ((m_tickerUntil != 0) && (time > m_tickerUntil)) {
    m_tickerStarted = 0;
    m_tickerUntil = 0;
    m_pioValue |= c_tickerMask;
    Serial.print("Adv ticker end "); Serial.println(m_pioValue, HEX);
  } else if (m_tickerStarted > 0) {
    long delta = time - m_tickerStarted;
    if (delta & 0x100) {
      m_pioValue &= ~c_tickerMask;
    } else {
      m_pioValue |= c_tickerMask;
    }
  }
}