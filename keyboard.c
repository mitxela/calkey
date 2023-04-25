#include "pico/stdlib.h"
#include "tusb.h"

// GP1 to GP5 : rows
// GP6 to GP13 : columns
const uint32_t row_pins = 0b00000000000000000000000000111110;
const uint32_t col_pins = 0b00000000000000000011111111000000;
#define LED_PIN 25

// keycodes: pico-sdk/lib/tinyusb/src/class/hid/hid.h
const uint8_t keymap[5][8] = {
  { HID_KEY_NONE, HID_KEY_NONE, HID_KEY_NONE, HID_KEY_NONE, HID_KEY_A, HID_KEY_B, HID_KEY_C, HID_KEY_D },
  { HID_KEY_E, HID_KEY_F, HID_KEY_G, HID_KEY_H, HID_KEY_I, HID_KEY_J, HID_KEY_K, HID_KEY_L },
  { HID_KEY_M, HID_KEY_N, HID_KEY_O, HID_KEY_P, HID_KEY_Q, HID_KEY_R, HID_KEY_S, HID_KEY_T },
  { HID_KEY_U, HID_KEY_V, HID_KEY_W, HID_KEY_X, HID_KEY_Y, HID_KEY_Z, HID_KEY_0, HID_KEY_1 },
  { HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5, HID_KEY_6, HID_KEY_7, HID_KEY_8, HID_KEY_9 }
};

static inline void gpio_setup(void){

  gpio_init_mask( row_pins | col_pins );
  gpio_set_mask( row_pins | col_pins );

  for (uint i=1;i<=13;i++) {
    gpio_set_pulls(i, false, true); // pull down all
  }

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, 1);

}

uint8_t scan_matrix(){

  gpio_set_dir_out_masked( row_pins );
  gpio_set_dir_in_masked( col_pins );
  sleep_us(100);
  uint32_t cols = (gpio_get_all() & col_pins) >>5;

  gpio_set_dir_out_masked( col_pins );
  gpio_set_dir_in_masked( row_pins );
  sleep_us(100);
  uint32_t rows = (gpio_get_all() & row_pins);

  uint8_t x=0, y=0;

  while (cols >>= 1) { x++; }
  while (rows >>= 1) { y++; }

  if (x && y) return keymap[y-1][x-1];
  return 0;
}

void send_key(uint8_t key){
  if ( !tud_hid_ready() ) return;

  uint8_t keycode[6] = { 0 };
  keycode[0] = key;
  tud_hid_keyboard_report(1, 0, keycode);
}

void tud_task_wait(uint ms) {
  while (ms--) {
    tud_task();
    sleep_ms(1);
  }
}

void main(){

  gpio_setup();
  tusb_init();

  uint8_t lastkey = 0;

  while (1) {
    tud_task_wait(1);

    uint8_t key = scan_matrix();

    if (lastkey!=key) {
      tud_task_wait(10);
      uint8_t debounce = scan_matrix();

      if (key==debounce) {
        send_key(key);
        lastkey=key;
      }
    }
  }

}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint8_t len)
{
  //send_hid_report( ... );
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  if (report_type == HID_REPORT_TYPE_OUTPUT && report_id == 1) {

    if ( bufsize < 1 ) return;
    uint8_t const kbd_leds = buffer[0];

    if (kbd_leds & KEYBOARD_LED_NUMLOCK) {
      gpio_put(LED_PIN, 1);
    } else {
      gpio_put(LED_PIN, 0);
    }
  }
}
