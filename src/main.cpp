//Libraries
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


//Variables
bool lastButtonState = HIGH;
Adafruit_SSD1306 display(128,64,&Wire, -1);
bool isRecording = false;
int recordingSeconds = 0;
unsigned long lastSecondTick = 0;
int32_t audioBuffer[BUFFER_SIZE];




//I2S setup
void setupI2S(){
  //config struct
  i2s_config_t config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),//Recieve only
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,//Stereo, switch to _ONLY_LEFT for mono
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
    .data_out_num = I2S_PIN_NO_CHANGE, //No data out
    .data_in_num = I2S_SD_PIN

  };

  //I2s activation
  i2s_driver_install(I2S_PORT, &config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pins);
  i2s_start(I2S_PORT);

}


//display idle screen
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

//display recording screen
void showRecording(int seconds) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("  Audio Recorder");
  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

    // Flashing REC indicator
  if ((millis() / 500) % 2 == 0) { //fashing circle every half second
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
//display saved screen
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

  //start the display and check if its findable
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    
    Serial.println("Display not found");
    while(true);
  }

  setupI2S();
  delay(100);
  lastSecondTick = millis();
  showIdle();
 

}

//send audio data over serial
void sendAudioOverSerial(int32_t* buffer, size_t samples){
  Serial.write((uint8_t*)buffer, samples * sizeof(int32_t));
}

void loop() {

  //Button logic
  bool currentButtonState = digitalRead(BUTTON_PIN);

  if (currentButtonState == LOW && lastButtonState == HIGH) {
    lastButtonState = currentButtonState;
    delay(50); //debounce

    //if button is pressed and not recording, start recording and start counting time
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (!isRecording) {
        isRecording = true;
        recordingSeconds = 0;
        lastSecondTick = millis();
        
        Serial.write("START");//send to python script
        showRecording(0);
      } else { //if button pressed and you are recording, stop recording
        isRecording = false;
        Serial.write("STOP");
        showSaved(recordingSeconds);
        delay(2000);
        showIdle();
      }
    }
  }

  lastButtonState = currentButtonState;

  //take data from dma buffer and put it into audio buffer
  if (isRecording) {
    size_t bytesRead = 0;
    i2s_read(I2S_PORT, audioBuffer, sizeof(audioBuffer), &bytesRead, pdMS_TO_TICKS(10));
    //if data was read, send the data over serial
     if (bytesRead > 0) {
      sendAudioOverSerial(audioBuffer, bytesRead / sizeof(int32_t));
    }


    //increment recording seconds every second
    if (millis() - lastSecondTick >= 1000) {
      recordingSeconds++;
      lastSecondTick = millis();
      showRecording(recordingSeconds);
    }
  }
}