#include <Arduino.h>
#include <avr/io.h>

// ============================================================================
// DONANIM VE PİN TANIMLAMALARI (1. LED = Pin 13 ... 10. LED = Pin 4)
// ============================================================================
constexpr uint8_t LED_COUNT = 10;
constexpr uint8_t LED_PINS[LED_COUNT] = {13, 12, 11, 10, 9, 8, 7, 6, 5, 4};

constexpr uint8_t  PWM_MAX_STEPS = 100;
constexpr uint16_t PWM_STEP_US   = 20; // 500 Hz
constexpr uint8_t  PHASE_OFFSET[LED_COUNT] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};

// GÜVENLİK: Aynı anda çekilecek tepe akımı sınırla (~45 mA bütçe)
constexpr uint16_t MAX_CURRENT_BUDGET = 500;
constexpr uint8_t  SAFE_LED_MAX_DUTY  = 75;

// ============================================================================
// ÇALIŞMA MODLARI VE KALICI BELLEK (.noinit)
// ============================================================================
enum class LightMode : uint8_t {
    STANDBY           = 0,
    POLICE_STROBE     = 1,
    AUDI_SIGNAL       = 2,
    COMET_TRAIL       = 3,
    KNIGHT_RIDER      = 4,
    RUNNING_BOUNCE    = 5,
    BREATHING_WAVE    = 6,
    RANDOM_SPARKLE    = 7,
    BINARY_COUNTER    = 8,
    ALTERNATING_PINGPONG = 9,
    FIRE_EFFECT       = 10
};

struct PersistentState {
    uint16_t magicSignature;
    uint8_t  savedMode;
};

constexpr uint16_t VALID_SIGNATURE = 0xA55A;
__attribute__((section(".noinit"))) PersistentState persistentState;

LightMode currentMode = LightMode::POLICE_STROBE;

uint8_t targetBrightness[LED_COUNT] = {0};
uint8_t safeBrightness[LED_COUNT]   = {0};

uint32_t lastStepTimeMs = 0;
uint32_t lastFadeTimeMs = 0;
uint16_t lastPwmTickUs  = 0;
uint8_t  pwmCounter     = 0;

uint8_t  animStep      = 0;
int8_t   cometPos      = 0;
int8_t   cometDir      = 1;
uint8_t  subCycleCount = 0;
uint32_t customCounter = 0;

// ============================================================================
// GÜÇ VE AKIM YÖNETİCİSİ
// ============================================================================
inline void hardwarePinsOff() {
#if defined(__AVR_ATmega328P__)
    PORTB &= ~0x3F;
    PORTD &= ~0xF0;
#else
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        digitalWrite(LED_PINS[i], LOW);
    }
#endif
}

void enforceCurrentBudget() {
    uint16_t totalRequested = 0;
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        totalRequested += targetBrightness[i];
    }

    if (totalRequested <= MAX_CURRENT_BUDGET) {
        for (uint8_t i = 0; i < LED_COUNT; i++) {
            safeBrightness[i] = targetBrightness[i];
        }
    } else {
        for (uint8_t i = 0; i < LED_COUNT; i++) {
            uint32_t scaled = (static_cast<uint32_t>(targetBrightness[i]) * MAX_CURRENT_BUDGET) / totalRequested;
            safeBrightness[i] = static_cast<uint8_t>(scaled);
        }
    }
}

void clearAllLeds() {
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        targetBrightness[i] = 0;
        safeBrightness[i]   = 0;
    }
    hardwarePinsOff();
}

inline void setLed(uint8_t index, uint8_t brightness) {
    if (index >= LED_COUNT) return;
    uint8_t scaled = (static_cast<uint16_t>(brightness) * SAFE_LED_MAX_DUTY) / 100;
    targetBrightness[index] = (scaled > SAFE_LED_MAX_DUTY) ? SAFE_LED_MAX_DUTY : scaled;
}

// ============================================================================
// YAZILIMSAL PWM MOTORU
// ============================================================================
void updateSoftPWM() {
    uint16_t nowUs = static_cast<uint16_t>(micros());
    if (static_cast<uint16_t>(nowUs - lastPwmTickUs) < PWM_STEP_US) return;
    lastPwmTickUs = nowUs;

    pwmCounter++;
    if (pwmCounter >= PWM_MAX_STEPS) pwmCounter = 0;

#if defined(__AVR_ATmega328P__)
    uint8_t portB_bits = 0;
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t phase = pwmCounter + PHASE_OFFSET[i];
        if (phase >= PWM_MAX_STEPS) phase -= PWM_MAX_STEPS;
        if (safeBrightness[i] > phase) portB_bits |= (1 << (5 - i));
    }

    uint8_t portD_bits = 0;
    for (uint8_t i = 6; i < 10; i++) {
        uint8_t phase = pwmCounter + PHASE_OFFSET[i];
        if (phase >= PWM_MAX_STEPS) phase -= PWM_MAX_STEPS;
        if (safeBrightness[i] > phase) portD_bits |= (1 << (13 - i));
    }

    PORTB = (PORTB & ~0x3F) | portB_bits;
    PORTD = (PORTD & ~0xF0) | portD_bits;
#else
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        uint8_t phase = pwmCounter + PHASE_OFFSET[i];
        if (phase >= PWM_MAX_STEPS) phase -= PWM_MAX_STEPS;
        digitalWrite(LED_PINS[i], (safeBrightness[i] > phase) ? HIGH : LOW);
    }
#endif
}

// ============================================================================
// 10 ADET PROFESYONEL ANİMASYON FONKSİYONU
// ============================================================================

// Mod 1: Polis / Ambulans Çakar
void runMode1(uint32_t nowMs) {
    if (nowMs - lastStepTimeMs >= 55) {
        lastStepTimeMs = nowMs;
        clearAllLeds();
        if (animStep < 6) {
            if ((animStep & 1) == 0) for (uint8_t i = 0; i < 5; i++) setLed(i, 100);
        } else if (animStep < 12) {
            if ((animStep & 1) == 0) for (uint8_t i = 5; i < 10; i++) setLed(i, 100);
        } else {
            if ((animStep & 1) == 0) {
                setLed(0,100); setLed(1,100); setLed(8,100); setLed(9,100);
            } else {
                for (uint8_t i = 3; i <= 6; i++) setLed(i, 100);
            }
        }
        enforceCurrentBudget();
        animStep = (animStep + 1) % 16;
    }
}

// Mod 2: Audi / VW Dinamik Kayar Sinyal
void runMode2(uint32_t nowMs) {
    uint16_t waitTime = (animStep == LED_COUNT) ? 220 : ((animStep == LED_COUNT + 1) ? 320 : 35);
    if (nowMs - lastStepTimeMs >= waitTime) {
        lastStepTimeMs = nowMs;
        if (animStep < LED_COUNT) {
            setLed(animStep, 100);
            enforceCurrentBudget();
            animStep++;
        } else if (animStep == LED_COUNT) {
            clearAllLeds();
            animStep++;
        } else {
            setLed(0, 100);
            enforceCurrentBudget();
            animStep = 1;
        }
    }
}

// Mod 3: Kuyruklu Yıldız ve Çarpışma
void runMode3(uint32_t nowMs) {
    if (nowMs - lastFadeTimeMs >= 18) {
        lastFadeTimeMs = nowMs;
        for (uint8_t i = 0; i < LED_COUNT; i++) targetBrightness[i] = (targetBrightness[i] > 9) ? (targetBrightness[i] - 9) : 0;
        enforceCurrentBudget();
    }
    if (nowMs - lastStepTimeMs >= ((subCycleCount < 4) ? 55 : 65)) {
        lastStepTimeMs = nowMs;
        if (subCycleCount < 4) {
            setLed(cometPos, 100);
            if (cometDir > 0) {
                if (cometPos >= 9) { cometDir = -1; cometPos = 8; subCycleCount++; }
                else cometPos++;
            } else {
                if (cometPos <= 0) { cometDir = 1; cometPos = 1; subCycleCount++; }
                else cometPos--;
            }
        } else {
            if (animStep < 5) {
                setLed(animStep, 100); setLed(9 - animStep, 100); animStep++;
            } else if (animStep == 5) {
                for (uint8_t i = 0; i < 10; i++) setLed(i, 100);
                lastFadeTimeMs = nowMs; animStep++;
            } else if (animStep < 8) { animStep++; }
            else { animStep = 0; subCycleCount = 0; cometPos = 0; cometDir = 1; }
        }
        enforceCurrentBudget();
    }
}

// Mod 4: Klasik Kara Şimşek (Knight Rider Tarayıcı)
void runMode4(uint32_t nowMs) {
    if (nowMs - lastStepTimeMs >= 60) {
        lastStepTimeMs = nowMs;
        clearAllLeds();
        setLed(cometPos, 100);
        if (cometPos > 0 && cometPos < 9) {
            setLed(cometPos - 1, 50);
            setLed(cometPos + 1, 50);
        }
        cometPos += cometDir;
        if (cometPos >= 9) { cometPos = 9; cometDir = -1; }
        else if (cometPos <= 0) { cometPos = 0; cometDir = 1; }
        enforceCurrentBudget();
    }
}

// Mod 5: Karşılıklı Çarpışma ve Geri Sekme (Ping-Pong)
void runMode5(uint32_t nowMs) {
    if (nowMs - lastStepTimeMs >= 70) {
        lastStepTimeMs = nowMs;
        clearAllLeds();
        setLed(animStep, 100);
        setLed(9 - animStep, 100);
        enforceCurrentBudget();
        animStep = (animStep + 1) % 5;
    }
}

// Mod 6: Sinüzoidal Nefes Alma (Breathing Glow)
void runMode6(uint32_t nowMs) {
    if (nowMs - lastStepTimeMs >= 25) {
        lastStepTimeMs = nowMs;
        customCounter += 5;
        uint8_t breathVal = (sin((customCounter * 3.1416) / 180.0) + 1.0) * 45.0 + 10;
        for (uint8_t i = 0; i < LED_COUNT; i++) setLed(i, breathVal);
        enforceCurrentBudget();
    }
}

// Mod 7: Rastgele Yıldız Pırıltısı (Random Sparkle)
void runMode7(uint32_t nowMs) {
    if (nowMs - lastStepTimeMs >= 120) {
        lastStepTimeMs = nowMs;
        clearAllLeds();
        for (uint8_t k = 0; k < 3; k++) {
            uint8_t rndIdx = random(0, 10);
            setLed(rndIdx, 100);
        }
        enforceCurrentBudget();
    }
}

// Mod 8: İkili Sistem Sayacı (Binary Counter 0-1023)
void runMode8(uint32_t nowMs) {
    if (nowMs - lastStepTimeMs >= 350) {
        lastStepTimeMs = nowMs;
        clearAllLeds();
        for (uint8_t i = 0; i < LED_COUNT; i++) {
            if ((customCounter >> i) & 0x01) {
                setLed(i, 100);
            }
        }
        enforceCurrentBudget();
        customCounter++;
        if (customCounter > 1023) customCounter = 0;
    }
}

// Mod 9: Çift Taraflı Sıkıştırma (Accordion / Senkron Daralma)
void runMode9(uint32_t nowMs) {
    if (nowMs - lastStepTimeMs >= 100) {
        lastStepTimeMs = nowMs;
        clearAllLeds();
        if (animStep < 5) {
            setLed(animStep, 100);
            setLed(9 - animStep, 100);
        } else {
            uint8_t idx = 9 - animStep;
            setLed(idx, 100);
            setLed(9 - idx, 100);
        }
        enforceCurrentBudget();
        animStep = (animStep + 1) % 10;
    }
}

// Mod 10: Şömine Ateşi / Titrek Alev Efekti (Fire Simulation)
void runMode10(uint32_t nowMs) {
    if (nowMs - lastStepTimeMs >= 80) {
        lastStepTimeMs = nowMs;
        for (uint8_t i = 0; i < LED_COUNT; i++) {
            uint8_t flicker = random(20, 90);
            setLed(i, flicker);
        }
        enforceCurrentBudget();
    }
}

// ============================================================================
// TERMİNAL MENÜSÜ VE KONTROL
// ============================================================================
const __FlashStringHelper* getModeTitle(LightMode mode) {
    switch (mode) {
        case LightMode::POLICE_STROBE:        return F("1 - Polis / Ambulans Cakar");
        case LightMode::AUDI_SIGNAL:          return F("2 - Audi / VW Kayar Sinyal");
        case LightMode::COMET_TRAIL:          return F("3 - Kuyruklu Yildiz & Carpisma");
        case LightMode::KNIGHT_RIDER:         return F("4 - Kara Simsek (Knight Rider)");
        case LightMode::RUNNING_BOUNCE:       return F("5 - Karsilikli Sekme (Ping-Pong)");
        case LightMode::BREATHING_WAVE:       return F("6 - Sinuzoidal Nefes Alma");
        case LightMode::RANDOM_SPARKLE:       return F("7 - Rastgele Yildiz Piriltisi");
        case LightMode::BINARY_COUNTER:       return F("8 - Ikili Sayac (Binary Counter)");
        case LightMode::ALTERNATING_PINGPONG: return F("9 - Cift Taraflı Sikistirma");
        case LightMode::FIRE_EFFECT:          return F("10 - Somine Atesi Alev Efekti");
        case LightMode::STANDBY:
        default:                              return F("0 - Guvenli Bekleme (Kapali)");
    }
}

void printMenu() {
    hardwarePinsOff();
    Serial.println(F("\n=================================================="));
    Serial.println(F("     ARDUINO UNO - 10 MODLU PROFESYONEL KONTROL"));
    Serial.println(F("=================================================="));
    Serial.println(F(" [1] Polis / Ambulans Cakar"));
    Serial.println(F(" [2] Audi / VW Kayar Sinyal"));
    Serial.println(F(" [3] Kuyruklu Yildiz & Carpisma"));
    Serial.println(F(" [4] Kara Simsek (Knight Rider)"));
    Serial.println(F(" [5] Karsilikli Sekme (Ping-Pong)"));
    Serial.println(F(" [6] Sinuzoidal Nefes Alma"));
    Serial.println(F(" [7] Rastgele Yildiz Piriltisi"));
    Serial.println(F(" [8] Ikili Sayac (Binary Counter)"));
    Serial.println(F(" [9] Cift Taraflı Sikistirma"));
    Serial.println(F("[10] Somine Atesi Alev Efekti"));
    Serial.println(F(" [0] Guvenli Bekleme (Tum LED'ler Kapali)"));
    Serial.println(F(" [m] Menuyu Tekrar Yazdir"));
    Serial.println(F("--------------------------------------------------"));
    Serial.flush();

    lastPwmTickUs  = static_cast<uint16_t>(micros());
    lastStepTimeMs = millis();
    lastFadeTimeMs = millis();
}

void switchMode(LightMode newMode) {
    currentMode = newMode;
    persistentState.magicSignature = VALID_SIGNATURE;
    persistentState.savedMode      = static_cast<uint8_t>(newMode);

    animStep      = 0;
    cometPos      = 0;
    cometDir      = 1;
    subCycleCount = 0;
    customCounter = 0;
    clearAllLeds();

    Serial.print(F(">> AKTIF MOD: "));
    Serial.println(getModeTitle(newMode));

    uint32_t nowMs = millis();
    lastStepTimeMs = nowMs;
    lastFadeTimeMs = nowMs;
    lastPwmTickUs  = static_cast<uint16_t>(micros());

    if (newMode == LightMode::AUDI_SIGNAL) {
        setLed(0, 100);
        enforceCurrentBudget();
        animStep = 1;
    }
}

void handleSerialInput() {
    static int inputNumber = -1;
    static bool hasNumber = false;

    while (Serial.available() > 0) {
        char cmd = Serial.read();
        if (cmd == '\n' || cmd == '\r') {
            if (hasNumber) {
                if (inputNumber >= 0 && inputNumber <= 10) {
                    switchMode(static_cast<LightMode>(inputNumber));
                } else {
                    Serial.println(F("(!) Gecersiz mod numarasi (0-10 arasi secin)."));
                }
                inputNumber = -1;
                hasNumber = false;
            }
            continue;
        }
        if (cmd == ' ') continue;

        if (cmd == 'm' || cmd == 'M') {
            printMenu();
            inputNumber = -1;
            hasNumber = false;
            continue;
        }

        if (cmd >= '0' && cmd <= '9') {
            int digit = cmd - '0';
            if (!hasNumber) {
                inputNumber = digit;
                hasNumber = true;
            } else {
                inputNumber = (inputNumber * 10) + digit;
            }
        }
    }
}

// ============================================================================
// SETUP & LOOP
// ============================================================================
void setup() {
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        digitalWrite(LED_PINS[i], LOW);
        pinMode(LED_PINS[i], OUTPUT);
    }

    Serial.begin(9600);
    printMenu();

    if (persistentState.magicSignature == VALID_SIGNATURE && persistentState.savedMode <= 10) {
        switchMode(static_cast<LightMode>(persistentState.savedMode));
    } else {
        switchMode(LightMode::POLICE_STROBE);
    }
}

void loop() {
    handleSerialInput();

    uint32_t nowMs = millis();

    switch (currentMode) {
        case LightMode::POLICE_STROBE:        runMode1(nowMs); break;
        case LightMode::AUDI_SIGNAL:          runMode2(nowMs); break;
        case LightMode::COMET_TRAIL:          runMode3(nowMs); break;
        case LightMode::KNIGHT_RIDER:         runMode4(nowMs); break;
        case LightMode::RUNNING_BOUNCE:       runMode5(nowMs); break;
        case LightMode::BREATHING_WAVE:       runMode6(nowMs); break;
        case LightMode::RANDOM_SPARKLE:       runMode7(nowMs); break;
        case LightMode::BINARY_COUNTER:       runMode8(nowMs); break;
        case LightMode::ALTERNATING_PINGPONG: runMode9(nowMs); break;
        case LightMode::FIRE_EFFECT:          runMode10(nowMs); break;
        case LightMode::STANDBY:
        default:                              break;
    }

    if (currentMode != LightMode::STANDBY) {
        updateSoftPWM();
    }
}