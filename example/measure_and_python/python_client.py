import serial

device = input("Please input the serial device: ")
    # e.g. "/dev/ttyACM0" on unix, "COM2" on windows
with serial.Serial(device) as ser:
    while True:
        print("Currently measuring at 10kSPS on all 3 axes.")
        num_measure = int(input("Please input the number of measurements to take: "))
        ser.write(b"S")                                         # start conversion signal
        bytes_to_write = num_measure.to_bytes(4, "little")      # convert int to bytes in little endian
        ser.write(bytes_to_write)                               # send number of conversion to pico

        while num_measure > 0:
            bytes_read = ser.read(10)                           # read 10 bytes from serial port
            # convert data from bytes to int and apply adequate scaling for magnetic field values
            t1 = int.from_bytes(bytes_read[0:4], "little")
            Bx = int.from_bytes(bytes_read[4:6], "little", signed=True) * 300 / 32768
            By = int.from_bytes(bytes_read[6:8], "little", signed=True) * 300 / 32768
            Bz = int.from_bytes(bytes_read[8:10], "little", signed=True) * 300 / 32768
            print(f"{t1}: {Bx}, {By}, {Bz}")

            num_measure -= 1