#include <Arduino.h>
#include <Wire.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <string.h>
#include <lvgl.h>
#include <Adafruit_MLX90614.h>
#include "ui/ui.h"

static constexpr int kWidth = 1024;
static constexpr int kHeight = 600;
static esp_lcd_panel_handle_t panel = nullptr;
static lv_disp_draw_buf_t lv_draw_buffer;
static lv_color_t *lv_pixels = nullptr;

static Adafruit_MLX90614 mlx = Adafruit_MLX90614();

static uint8_t writeIoExtension(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(0x24);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission();
}

static void lvglFlush(lv_disp_drv_t *display, const lv_area_t *area, lv_color_t *pixels) {
  esp_err_t result = esp_lcd_panel_draw_bitmap(
      panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, pixels);
  if (result != ESP_OK) {
    Serial.printf("LVGL display flush failed: %s\n", esp_err_to_name(result));
  }
  lv_disp_flush_ready(display);
}

static bool initLvglDisplay() {
  lv_init();

  constexpr uint32_t buffer_pixels = kWidth * 60; // 60 linhas de buffer são suficientes e economizam RAM interna
  lv_pixels = static_cast<lv_color_t *>(heap_caps_malloc(
      buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (lv_pixels == nullptr) {
    Serial.println("LVGL draw buffer allocation failed");
    return false;
  }
  memset(lv_pixels, 0, buffer_pixels * sizeof(lv_color_t));
  lv_disp_draw_buf_init(&lv_draw_buffer, lv_pixels, nullptr, buffer_pixels);

  static lv_disp_drv_t display_driver;
  lv_disp_drv_init(&display_driver);
  display_driver.hor_res = kWidth;
  display_driver.ver_res = kHeight;
  display_driver.flush_cb = lvglFlush;
  display_driver.draw_buf = &lv_draw_buffer;
  
  // Habilita a inversão de bytes para renderizar os bitmaps de texto corretamente em RGB565
  //display_driver.color_swap = 1; 

  lv_disp_drv_register(&display_driver);
  return true;
}

static void initRgbPanel() {
  esp_lcd_rgb_panel_config_t config = {};
  config.clk_src = LCD_CLK_SRC_DEFAULT;
  config.timings.pclk_hz = 30 * 1000 * 1000;
  config.timings.h_res = kWidth;
  config.timings.v_res = kHeight;
  config.timings.hsync_pulse_width = 162;
  config.timings.hsync_back_porch = 152;
  config.timings.hsync_front_porch = 48;
  config.timings.vsync_pulse_width = 45;
  config.timings.vsync_back_porch = 13;
  config.timings.vsync_front_porch = 3;
  config.timings.flags.pclk_active_neg = 1;

  config.data_width = 16;
  config.bits_per_pixel = 16;
  config.num_fbs = 1;
  config.bounce_buffer_size_px = kWidth * 10;
  config.sram_trans_align = 4;
  config.psram_trans_align = 64;
  config.hsync_gpio_num = GPIO_NUM_46;
  config.vsync_gpio_num = GPIO_NUM_3;
  config.de_gpio_num = GPIO_NUM_5;
  config.pclk_gpio_num = GPIO_NUM_7;
  config.disp_gpio_num = -1;

  const int data_pins[] = {
      GPIO_NUM_14, GPIO_NUM_38, GPIO_NUM_18, GPIO_NUM_17,
      GPIO_NUM_10, GPIO_NUM_39, GPIO_NUM_0,  GPIO_NUM_45,
      GPIO_NUM_48, GPIO_NUM_47, GPIO_NUM_21, GPIO_NUM_1,
      GPIO_NUM_2,  GPIO_NUM_42, GPIO_NUM_41, GPIO_NUM_40};
  for (int bit = 0; bit < 16; ++bit) {
    config.data_gpio_nums[bit] = data_pins[bit];
  }
  config.flags.fb_in_psram = 1;

  ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&config, &panel));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 1. Velocidade reduzida para 50kHz (mais estável para SMBus/GY-906)
  // 2. Wire.begin(SDA, SCL, FREQ)
  Wire.begin(8, 9, 50000);
  Wire.setTimeOut(200); // Timeout maior para o sensor responder

  Serial.println("Waveshare LCD test starting");

  // Scanner de segurança
  Serial.println("Scanner I2C:");
  for (uint8_t i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.printf(" - Encontrado 0x%02X\n", i);
    }
  }

  // Inicializa o expansor da tela primeiro (essencial para ligar o LCD)
  uint8_t mode_status = writeIoExtension(0x02, 0xFF);
  Serial.printf("IO expander status=%u\n", mode_status);

  // Inicializa o sensor GY-906
  if (!mlx.begin()) {
    Serial.println("ERRO: Sensor GY-906 não responde em 0x5A!");
  } else {
    Serial.println("GY-906 OK!");
  }

  initRgbPanel();
  Serial.println("Official RGB panel initialized");

  void *frame_buffer = nullptr;
  ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel, 1, &frame_buffer));
  memset(frame_buffer, 0, kWidth * kHeight * sizeof(uint16_t));

  uint8_t backlight_status = writeIoExtension(0x03, 0xFF);
  Serial.printf("IO expander output=%u\n", backlight_status);

  if (!initLvglDisplay()) return;

  ui_init(); // Carrega a interface do EEZ Studio
  Serial.println("EEZ UI loaded");

  // Atualização forçada inicial
  lv_timer_handler();
}

void loop() {
  lv_timer_handler();
  ui_tick();
  delay(5);

  static uint32_t tempo_anterior = 0;
  if (millis() - tempo_anterior > 800) { // Aumentei para 800ms para não sobrecarregar o barramento
    tempo_anterior = millis();

    // No ESP32-S3, às vezes o sensor trava se lermos muito rápido
    float t_amb = mlx.readAmbientTempC();
    delay(10); // Pequena pausa entre leituras
    float t_obj = mlx.readObjectTempC();

    // Se o erro -1 persistir, t_amb virá como NAN (Not a Number)
    if (!isnan(t_amb) && !isnan(t_obj)) {
    char str_buff[16];

        snprintf(str_buff, sizeof(str_buff), "%.1f C", t_amb);
    lv_label_set_text(objects.lbl_temp_amb, str_buff);
        lv_bar_set_value(objects.bar_temp_amb, (int)t_amb, LV_ANIM_ON);
        snprintf(str_buff, sizeof(str_buff), "%.1f C", t_obj);
        lv_label_set_text(objects.lbl_temp_obj, str_buff);
        lv_bar_set_value(objects.bar_temp_obj, (int)t_obj, LV_ANIM_ON);
    } else {
        Serial.println("Erro de leitura no GY-906");
}
  }
}

