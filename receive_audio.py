import serial
import wave
import struct
import sys


# Change this to your ESP32's serial port
# Run "ls /dev/ttyUSB*" or "ls /dev/ttyACM*" in terminal to find it
SERIAL_PORT = "/dev/ttyACM1"  #"/dev/ttyACM0"
BAUD_RATE = 921600
SAMPLE_RATE = 16000
OUTPUT_FILE = "recording.wav"



def save_wav(filename,samples,sample_rate):
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)

        samples_16bit = [max(-32768, min(32767, s >> 10)) for s in samples]
        wav_file.writeframes(struct.pack('<' + 'h' * len(samples_16bit), *samples_16bit))
    print(f"Saved {len(samples_16bit)} samples to {filename}")

def main():
    print(f"Opening {SERIAL_PORT} at {BAUD_RATE} baud...")
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print("Waiting for recording... press button on ESP32 to start")


    audio_data = []
    recording = False
    buffer = b""
    


    while True:
        chunk = ser.read(512)
        if not chunk:
            continue

        buffer += chunk





        if b"START" in buffer and not recording:
            recording = True
            audio_data = []
            
            buffer = buffer[buffer.index(b"START") + 5:]
            print("Recording started...")



        if b"STOP" in buffer and recording:
            stop_index = buffer.index(b"STOP")
            raw = buffer[:stop_index]
            recording = False
            buffer = b""

            # Convert raw bytes to 32 bit integers
            num_samples = len(raw) // 4
            samples = struct.unpack('<' + 'i' * num_samples, raw[:num_samples * 4])
            audio_data.extend(samples)

            print(f"Recording stopped. Saving...")
            save_wav(OUTPUT_FILE, audio_data, SAMPLE_RATE)
            print("Press button again to record another clip")

            # If recording, accumulate full 32 bit samples from buffer
        if recording:
            num_samples = len(buffer) // 4
            if num_samples > 0:
                raw = buffer[:num_samples * 4]
                samples = struct.unpack('<' + 'i' * num_samples, raw)
                audio_data.extend(samples)
                buffer = buffer[num_samples * 4:]

if __name__ == "__main__":
    main()