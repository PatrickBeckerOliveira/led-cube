#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <atomic>

// Circuit pins for the ESP32-C3 and two TPIC6B595 shift registers.
constexpr uint8_t PIN_DATA = 4;
constexpr uint8_t PIN_CLOCK = 5;
constexpr uint8_t PIN_LATCH = 6;
constexpr uint8_t PIN_LAYER[4] = {7, 8, 9, 10};
constexpr uint32_t LAYER_TIME_US = 2000;
constexpr uint32_t BLANK_TIME_US = 30;

#define SERVICE_UUID "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define COMMAND_UUID "6e400002-b5a3-f393-e0a9-e50e24dcca9e"

enum Command { CMD_NONE, CMD_ON, CMD_OFF };
std::atomic<int> pendingCommand{CMD_NONE};
std::atomic<bool> restartAdvertising{false};
bool cubeEnabled = false;

// One element per layer; each bit represents a column.
uint16_t frame[4] = {0, 0, 0, 0};
uint8_t scanLayer = 0;
uint32_t lastScanUs = 0;
uint8_t effect = 0;
uint32_t effectStartMs = 0;
uint32_t lastAnimationStep = UINT32_MAX;
const uint32_t EFFECT_DURATION_MS[] = {4200, 4800, 4000, 1800, 1500};
constexpr uint8_t EFFECT_COUNT =
  sizeof(EFFECT_DURATION_MS) / sizeof(EFFECT_DURATION_MS[0]);

void disableLayers() {
  for (uint8_t pin : PIN_LAYER) digitalWrite(pin, LOW);
}

void setColumns(uint16_t columns) {
  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, highByte(columns));
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, lowByte(columns));
  digitalWrite(PIN_LATCH, HIGH);
}

void clearFrame() {
  for (uint8_t layer = 0; layer < 4; layer++) frame[layer] = 0;
}

void fillFrame() {
  for (uint8_t layer = 0; layer < 4; layer++) frame[layer] = 0xFFFF;
}

void turnCubeOff() {
  disableLayers();
  setColumns(0);
  clearFrame();
}

void refreshCube() {
  const uint32_t nowUs = micros();
  if (nowUs - lastScanUs < LAYER_TIME_US) return;
  lastScanUs = nowUs;
  // Blank the outputs before switching layers to prevent overlap.
  disableLayers();
  setColumns(0);
  delayMicroseconds(BLANK_TIME_US);
  const uint16_t columns = frame[scanLayer];
  setColumns(columns);
  if (columns != 0) digitalWrite(PIN_LAYER[scanLayer], HIGH);
  scanLayer = (scanLayer + 1) % 4;
}

void updateAnimation(uint32_t nowMs) {
  uint32_t elapsed = nowMs - effectStartMs;
  if (elapsed >= EFFECT_DURATION_MS[effect]) {
    effect = (effect + 1) % EFFECT_COUNT;
    effectStartMs = nowMs;
    lastAnimationStep = UINT32_MAX;
    elapsed = 0;
    clearFrame();
  }
  uint32_t step;
  switch (effect) {
    case 0: { // Layers move up and down.
      step = elapsed / 175;
      if (step == lastAnimationStep) return;
      lastAnimationStep = step;
      const uint8_t sequence[] = {0, 1, 2, 3, 2, 1};
      clearFrame();
      frame[sequence[step % 6]] = 0xFFFF;
      break;
    }
    case 1: { // A vertical column moves through all 16 positions.
      step = elapsed / 150;
      if (step == lastAnimationStep) return;
      lastAnimationStep = step;
      const uint8_t column = step % 16;
      const uint16_t mask = uint16_t(1U << column);
      for (uint8_t layer = 0; layer < 4; layer++) frame[layer] = mask;
      break;
    }
    case 2: { // Eight twinkling points.
      step = elapsed / 100;
      if (step == lastAnimationStep) return;
      lastAnimationStep = step;
      clearFrame();
      uint8_t count = 0;
      while (count < 8) {
        const uint8_t layer = random(4);
        const uint8_t column = random(16);
        const uint16_t mask = uint16_t(1U << column);
        if ((frame[layer] & mask) == 0) {
          frame[layer] |= mask;
          count++;
        }
      }
      break;
    }
    case 3: { // Fill and empty the cube.
      step = elapsed / 200;
      if (step == lastAnimationStep) return;
      lastAnimationStep = step;
      const uint8_t levels[] = {0, 1, 2, 3, 4, 3, 2, 1, 0};
      const uint8_t count = levels[step % 9];
      clearFrame();
      for (uint8_t layer = 0; layer < count; layer++) frame[layer] = 0xFFFF;
      break;
    }
    case 4: { // Three flashes followed by a pause.
      step = elapsed / 150;
      if (step == lastAnimationStep) return;
      lastAnimationStep = step;
      if (step < 6 && (step % 2 == 0)) fillFrame();
      else clearFrame();
      break;
    }
  }
}

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String command = String(characteristic->getValue().c_str());
    command.trim();
    command.toUpperCase();
    if (command == "ON" || command == "1") pendingCommand.store(CMD_ON);
    else if (command == "OFF" || command == "0") pendingCommand.store(CMD_OFF);
  }
};

class ServerCallbacks : public BLEServerCallbacks {
  void onDisconnect(BLEServer *server) override {
    restartAdvertising.store(true);
  }
};

void setup() {
  Serial.begin(115200);
  for (uint8_t pin : PIN_LAYER) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  digitalWrite(PIN_DATA, LOW);
  digitalWrite(PIN_CLOCK, LOW);
  digitalWrite(PIN_LATCH, LOW);
  turnCubeOff();
  BLEDevice::init("Cubo_LED_4x4x4");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  BLEService *service = server->createService(SERVICE_UUID);
  BLECharacteristic *commandCharacteristic = service->createCharacteristic(
    COMMAND_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  commandCharacteristic->setCallbacks(new CommandCallbacks());
  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->start();
  Serial.println("Cubo pronto. Envie ON ou OFF via BLE.");
}

void loop() {
  if (restartAdvertising.exchange(false)) BLEDevice::startAdvertising();
  const int command = pendingCommand.exchange(CMD_NONE);
  if (command == CMD_ON) {
    turnCubeOff();
    effect = 0;
    effectStartMs = millis();
    lastAnimationStep = UINT32_MAX;
    scanLayer = 0;
    lastScanUs = micros() - LAYER_TIME_US;
    cubeEnabled = true;
    Serial.println("Animacoes iniciadas.");
  } else if (command == CMD_OFF) {
    cubeEnabled = false;
    turnCubeOff();
    Serial.println("Cubo desligado.");
  }
  if (!cubeEnabled) {
    delay(1);
    return;
  }
  updateAnimation(millis());
  refreshCube();
}
