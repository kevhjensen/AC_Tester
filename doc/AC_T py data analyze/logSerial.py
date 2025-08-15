import serial

ser = serial.Serial('COM6', 115200)  # adjust COM port and baudrate
with open('emulateData1.csv', 'w') as f:
    while True:
        line = ser.readline().decode(errors='ignore').strip()
        f.write(line + '\n')
        print(line)


