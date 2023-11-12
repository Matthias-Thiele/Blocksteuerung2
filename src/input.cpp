#include "input.hpp"

/**
 * @brief Erzeugt einen entprellten Eingang
 * 
 * Über den Parameter triggerCount kann man steuern
 * wie viele Perioden bei einem Wechselstromeingang
 * durchlaufen werden müssen bevor der Trigger
 * aktiv wird. Bei einem Gleichstromeingang muss
 * hier der Wert 0 gesetzt werden wenn der Trigger
 * beim Betätigen ausgelöst werden soll und auf 1,
 * wenn der Trigger beim Loslassen ausgelöst werden
 * soll. Bei einem AC Eingang muss die doppelte
 * Anzahl von Perioden angegeben werden.
 * 
 * @param mask Bitmaske für den Eingabeport
 * @param triggerCount Triggerpunkt
 */
Input::Input(uint8_t mask, int triggerCount) {
  m_mask = mask;
  m_triggerCount = triggerCount;
}

/**
 * @brief Meldet zurück ob der Eingang betätigt wurde.
 * 
 * Wenn der Eingang die konfigurierte Anzahl von
 * Perioden durchlaufen hat (bei Wechselstrom) oder
 * zwei Abtastungen hintereinander aktiv war (bei 
 * Gleichstrom), wird der Trigger ausgelöst.
 * 
 * Das Ende einer Betätigung wird daran gemessen,
 * dass es innerhalb von c_idleDuration (2 Sek.)
 * keine weitere Veränderung gegeben hat.
 * 
 * Eine erneute Betätigung kann es also erst
 * nach dieser Wartezeit geben.
 * 
 * Eine längere Wechselstrombetätigung löst nicht
 * mehrere Trigger aus.
 * 
 * @param time 
 * @param actVal 
 * @return true 
 * @return false 
 */
bool Input::checkTrigger(long time, uint8_t actVal) {
  bool newState = actVal & m_mask;
  bool changed = false;

  if ((m_unlockAt > 0) && (m_unlockAt < time)) {
    // verzögerte Freigabe da das Relais noch abfallen muss.
    Serial.println("Unlocked");
    m_isLocked = false;
    m_unlockAt = 0;
    m_lastState = newState;
    return false;
  }

  if (m_isLocked) {
    return false;
  }

  if (!newState & m_lastState) {
    changed = true;
  }

  m_lastState = newState;

  if (changed) {
    // wechsel von true -> false
    m_lastChange = time;
    m_changeCounter++;
    Serial.print("Input changed "); Serial.print(m_triggerCount); Serial.print(" - "); Serial.print(m_changeCounter); Serial.print(" - "); Serial.println(newState);
    if (m_changeCounter == 1) {
      // Änderung ist aktiv
      Serial.println("Changing.");
      m_isChanging = true;
    }


    if ((m_triggerCount == 0) && (m_changeCounter == 1)) {
      // Einfachbetätigung beim Drücken
      Serial.println("Button pressed.");
      m_isChanging = false;
      return true;
    } else if ((m_triggerCount == 1) && (m_changeCounter == 2) ) {
      // Einfachbestätigung beim Loslassen
      Serial.println("Button released.");
      m_isChanging = false;
      return true;
    } else if ((m_triggerCount > 1) && (m_changeCounter == m_triggerCount)) {
      // Triggerpunkt erreicht, Aktivierung zurückmelden
      m_isChanging = false;
      return true;
    }
  } else {
    if ((time - m_lastChange) > c_idleDuration) {
      // längere Zeit keine Änderung, warte auf neues Ereignis
      if (m_isChanging) { Serial.println("Released."); }
      m_changeCounter = 0;
      m_isChanging = false;
    }
  }

  return false;
}

/**
 * @brief Meldet zurück, ob gerade ein Rückblockvorgang aktiv ist.
 * 
 * Dieser Aufruf wird aktiv nachdem mindestens ein Statuswechsel
 * stattgefunden hat und wird wieder inaktiv wenn das Trigger
 * Signal gesendet wurde oder längere Zeit keine weiteren
 * Änderungen stattgefunden haben.
 * 
 * @return true 
 * @return false 
 */
bool Input::isChanging() {
  return m_isChanging;
}

/**
 * @brief Sperrt den Eingabeport
 * 
 * Während des Rückblockens darf der AC Input nicht auf
 * die eigene Triggerspannung reagieren. Solange der
 * Induktor läuft, wird der Input abgeschaltet.
 */
void Input::lock() {
  m_isLocked = true;
}

/**
 * @brief Beendet die Eingabesperre
 * 
 * Da das Relais etwas Zeit zum Abfallen benötigt, wird
 * die Sperre verzögert freigegeben.
 * 
 */
void Input::unlock() {
  m_unlockAt = millis() + 1000;
}