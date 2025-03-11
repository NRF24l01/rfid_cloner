#include <Adafruit_NeoPixel.h>

#define LED_PIN 6
#define LED_COUNT 1
#define LED_BRIGHTNESS 100

class StatusLED {
  private:
    Adafruit_NeoPixel pixel;
    bool task_blinking = false;
    uint8_t r = 0, g = 0, b = 0;
    uint16_t timer = 0;
    uint32_t last_blink_time = 0;
    bool is_blink = false;

    bool slow_blinking = false;
    bool is_faling = true;
    uint8_t last_brightness = 0;
    uint16_t change_time = 0;
    uint32_t last_fade_time = 0;

  public:
    StatusLED() : pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800) { }

    void begin() {
      pixel.begin();
      pixel.setBrightness(LED_BRIGHTNESS);
      pixel.show();
    }

    void tick() {
      if (task_blinking) {
        if (millis() - last_blink_time > timer) {
          last_blink_time = millis();
          is_blink = !is_blink;
          
          if (is_blink) {
            pixel.setPixelColor(0, pixel.Color(r, g, b));
          } else {
            pixel.setPixelColor(0, pixel.Color(0, 0, 0));
          }
        }
      } else if (slow_blinking){
        if (millis() - last_fade_time > timer) {
          if (is_faling){
            last_brightness--;
            pixel.setBrightness(last_brightness);
            if (last_brightness<=0){
              is_failing = false;
            }
          } else {
            last_brightness++;
            pixel.setBrightness(last_brightness);
            if (last_brightness>=LED_BRIGHTNESS){
              is_failing = true;
            }
          }
          pixel.show();
        }
      }
    }

    void set_state(int rn, int gn, int bn) {
      task_blinking = false;
      slow_blinking = false;
      r = rn;
      g = gn;
      b = bn;
      pixel.setBrightness(LED_BRIGHTNESS);
      pixel.setPixelColor(0, pixel.Color(r, g, b));
      pixel.show();
    }

    void set_state(int rn, int gn, int bn, uint16_t time_interval) {
      task_blinking = true;
      slow_blinking = false;
      timer = time_interval;
      r = rn;
      g = gn;
      b = bn;
      last_blink_time = millis();
      pixel.setBrightness(LED_BRIGHT);
    }

    void set_state_fadnes(int rn, int gn, int bn, uint16_t time_interval) {
      slow_blinking = true;
      task_blinking = false;
      timer = time_interval;
      r = rn;
      g = gn;
      b = bn;
      pixel.setPixelColor(0, pixel.Color(r, g, b));
      last_brightness = LED_BRIGHTNESS;
      last_fade_time = millis();
    }
};
