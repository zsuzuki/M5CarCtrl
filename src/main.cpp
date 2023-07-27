#include <M5Unified.hpp>
#include <BluetoothSerial.h>
#include <Wire.h>

// M5Stick-C & joystick Hat を使用する
// https://www.switch-science.com/products/6074
#define USE_M5STICKC 1

namespace
{
  BluetoothSerial BTSerial;
  bool connected = false;
  // ここに接続する対象のBT Mac-adrsを入れる
  uint8_t adrs[6] = {0x28, 0xcd, 0xc1, 0x09, 0x77, 0x78};

  constexpr int JOY_ADDR = 0x38;

  bool optionButton = false;

#if USE_M5STICKC
  //
  void getStick(int &xAxis, int &yAxis)
  {
    Wire.beginTransmission(JOY_ADDR);
    Wire.write(0x02);
    Wire.endTransmission();
    Wire.requestFrom(JOY_ADDR, 3);
    if (Wire.available())
    {
      int8_t x_data = Wire.read();
      int8_t y_data = Wire.read();
      optionButton = !Wire.read();

      xAxis = x_data;
      yAxis = y_data;
    }
  }
#endif

  //
  [[maybe_unused]] void readState()
  {
    // if (BTSerial.available())
    // {
    //   String data = BTSerial.readStringUntil('\r');
    //   M5.Display.fillRect(10, 130, 100, 50, TFT_BLACK);
    //   M5.Display.drawString(data, 10, 130);
    // }
  }

  enum BtnIdx : size_t
  {
    BTN_A, // A
    BTN_B, // B
    BTN_C, // C
    BTN_P, // Power
    BTN_E, // Ext
    BTN_O, // Option
    BTN_SZ
  };
  using BtnArray = std::array<bool, BTN_SZ>;
  std::array<int, BTN_SZ> btnClr = {{
      TFT_RED,
      TFT_GREEN,
      TFT_BLUE,
      TFT_YELLOW,
      TFT_WHITE,
      TFT_SKYBLUE,
  }};

  //
  void dispButtons(const BtnArray &btns)
  {
    constexpr int sz = 15;
    int x = M5.Display.width() - sz * BTN_SZ;
    int y = M5.Display.height() - sz;

    for (size_t i = 0; i < BTN_SZ; i++)
    {
      M5.Display.fillRect(x, y, sz, sz, btns[i] ? btnClr[i] : TFT_BLACK);
      x += sz;
    }
  }

  //
  void dispBattery()
  {
    int x = M5.Display.width() - 100;
    int w = M5.Power.getBatteryLevel();
    int c = M5.Power.isCharging();
    M5.Display.fillRect(x + w, 2, 100 - w, 4, TFT_BLACK);
    M5.Display.fillRect(x, 2, w, 4, c ? TFT_GREEN : TFT_WHITE);
  }

  //
  void dispState()
  {
    constexpr int sz = 15;
    int x = 5;
    int y = M5.Display.height() - sz;
    auto col = BTSerial.connected() ? TFT_PINK : BTSerial.isClosed() ? TFT_PURPLE
                                                                     : TFT_BLACK;
    M5.Display.fillRect(x, y, sz, sz, col);
  }

}

//
void setup()
{
  M5.begin();

#if USE_M5STICKC
  Wire.begin(0, 26, 100000UL);

  M5.Display.setRotation(1);
  // M5.Display.setFont(&lgfx::v1::fonts::DejaVu9);
  M5.Display.setFont(&lgfx::v1::fonts::AsciiFont8x16);
#else
  M5.Display.setFont(&lgfx::v1::fonts::AsciiFont8x16);
#endif
  M5.Display.clear();

  String str = "Hello";
  M5.Display.drawString(str, 5, 0);

  BTSerial.begin("M5Propo", true);

  // uint8_t macBT[6];
  // esp_read_mac(macBT, ESP_MAC_BT);
  // char buff[32];
  // snprintf(buff, sizeof(buff), "%02x:%02x:%02x:%02x:%02x:%02x",
  //          macBT[0], macBT[1], macBT[2], macBT[3], macBT[4], macBT[5]);
  // M5.Display.drawString(buff, 10, 30);

  while (!connected)
  {
    connected = BTSerial.connect(adrs);
  }
  M5.Display.drawString("Connected", 10, 20);
}

//
void loop()
{
  M5.update();

  M5.Display.startWrite();
  {
    std::array<bool, BTN_SZ> btn;

    btn[BTN_A] = M5.BtnA.isPressed();
    btn[BTN_B] = M5.BtnB.isPressed();
    btn[BTN_C] = M5.BtnC.isPressed();
    btn[BTN_P] = M5.BtnPWR.isPressed();
    btn[BTN_E] = M5.BtnEXT.isPressed();
    btn[BTN_O] = optionButton;
    dispButtons(btn);

    dispBattery();

#if USE_M5STICKC
    int xAx = 0;
    int yAx = 0;
    getStick(yAx, xAx);

    constexpr int cw = 50;
    xAx = -(xAx * cw) / 128;
    {
      // 有効範囲は 10~40くらい
      constexpr int cx = 80;
      constexpr int y = 45;
      constexpr int h = 10;
      int lxw = xAx < 0 ? xAx : 0;
      int rxw = xAx > 0 ? xAx : 0;
      M5.Display.fillRect(cx - cw, y, cw + lxw, h, TFT_BLACK);
      M5.Display.fillRect(cx + lxw, y, -lxw, h, TFT_GREENYELLOW);
      M5.Display.fillRect(cx, y, rxw, h, TFT_GREENYELLOW);
      M5.Display.fillRect(cx + rxw, y, cw - rxw, h, TFT_BLACK);

      int left = xAx < -10 ? 0x1 : 0;
      int right = xAx > 10 ? 0x4 : 0;
      int steer = left | right | 0x10;
      static int prevSteer = 0x10;
      if (prevSteer != steer)
      {
        BTSerial.write(steer);
        prevSteer = steer;
      }

      int forward = btn[BTN_A] ? 0x1 : 0;
      int back = btn[BTN_B] ? 0x4 : 0;
      int accel = forward | back;
      static int prevAccel = 0;
      if (prevAccel != accel)
      {
        BTSerial.write(accel);
        prevAccel = accel;
      }
    }
#else
    static int steer = 0;
    static int throttle = 0;
#endif

    // bool left = M5.BtnA.isPressed();
    // bool right = M5.BtnC.isPressed();
    // bool accel = M5.BtnB.isPressed();

    // int newSteer = left ? 0x1c : right ? 0x13
    //                                    : 0x10;
    // int newThr = accel ? 0x01 : 0x00;

    // if (newSteer != steer)
    // {
    //   BTSerial.write(newSteer);
    //   steer = newSteer;
    // }
    // if (newThr != throttle)
    // {
    //   BTSerial.write(newThr);
    //   throttle = newThr;
    // }

    // M5.Display.fillRect(10, 100, 20, 20, throttle ? TFT_RED : TFT_BLACK);
    // M5.Display.fillRect(30, 100, 20, 20, steer & 0xc ? TFT_GREEN : steer & 0x3 ? TFT_YELLOW
    //                                                                            : TFT_BLACK);
    dispState();
  }

  M5.Display.endWrite();
  delay(10);
}
