A custom from scratch voice recorder.

(Prototype)Hardware: 
- Esp32 S3 Super mini
- I2S omnidirectional Microphone
- Button
- SSD1306 128x64 OLED display

How to Use:
- Currently on the prototype, it is only able to connect through serial. run the python script
  with the main c++ file uploaded to the esp-32 and it should start recording. Click again to
  end the recording and save the data to a recording.wav file.

Future Additions:
- While this is the prototype, the finished version will include an stl file for a
  3d printed casing as well as a kicad schematic for the custom PCB. the final project will have
  its own SD card as well as an option to transfer data through wifi.
