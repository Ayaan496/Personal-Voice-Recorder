#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>

//Pins
#define SDA_PIN 8
#define SCL_PIN 9
#define I2S_SD_PIN 4
#define I2S_WS_PIN 5
#define I2S_SCK_PIN 6
#define BUTTON_PIN 3

//I2S config
#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 16000
#define BUFFER_SIZE 512

bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;




Adafruit_SSD1306 display(128,64,&Wire, -1);



bool isRecording = false;
int recordingSeconds = 0;
unsigned long lastSecondTick = 0;

int32_t audioBuffer[BUFFER_SIZE];


// put function declarations here:


void setupI2S(){

i2s_config_t config = {
  .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
  .sample_rate          = SAMPLE_RATE,
  .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
  .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
  .communication_format = I2S_COMM_FORMAT_STAND_I2S,
  .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count        = 4,
  .dma_buf_len          = BUFFER_SIZE,
  .use_apll             = false,
  .tx_desc_auto_clear   = false,
  .fixed_mclk           = 0
};

i2s_pin_config_t pins = {

  .bck_io_num = I2S_SCK_PIN,
  .ws_io_num = I2S_WS_PIN,
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = I2S_SD_PIN

};

i2s_driver_install(I2S_PORT, &config, 0, NULL);
i2s_set_pin(I2S_PORT, &pins);
i2s_start(I2S_PORT);

}



void showIdle(){

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("  Audio Recorder");
  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println("Tap button to");
  display.println("record audio.");
  display.setCursor(0, 50);
  display.println("Status: IDLE");
  display.display();
}


void showRecording(int seconds) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("  Audio Recorder");
  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

    // Flashing REC indicator
  if ((millis() / 500) % 2 == 0) {
    display.fillCircle(6, 28, 5, SSD1306_WHITE);
  }
  display.setTextSize(2);
  display.setCursor(16, 20);
  display.println("REC");

  display.setTextSize(1);
  display.setCursor(0, 44);
  display.print("Time: ");
  display.print(seconds);
  display.println("s");

  display.setCursor(0, 54);
  display.println("Click again to stop");
  display.display();
}

void showSaved(int seconds) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("  Audio Recorder");
  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(20, 20);
  display.println("SAVED");
  display.setTextSize(1);
  display.setCursor(0, 44);
  display.print("Duration: ");
  display.print(seconds);
  display.println("s");
  display.display();
}


void setup() {
  Serial.begin(921600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Wire.begin(SDA_PIN, SCL_PIN);


  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    
    Serial.println("Display not found");
    while(true);
  }

 setupI2S();

delay(100);
lastSecondTick = millis();
showIdle();
 

}

void sendAudioOverSerial(int32_t* buffer, size_t samples){
  Serial.write((uint8_t*)buffer, samples * sizeof(int32_t));
}

void loop() {
  bool currentButtonState = digitalRead(BUTTON_PIN);

  if (currentButtonState == LOW && lastButtonState == HIGH) {
    lastButtonState = currentButtonState;
    delay(50); // simple debounce

    // confirm it's still pressed after debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (!isRecording) {
        isRecording = true;
        recordingSeconds = 0;
        lastSecondTick = millis();
        
        Serial.write("START");
        showRecording(0);
      } else {
        isRecording = false;
        Serial.write("STOP");
        showSaved(recordingSeconds);
        delay(2000);
        showIdle();
      }
    }
  }

  lastButtonState = currentButtonState;

  if (isRecording) {
    size_t bytesRead = 0;
    i2s_read(I2S_PORT, audioBuffer, sizeof(audioBuffer), &bytesRead, pdMS_TO_TICKS(10));

     if (bytesRead > 0) {
      sendAudioOverSerial(audioBuffer, bytesRead / sizeof(int32_t));
    }



    if (millis() - lastSecondTick >= 1000) {
      recordingSeconds++;
      lastSecondTick = millis();
      showRecording(recordingSeconds);
    }
  }
}