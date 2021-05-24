# NodeMcu
Arduino+NodeMcu+WiFi+MQTT+OLED+DHT11

上电后 WiFi 未连接状态，ESP8266 模组上的 LED 灯会不停闪烁（亮灭各500ms）
此时可通过 Esptouch APP 配网，配网成功后 LED 灯会快速闪烁5秒种。
配网成功后 WiFi SSID 和密码将保存到 FLASH，下次上电会读取 SSID 和密码，直接连接 WiFi。
