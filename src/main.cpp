#include <M5Unified.hpp>
#include <BluetoothSerial.h>

BluetoothSerial BTSerial;
bool connected = false;
// ここに接続する対象のBT Mac-adrsを入れる
uint8_t adrs[6] = {0x28, 0xcd, 0xc1, 0x09, 0x77, 0x78};

//
void setup()
{
  M5.begin();

  M5.Display.clear();
  M5.Display.setFont(&lgfx::v1::fonts::AsciiFont8x16);

  String str = "Hello,World";
  M5.Display.drawString(str, 10, 10);

  BTSerial.begin("M5Propo", true);

  uint8_t macBT[6];
  esp_read_mac(macBT, ESP_MAC_BT);
  char buff[32];
  snprintf(buff, sizeof(buff), "%02x:%02x:%02x:%02x:%02x:%02x",
           macBT[0], macBT[1], macBT[2], macBT[3], macBT[4], macBT[5]);
  M5.Display.drawString(buff, 10, 30);

  while (!connected)
  {
    // connected = BTSerial.connect(adrs, true);
    connected = BTSerial.connect(adrs);
  }
  M5.Display.drawString("Connected", 10, 50);
}

//
void loop()
{
  M5.update();

  M5.Display.startWrite();

  // if (BTSerial.available())
  // {
  //   String data = BTSerial.readStringUntil('\r');
  //   M5.Display.fillRect(10, 130, 100, 50, TFT_BLACK);
  //   M5.Display.drawString(data, 10, 130);
  // }
  static int steer = 0;
  static int throttle = 0;
  bool left = M5.BtnA.isPressed();
  bool right = M5.BtnC.isPressed();
  bool accel = M5.BtnB.isPressed();

  int newSteer = left ? 0x1c : right ? 0x13
                                     : 0x10;
  int newThr = accel ? 0x01 : 0x00;

  if (newSteer != steer)
  {
    BTSerial.write(newSteer);
    steer = newSteer;
  }
  if (newThr != throttle)
  {
    BTSerial.write(newThr);
    throttle = newThr;
  }

  M5.Display.fillRect(10, 100, 20, 20, throttle ? TFT_RED : TFT_BLACK);
  M5.Display.fillRect(30, 100, 20, 20, steer & 0xc ? TFT_GREEN : steer & 0x3 ? TFT_YELLOW
                                                                             : TFT_BLACK);

  M5.Display.fillRect(90, 100, 20, 20, BTSerial.connected() ? TFT_PINK : TFT_BLACK);
  M5.Display.fillRect(110, 100, 20, 20, BTSerial.isClosed() ? TFT_PURPLE : TFT_BLACK);

  M5.Display.endWrite();
}
