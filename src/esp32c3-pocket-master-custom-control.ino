// ESP32-C3 BLE MIDI 控制端（主动连接 Pocket Master BLE）
//
// 按键功能可配置：8 个按键各自保存 {CC 号, 值, 通道} 到 NVS，
// 开机 setup() 读一次，loop() 查表调用 sendControlChange()。
// 修改配置通过 USB 串口（行式 JSON），写入 NVS 后重启生效。
//
// 重要：需要在 IDE 里把 “USB CDC On Boot” 设为 Enabled（FQBN 加 :CDCOnBoot=cdc），
//       否则 Serial 是 UART0(GPIO20/21)，USB 口上收不到任何数据。
//
// 串口协议（每行一个 JSON，'\n' 结束）
//   上位机 -> 设备
//     {"cmd":"ping"}                                       探测设备
//     {"cmd":"get"}                                        读配置
//     {"cmd":"set","key":1,"cc":24,"value":0,"channel":1}  改单键（key=1..8, cc=0..127, value=0..127 或 255=切换, channel=1..16）
//     {"cmd":"reset"}                                      恢复默认
//   设备 -> 上位机
//     {"ok":true,"cmd":"ping","product":"esp32c3-ble-midi-host"}
//     {"ok":true,"cmd":"get","keys":[{"key":1,"cc":24,"value":0,"channel":1}, ...]}
//     {"ok":false,"cmd":"set","error":"out of range"}
//   提示：BLE-MIDI 库自己的调试日志也会打在同一个串口上，那是纯文本行；
//         上位机只需解析以 '{' 开头并且含 "ok" 字段的行。
//         上位机必须校验 ping 响应里的 product == "esp32c3-ble-midi-host"，
//         否则可能误连到其他 ESP32 项目。

#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_Client_ESP32.h>

#include <Arduino_JSON.h>
#include <Preferences.h>

#define POCKET_MASTER_BLE "Pocket Master BLE" // device broadcast name, do not change!

#define ERASE_PIN GPIO_NUM_1
#define LED1_PIN GPIO_NUM_12
#define LED2_PIN GPIO_NUM_13

#define KEY_COUNT 8
#define KEY1_PIN GPIO_NUM_2
#define KEY2_PIN GPIO_NUM_3
#define KEY3_PIN GPIO_NUM_10
#define KEY4_PIN GPIO_NUM_6
#define KEY5_PIN GPIO_NUM_5
#define KEY6_PIN GPIO_NUM_4
#define KEY7_PIN GPIO_NUM_8
#define KEY8_PIN GPIO_NUM_9

#define CONFIG_PRODUCT "pocket-master-custom-control"
#define CONFIG_NAMESPACE "midicfg"
#define CONFIG_KEY "keys"

// value 的特殊取值：不用固定值，每次触发在 0 / 127 之间切换
#define CC_VALUE_TOGGLE 255

BLEMIDI_CREATE_INSTANCE(POCKET_MASTER_BLE, MIDI);

struct KeyConfig {
  uint8_t cc;
  uint8_t value;
  uint8_t channel;
};

static const KeyConfig DEFAULT_KEYS[KEY_COUNT] = {
    {24, 0,   1}, // KEY1  Preset -
    {25, 0,   1}, // KEY2  Preset +
    {58, 0,   1}, // KEY3  Tuner GUI Off
    {58, 127, 1}, // KEY4  Tuner GUI On
    {92, 0,   1}, // KEY5  Drum GUI Off
    {92, 127, 1}, // KEY6  Drum GUI On
    {93, 0,   1}, // KEY7  Drum Stop
    {93, 127, 1}, // KEY8  Drum Play
};

const uint8_t keyPins[KEY_COUNT] = {KEY1_PIN, KEY2_PIN, KEY3_PIN, KEY4_PIN, KEY5_PIN, KEY6_PIN, KEY7_PIN, KEY8_PIN};
KeyConfig keyConfig[KEY_COUNT];
bool toggleState[KEY_COUNT] = {false}; // “切换”型按键当前状态
Preferences prefs;
String rxLine;

bool isConnected = false;

// 按键去抖 + 边沿检测：只在“按下”的那一刻发一次，长按不会重复触发
struct KeyState {
  bool lastRaw;             // 上一次读到的原始电平（按下为 true）
  bool stable;              // 去抖后的稳定电平
  unsigned long changedAt;  // lastRaw 最近一次变化的时间
  unsigned long repeatAt;   // 长按连发的下一次触发时间
};
const unsigned long DEBOUNCE_MS = 15; // 电平需稳定这么久才认作有效变化
KeyState keyState[KEY_COUNT];

// 长按连发：仅这几个 CC 需要（预设组 -/+、预设 -/+）
const uint8_t REPEAT_CCS[] = {22, 23, 24, 25};
const unsigned long REPEAT_DELAY_MS = 500;    // 按住多久后开始连发
const unsigned long REPEAT_INTERVAL_MS = 200; // 连发间隔

bool isRepeatable(uint8_t cc) {
  for (uint8_t c : REPEAT_CCS) {
    if (c == cc) return true;
  }
  return false;
}

void onConnected() {
  isConnected = true;
  digitalWrite(LED1_PIN, HIGH);
}

void onDisconnected() {
  isConnected = false;
  digitalWrite(LED1_PIN, LOW);
}

// ---------------- NVS 读写 ----------------
bool readKeysFromNvs(KeyConfig *out) {
  const size_t bytes = sizeof(KeyConfig) * KEY_COUNT;
  prefs.begin(CONFIG_NAMESPACE, true);
  bool ok = (prefs.getBytesLength(CONFIG_KEY) == bytes) && (prefs.getBytes(CONFIG_KEY, out, bytes) == bytes);
  prefs.end();
  return ok;
}

void writeKeysToNvs(const KeyConfig *in) {
  prefs.begin(CONFIG_NAMESPACE, false);
  prefs.putBytes(CONFIG_KEY, in, sizeof(KeyConfig) * KEY_COUNT);
  prefs.end();
}

void loadKeyConfig() {
  if (readKeysFromNvs(keyConfig)) {
    Serial.println("[配置] 已从 NVS 读取按键配置");
  } else {
    memcpy(keyConfig, DEFAULT_KEYS, sizeof(keyConfig));
    writeKeysToNvs(keyConfig);
    Serial.println("[配置] NVS 无有效配置，已写入默认值");
  }

  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    if (keyConfig[i].value == CC_VALUE_TOGGLE) {
      Serial.printf("[配置] KEY%u: CC %u = 切换 (CH %u)\n", i + 1, keyConfig[i].cc, keyConfig[i].channel);
    } else {
      Serial.printf("[配置] KEY%u: CC %u = %u (CH %u)\n", i + 1, keyConfig[i].cc, keyConfig[i].value, keyConfig[i].channel);
    }
  }
}

void replyKeys(const char *cmd, const KeyConfig *keys) {
  JSONVar root;
  root["ok"] = true;
  root["cmd"] = cmd;

  JSONVar arr;
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    JSONVar item;
    item["key"] = i + 1;
    item["cc"] = keys[i].cc;
    item["value"] = keys[i].value;
    item["channel"] = keys[i].channel;
    arr[i] = item;
  }
  root["keys"] = arr;

  Serial.println(JSON.stringify(root));
}

void replyError(const char *cmd, const char *error) {
  JSONVar root;
  root["ok"] = false;
  root["cmd"] = cmd;
  root["error"] = error;
  Serial.println(JSON.stringify(root));
}

void handleCommand(const String &line) {
  JSONVar req = JSON.parse(line);
  if (JSON.typeof(req) != "object") {
    replyError("", "invalid json");
    return;
  }

  const char *cmd = (const char *)req["cmd"];
  if (cmd == nullptr) {
    replyError("", "missing cmd");
    return;
  }

  if (strcmp(cmd, "ping") == 0) {
    JSONVar root;
    root["ok"] = true;
    root["cmd"] = "ping";
    root["product"] = CONFIG_PRODUCT;
    Serial.println(JSON.stringify(root));
    return;
  }

  if (strcmp(cmd, "get") == 0) {
    KeyConfig tmp[KEY_COUNT];
    if (!readKeysFromNvs(tmp)) {
      memcpy(tmp, keyConfig, sizeof(tmp));
    }
    replyKeys("get", tmp);
    return;
  }

  if (strcmp(cmd, "set") == 0) {
    if (!req.hasOwnProperty("key") || !req.hasOwnProperty("cc") || !req.hasOwnProperty("value") || !req.hasOwnProperty("channel")) {
      replyError("set", "need key,cc,value,channel");
      return;
    }

    int key = (int)req["key"];
    int cc = (int)req["cc"];
    int value = (int)req["value"];
    int channel = (int)req["channel"];

    if (key < 1 || key > KEY_COUNT || cc < 0 || cc > 127 || value < 0 || (value > 127 && value != CC_VALUE_TOGGLE) || channel < 1 || channel > 16) {
      replyError("set", "out of range");
      return;
    }

    KeyConfig tmp[KEY_COUNT];
    if (!readKeysFromNvs(tmp)) {
      memcpy(tmp, keyConfig, sizeof(tmp));
    }
    tmp[key - 1].cc = (uint8_t)cc;
    tmp[key - 1].value = (uint8_t)value;
    tmp[key - 1].channel = (uint8_t)channel;
    writeKeysToNvs(tmp);

    Serial.printf("[配置] KEY%d 已写入 NVS，重启后生效\n", key);
    replyKeys("set", tmp); // 回显整表，上位机无需再 get
    return;
  }

  if (strcmp(cmd, "reset") == 0) {
    writeKeysToNvs(DEFAULT_KEYS);
    Serial.println("[配置] 已恢复默认，重启后生效");
    replyKeys("reset", DEFAULT_KEYS);
    return;
  }

  replyError(cmd, "unknown cmd");
}

void pollConfigSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      if (rxLine.length() > 0) {
        handleCommand(rxLine);
      }
      rxLine = "";
    }
    else if (c != '\r' && rxLine.length() < 256) {
      rxLine += c;
    }
  }
}

// ---------------- 按键扫描 ----------------
void sendKey(uint8_t i) {
  if (!isConnected) return;

  uint8_t value = keyConfig[i].value;
  if (value == CC_VALUE_TOGGLE) { // 切换型：每次按下在 开/关 之间翻转
    toggleState[i] = !toggleState[i];
    value = toggleState[i] ? 127 : 0;
  }
  MIDI.sendControlChange(keyConfig[i].cc, value, keyConfig[i].channel);
}

// 每轮 loop 调用一次：检测下降沿（按下），支持多键同时按
void pollKeys() {
  const unsigned long now = millis();
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    const bool raw = (digitalRead(keyPins[i]) == LOW); // 按下 = true

    if (raw != keyState[i].lastRaw) {
      keyState[i].lastRaw = raw;
      keyState[i].changedAt = now;
      continue;
    }
    if (now - keyState[i].changedAt < DEBOUNCE_MS) continue; // 还在抖动窗口内

    if (keyState[i].stable != raw) {
      keyState[i].stable = raw;
      if (raw) {
        sendKey(i); // 按下沿立即发一次
        keyState[i].repeatAt = now + REPEAT_DELAY_MS;
      }
      continue;
    }

    // 长按连发：只对预设 -/+ 这类 CC 生效，且不用于“切换”型按键
    if (!keyState[i].stable || keyConfig[i].value == CC_VALUE_TOGGLE) continue;
    if (!isRepeatable(keyConfig[i].cc)) continue;
    if ((long)(now - keyState[i].repeatAt) < 0) continue;

    keyState[i].repeatAt = now + REPEAT_INTERVAL_MS;
    sendKey(i);
  }
}

void setup() {
  Serial.begin(115200);

  loadKeyConfig();
  pinMode(LED1_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);

  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    pinMode(keyPins[i], INPUT_PULLUP);
    const bool raw = (digitalRead(keyPins[i]) == LOW);
    keyState[i].lastRaw = raw;
    keyState[i].stable = raw;
    keyState[i].changedAt = millis();
    keyState[i].repeatAt = millis() + REPEAT_DELAY_MS;
  }

  BLEMIDI.configErasePin(ERASE_PIN);
  BLEMIDI.setHandleConnected(onConnected);
  BLEMIDI.setHandleDisconnected(onDisconnected);
  MIDI.begin();
}

void loop() {
  MIDI.read();
  pollConfigSerial();
  pollKeys();
  delay(2);
}
