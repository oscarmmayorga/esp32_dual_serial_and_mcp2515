# esp32_dual_serial_and_mcp2515

Dual MCP2515 (16 MHz) + Dual SLCAN  for ESP32
---------------------------------------------------

USB  <---SLCAN--->  CAN1 (MCP2515 #1)
Serial1 <---SLCAN---> CAN2 (MCP2515 #2)

This project provides firmware for an ESP32 that bridges two MCP2515 CAN controllers to two serial endpoints using the SLCAN protocol:

- MCP2515 #1 <-> USB Serial (Serial) — for tools like SavvyCAN or slcan host tools
- MCP2515 #2 <-> Serial1 (Hardware UART1) — for a second serial/CAN endpoint

Features
- Two independent MCP2515 controllers (16 MHz crystal)
- Each MCP2515 uses the shared SPI bus but separate CS and INT pins
- Converts between SLCAN frames and MCP2515 CAN frames in both directions
- Default CAN bitrate: 500 kbps (set in code)
- Uses Arduino-style HardwareSerial(1) for the second serial port

Status
- ESP32 code provided in the repository (main sketch).
- Tested for basic transmit/receive with SLCAN-style tools.

Wiring / Pinout
- SPI lines (shared between both MCP2515 modules). Default VSPI pins on many ESP32 boards:
  - SCK  -> GPIO 18
  - MISO -> GPIO 19
  - MOSI -> GPIO 23
  - (SPI.begin() uses default SPI pins; adjust if you use HSPI or custom assignment)

- MCP2515 #1 (connected to USB Serial):
  - CS  -> GPIO 5   (CAN1_CS)
  - INT -> GPIO 4   (CAN1_INT)
  - VCC -> 3.3V
  - GND -> GND
  - CANH/CANL -> CAN bus #1 (via transceiver on module, e.g., TJA1050)

- MCP2515 #2 (connected to Serial1):
  - CS  -> GPIO 15  (CAN2_CS)
  - INT -> GPIO 2   (CAN2_INT)
  - VCC -> 3.3V
  - GND -> GND
  - CANH/CANL -> CAN bus #2

- Serial1 (HardwareSerial(1)) pins used in code:
  - RX -> GPIO 16 (SERIAL1_RX)
  - TX -> GPIO 17 (SERIAL1_TX)
  - Baud: 115200 in sketch

Important notes
- MCP2515 modules often include the CAN transceiver (e.g., TJA1050). Make sure the module uses 3.3V levels and the transceiver matches your CAN bus voltage/signalling.
- MCP2515 INT pins are typically open-drain: enable (or ensure) a pull-up (many modules include a pull-up). If you see unstable behavior, add a 10k pull-up to 3.3V on each INT line.
- SPI pin assignments can vary by board. If your ESP32 board exposes different default SPI pins or you want to use HSPI, call SPI.begin(SCK, MISO, MOSI) with explicit pins or adapt wiring.
- CAN bitrate is currently set to 500 kbps in the code. Change the CAN begin parameters in the sketch if you need a different bitrate or crystal frequency.

Software / Build
- Arduino IDE:
  - Install ESP32 board support (Espressif) and select the correct board.
  - Install the MCP_CAN library  by Cory J. Fowler 
  - Open the .ino/.cpp file in this repo, compile and upload.

- PlatformIO:
  - Create a platformio.ini for your board, add the MCP_CAN library as dependency or include it in lib/.
  - Build & upload to the selected ESP32 board.

SLCAN protocol (brief)
- This firmware expects SLCAN-formatted frames ending with CR (\r).
- Common SLCAN frame types supported by the sketch:
  - "O" — open
  - "C" — close
  - "Sx" — set bitrate (where x is a single character)
  - "t" — standard (11-bit) data frame (lowercase 't') e.g. t123DATABYTES
  - "T" — extended (29-bit) data frame (uppercase 'T') e.g. T0011223344...
  - "r" / "R" — remote frames
- Example SLCAN data frame (standard ID = 0x123, 2 bytes: 0xAB 0xCD):
  - t1232ABCD\r
- Example SLCAN extended frame (ID = 0x1ABCDEFF, 3 bytes):
  - T1ABCDEFF3A1B2C\r

Usage examples
- Connect MCP2515 #1 via USB Serial to PC and use SavvyCAN or slcan utilities (e.g., slcand + cansend/candump) to send/receive CAN on CAN1.
- Connect another serial terminal or device to Serial1 pins (16/17) at 115200 to talk SLCAN to CAN2.

Troubleshooting
- If CAN modules don't initialize: check SPI wiring, CS pins, and that MCP_16MHZ constant matches the module's crystal (16 MHz).
- If interrupts aren't detected: verify INT pins wiring, ensure pull-ups exist, and that interrupts are configured as digital inputs.
- If you see garbled serial data: check baud rates and wiring, ensure TX/RX cross (TX->RX, RX->TX).

Files added
- README.md — this document
- SCHEMATIC.svg — wiring diagram (vector) showing ESP32, SPI lines, CS, INT and Serial1 pins

License
- Add your preferred license to the repository. Currently no license file is included — add LICENSE if you want to allow reuse.

Credits
- Uses MCP_CAN library and Arduino/ESP32 core. Original code and wiring by oscarmmayorga.

If you want, I can:
- Add a PlatformIO example/project file (platformio.ini)
- Produce PNG export of the SVG schematic
- Add a LICENSE file (MIT suggested)
- Add a minimal Arduino .ino with comments and compile instructions tailored to the Arduino IDE
