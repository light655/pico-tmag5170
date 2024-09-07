import serial

device = input("Please input the serial device: ")
with serial.Serial(device) as ser:
    while True:
        print("Currently measuring at 10kSPS on all 3 axes.")
        num_measure = int(input("Please input the number of measurements to take: "))
        print(num_measure)
        ser.write(b"S")
        bytes_to_write = num_measure.to_bytes(4, "little")
        print(bytes_to_write)
        ser.write(bytes_to_write)

        while num_measure > 0:
            bytes_read = ser.read(10)
            t1 = int.from_bytes(bytes_read[0:4], "little")
            Bx = int.from_bytes(bytes_read[4:6], "little", signed=True) * 300 / 32768
            By = int.from_bytes(bytes_read[6:8], "little", signed=True) * 300 / 32768
            Bz = int.from_bytes(bytes_read[8:10], "little", signed=True) * 300 / 32768
            print(f"{t1}: {Bx}, {By}, {Bz}")

            num_measure -= 1