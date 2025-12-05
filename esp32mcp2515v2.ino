//------------------------------------------------------------
//   Dual MCP2515 (16 MHz) + Dual SLCAN Bridged
//   USB  <---SLCAN--->  CAN1 (MCP2515 #1)
//   Serial1 <---SLCAN---> CAN2 (MCP2515 #2)
//------------------------------------------------------------

#include <mcp_can.h>
#include <SPI.h>

// ------------------------------------------------------------
// MCP2515 #1  - asociado al USB (Serial)
// ------------------------------------------------------------
#define CAN1_CS 5
#define CAN1_INT 4
MCP_CAN CAN1(CAN1_CS);

// ------------------------------------------------------------
// MCP2515 #2 - asociado a Serial1
// ------------------------------------------------------------
#define CAN2_CS 15
#define CAN2_INT 2
MCP_CAN CAN2(CAN2_CS);

// ------------------------------------------------------------
// Serial1 HW
// ------------------------------------------------------------
#define SERIAL1_RX 16
#define SERIAL1_TX 17
HardwareSerial Serial1Hw(1);

// ------------------------------------------------------------
String usbBuffer = "";
String ser1Buffer = "";

// ============================================================
//          VALIDAR SI UNA CADENA ES UN FRAME SLCAN
// ============================================================
bool isValidSlcanFrame(const String &cmd) {
  if (cmd.length() < 1) return false;

  char c = cmd.charAt(0);

  if (c == 'O' || c == 'C') return true;
  if (c == 'S' && cmd.length() == 2) return true;

  if (c == 't' || c == 'r') {
    if (cmd.length() < 5) return false;
    return true;
  }

  if (c == 'T' || c == 'R') {
    if (cmd.length() < 10) return false;
    return true;
  }

  return false;
}

// ============================================================
//       Convertir SLCAN → MCP2515 send
// ============================================================
bool sendSlcanToMcp2515(const String &cmd, MCP_CAN &mcp) {
  char type = cmd.charAt(0);

  bool ext = (type == 'T' || type == 'R');

  int idLen = ext ? 8 : 3;
  String idStr = cmd.substring(1, 1 + idLen);
  unsigned long id = strtoul(idStr.c_str(), NULL, 16);

  int dlcIndex = ext ? 1 + 8 : 1 + 3;
  int dlc = cmd.charAt(dlcIndex) - '0';
  if (dlc < 0 || dlc > 8) return false;

  byte data[8];
  int dataIndex = dlcIndex + 1;

  for (int i = 0; i < dlc; i++) {
    String byteStr = cmd.substring(dataIndex + (i * 2), dataIndex + (i * 2) + 2);
    data[i] = strtoul(byteStr.c_str(), NULL, 16);
  }

  byte sendResult = mcp.sendMsgBuf(id, ext, dlc, data);

  return (sendResult == CAN_OK);
}

// ============================================================
//      Convertir MCP2515 → SLCAN y enviarlo
// ============================================================
void sendMcp2515ToSlcan(MCP_CAN &mcp, Stream &out) {
  unsigned long rxId;
  byte len;
  byte buf[8];

  mcp.readMsgBuf(&rxId, &len, buf);

  bool ext = (rxId > 0x7FF);

  char frameType = ext ? 'T' : 't';

  out.print(frameType);

  if (ext) {
    char idStr[9];
    sprintf(idStr, "%08lX", rxId);
    out.print(idStr);
  } else {
    char idStr[4];
    sprintf(idStr, "%03lX", rxId);
    out.print(idStr);
  }

  out.print(len);

  for (int i = 0; i < len; i++) {
    char byteStr[3];
    sprintf(byteStr, "%02X", buf[i]);
    out.print(byteStr);
  }

  out.print("\r");
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial1Hw.begin(115200, SERIAL_8N1, SERIAL1_RX, SERIAL1_TX);

  SPI.begin();

  // MCP2515 #1 → Serial USB
  if (CAN1.begin(MCP_STDEXT, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    Serial.println("CAN1 OK");
  } else {
    Serial.println("CAN1 FAIL");
  }
  CAN1.setMode(MCP_NORMAL);

  // MCP2515 #2 → Serial1
  if (CAN2.begin(MCP_STDEXT, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    Serial.println("CAN2 OK");
  } else {
    Serial.println("CAN2 FAIL");
  }
  CAN2.setMode(MCP_NORMAL);

  delay(200);
  Serial.println("Dual MCP2515 + Dual SLCAN READY");
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================
void loop() {
  // ------------------------------------------------------------
  //      CAN1 interrupt? → enviar SLCAN al USB
  // ------------------------------------------------------------
  if (!digitalRead(CAN1_INT)) {
    sendMcp2515ToSlcan(CAN1, Serial);
  }

  // ------------------------------------------------------------
  //      CAN2 interrupt? → enviar SLCAN al Serial1
  // ------------------------------------------------------------
  if (!digitalRead(CAN2_INT)) {
    sendMcp2515ToSlcan(CAN2, Serial1Hw);
  }

  // ------------------------------------------------------------
  //      USB (SavvyCAN) → CAN1 (MCP2515 #1)
  // ------------------------------------------------------------
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\r') {
      if (isValidSlcanFrame(usbBuffer)) {
        sendSlcanToMcp2515(usbBuffer, CAN1);
      }
      usbBuffer = "";
    } else usbBuffer += c;
  }

  // ------------------------------------------------------------
  //      Serial1 → CAN2 (MCP2515 #2)
  // ------------------------------------------------------------
  while (Serial1Hw.available()) {
    char c = Serial1Hw.read();

    if (c == '\r') {
      if (isValidSlcanFrame(ser1Buffer)) {
        sendSlcanToMcp2515(ser1Buffer, CAN2);
      }
      ser1Buffer = "";
    } else ser1Buffer += c;
  }

  
}
