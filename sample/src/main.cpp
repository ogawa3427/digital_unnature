#include "mbedtls/md.h"
#include <M5Unified.h>

std::vector<uint8_t> calculate_sha256(const uint8_t *input, size_t input_len)
{
  std::vector<uint8_t> output(32);
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, input, input_len);
  mbedtls_md_finish(&ctx, output.data());
  mbedtls_md_free(&ctx);

  return output;
}

void printAsHEX(const String &receivedData)
{
  for (int i = 0; i < receivedData.length(); i++)
  {
    USBSerial.print(receivedData[i], HEX); // 受信したデータを16進数で出力
    USBSerial.print(" ");                  // 数字の間にスペースを入れる
  }
  USBSerial.println(); // 改行を出力
}

void setup()
{
  USBSerial.begin(115200);
  USBSerial.println("Hello, World!");

  Serial2.begin(115200, SERIAL_8N1, 5, 6);

  M5.begin();
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_YELLOW);
}

std::vector<uint8_t> hash;
std::vector<uint8_t> pastHash;
uint8_t mode = 0;

void loop()
{
  M5.update();
  String receivedData = "";
  // UARTでデータを受信
  if (Serial2.available())
  {
    receivedData = Serial2.readStringUntil('\n');
  }
  if (receivedData.length() > 0)
  {
    USBSerial.print("Received: ");
    printAsHEX(receivedData);

    //ランダム化
    if (pastHash.size() > receivedData.length())
    {
      for (int i = 0; i < receivedData.length(); i++)
      {
        receivedData[i] = (receivedData[i] + pastHash[i]) % 256;
      }
    }
    else
    {
      for (int i = 0; i < pastHash.size(); i++)
      {
        receivedData[i] = receivedData[i] ^ pastHash[i];
      }
    }

    // 受信データを16進数文字列に変換
    String hexString = "";
    for (int i = 0; i < receivedData.length(); i++)
    {
        char hex[3];
        sprintf(hex, "%02X", receivedData[i]);
        hexString += hex;
    }
    // SHA256を計算
    hash = calculate_sha256((uint8_t *)hexString.c_str(), hexString.length());

    // SHA256の結果を16進数で出力
    USBSerial.print("SHA256: ");
    for (size_t i = 0; i < hash.size(); i++)
    {
      USBSerial.print(hash[i], HEX);
      USBSerial.print(" ");
    }
    USBSerial.println();

    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setCursor(0, 0);
    if (mode == 1)
    {
      String hashString = "";
      for (size_t i = 0; i < hash.size(); i++)
      {
        char hex[3];
        sprintf(hex, "%02X", hash[i]);
        hashString += hex;
      }
      M5.Lcd.print(hashString);
    }
    else if (mode == 2)
    {
      for (size_t i = 0; i < receivedData.length(); i++)
      {
        M5.Lcd.print(receivedData[i]);
        M5.Lcd.print(" ");
      }
    }
    else
    {
      M5.Lcd.startWrite();
      for (int x = 0; x < 16; x++)
      {
        for (int y = 0; y < 16; y++)
        {
          uint16_t color = hash[y * 16 + x];
          color = (color >> 8) | (color << 8);
          M5.Lcd.fillRect(x * 16, y * 16, 16, 16, color);
        }
      }
      M5.Lcd.endWrite();
    }
    pastHash = hash;
  }
  if (M5.BtnA.wasPressed())
  {
    mode = (mode + 1) % 3;
  }
  delay(10);
}