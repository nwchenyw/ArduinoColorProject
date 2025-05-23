#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// 腳位定義
#define TFT_CS      10
#define TFT_RST     9
#define TFT_DC      8
#define S0          3
#define S1          4
#define S2          5
#define S3          6
#define SENSOR_OUT  7
#define LED_PIN     A0          // 顏色感測LED控制
#define CPU_LED_PIN A1
#define TRIGGER_PIN 12
#define OUTPUT_PIN  2

// 顏色常數
#define RED    0
#define GREEN  1
#define BLUE   2
#define COLORS_COUNT 3

// 時間常數
#define DISPLAY_TIME 2000
#define SAMPLING_DELAY 50
#define SAMPLING_COUNT 5
#define STABILIZATION_DELAY 200

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// 顏色值和計數
const uint16_t colors[COLORS_COUNT] = {ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE};
uint8_t quantities[COLORS_COUNT] = {0, 0, 0};
const char* colorNames[COLORS_COUNT] = {"R", "G", "B"};

// 狀態變數
bool triggered = false;
unsigned long showColorTime = 0;
bool showingColor = false;
int8_t detectedColorIndex = -1;

void setup() {
  // 初始化 TFT 顯示器
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(SENSOR_OUT, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(CPU_LED_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(OUTPUT_PIN, OUTPUT);

  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // 啟動時閃爍 LED
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);

  // 初始化 LED 和輸出腳位
  digitalWrite(OUTPUT_PIN, LOW);
  digitalWrite(CPU_LED_PIN, LOW);

  drawSummary();
}

void loop() {
  // 檢查觸發按鈕是否按下，且不在掃描過程中
  if (digitalRead(TRIGGER_PIN) == LOW && !triggered && !showingColor) {
    triggered = true;
    scanColor();
  }

  // 顯示時間到後重置顏色顯示
  if (showingColor && millis() - showColorTime >= DISPLAY_TIME) {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(OUTPUT_PIN, LOW);
    showingColor = false;
    drawSummary();
  }

  // 按鈕釋放時重置觸發標誌
  if (digitalRead(TRIGGER_PIN) == HIGH) {
    triggered = false;
  }

  // CPU LED 閃爍
  static unsigned long lastBlinkTime = 0;
  static bool cpuLedState = false;

  if (millis() - lastBlinkTime >= 500) {
    cpuLedState = !cpuLedState;
    digitalWrite(CPU_LED_PIN, cpuLedState ? HIGH : LOW);
    lastBlinkTime = millis();
  }
}

void scanColor() {
  // 開啟 LED 以提高顏色檢測準確度
  digitalWrite(LED_PIN, HIGH);

  // 等待感測器在 LED 開啟後穩定
  delay(STABILIZATION_DELAY);

  // 進行多次採樣以提高準確度
  uint8_t colorVotes[COLORS_COUNT] = {0};

  for (uint8_t i = 0; i < SAMPLING_COUNT; i++) {
    uint8_t detectedColor = readColor();
    if (detectedColor < COLORS_COUNT) {
      colorVotes[detectedColor]++;
    }
    delay(SAMPLING_DELAY);
  }

  // 找出投票最多的顏色
  uint8_t maxVotes = 0;
  detectedColorIndex = -1;

  for (uint8_t i = 0; i < COLORS_COUNT; i++) {
    if (colorVotes[i] > maxVotes) {
      maxVotes = colorVotes[i];
      detectedColorIndex = i;
    }
  }

  if (detectedColorIndex >= 0 && maxVotes > (SAMPLING_COUNT / 3)) {
    quantities[detectedColorIndex]++;
    tft.fillScreen(colors[detectedColorIndex]);

    tft.setCursor(tft.width()/2 - 20, tft.height()/2 - 10);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(3);
    tft.print(colorNames[detectedColorIndex]);

    digitalWrite(OUTPUT_PIN, HIGH);
    showingColor = true;
    showColorTime = millis();
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

uint8_t readColor() {
  // 紅
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  uint16_t red = pulseIn(SENSOR_OUT, LOW, 25000);

  // 綠
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  uint16_t green = pulseIn(SENSOR_OUT, LOW, 25000);

  // 藍
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  uint16_t blue = pulseIn(SENSOR_OUT, LOW, 25000);

  if (red < green && red < blue && red > 0) return RED;
  else if (green < red && green < blue && green > 0) return GREEN;
  else if (blue > 0) return BLUE;
  else return 0;
}

void drawSummary() {
  tft.fillScreen(ST77XX_BLACK);

  uint8_t sectionWidth = tft.width() / COLORS_COUNT;-*
  uint8_t sectionHeight = tft.height() / 2;

  for (uint8_t i = 0; i < COLORS_COUNT; i++) {
    uint8_t x = i * sectionWidth;

    tft.fillRect(x, 0, sectionWidth, sectionHeight, colors[i]);

    tft.fillRect(x, sectionHeight + 2, sectionWidth, tft.height() - sectionHeight - 2, ST77XX_BLACK);

    tft.setCursor(x + sectionWidth/2 - 10, sectionHeight + 10);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.print(colorNames[i]);

    tft.setCursor(x + 5, sectionHeight + 40);
    tft.setTextSize(3);
    tft.print(quantities[i]);
  }
}
