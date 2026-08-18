#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// ===================== AUDIO IIS =====================
#define AUDIO_I2S_GPIO_MCLK      GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK      GPIO_NUM_5
#define AUDIO_I2S_GPIO_DIN       GPIO_NUM_6
#define AUDIO_I2S_GPIO_WS        GPIO_NUM_7
#define AUDIO_I2S_GPIO_DOUT      GPIO_NUM_8
#define AUDIO_CODEC_PA_PIN       GPIO_NUM_1

// ===================== AUDIO I2C =====================
#define AUDIO_CODEC_I2C_NUM      I2C_NUM_0
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_15
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_16
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR

// ===================== PCF8574 BUTTONS =====================
// Joystick
#define PCF_BTN_UP        0   // Volume +
#define PCF_BTN_DOWN      1   // Volume -
#define PCF_BTN_LEFT      2   // Backlight -
#define PCF_BTN_RIGHT     3   // Backlight +

// Middle = BOOT
#define PCF_BTN_MIDDLE    4   // BOOT (Listen/Idle)

// Set = liber
#define PCF_BTN_SET       5   // Unused for now

// RST = RESET
#define PCF_BTN_RST       6   // Software reset

// Lampă
#define PCF_PIN_LAMP      7   // Lamp output

// ===================== GPIO BUTTONS (DISABLED) =====================
#define BUILTIN_LED_GPIO        GPIO_NUM_42
#define BOOT_BUTTON_GPIO          GPIO_NUM_NC
#define VOLUME_UP_BUTTON_GPIO     GPIO_NUM_NC
#define VOLUME_DOWN_BUTTON_GPIO   GPIO_NUM_NC
#define BACKLIGHT_UP_BUTTON_GPIO  GPIO_NUM_NC
#define BACKLIGHT_DOWN_BUTTON_GPIO GPIO_NUM_NC
#define RESET_BUTTON_GPIO         GPIO_NUM_NC

// ===================== LCD DISPLAY =====================
#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_45

#define DISPLAY_RST_PIN       GPIO_NUM_NC
#define DISPLAY_SCK_PIN       GPIO_NUM_12
#define DISPLAY_DC_PIN        GPIO_NUM_46
#define DISPLAY_CS_PIN        GPIO_NUM_10
#define DISPLAY_MOSI_PIN      GPIO_NUM_11
#define DISPLAY_MIS0_PIN      GPIO_NUM_13
#define DISPLAY_SPI_SCLK_HZ   (20 * 1000 * 1000)

#define LCD_SPI_HOST          SPI3_HOST

#define LCD_TYPE_ILI9341_SERIAL
#define DISPLAY_WIDTH         320
#define DISPLAY_HEIGHT        240
#define DISPLAY_MIRROR_X      false
#define DISPLAY_MIRROR_Y      false
#define DISPLAY_SWAP_XY       true
#define DISPLAY_INVERT_COLOR  true
#define DISPLAY_RGB_ORDER     LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_OFFSET_X      0
#define DISPLAY_OFFSET_Y      0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE      0

// ===================== SD CARD =====================
#define SD_MMC_CMD GPIO_NUM_40
#define SD_MMC_CLK GPIO_NUM_38
#define SD_MMC_D0  GPIO_NUM_39
#define SD_MMC_D1  GPIO_NUM_41
#define SD_MMC_D2  GPIO_NUM_48
#define SD_MMC_D3  GPIO_NUM_47

// ===================== LAMP (native disabled) =====================
#define LAMP_GPIO GPIO_NUM_NC

// ===================== DHT SENSOR =====================
#define DHT11_PIN GPIO_NUM_NC

#endif  // _BOARD_CONFIG_H_
