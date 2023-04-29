#include "pico/stdlib.h"
#include "tusb.h"

// GP1 to GP5 : rows
// GP6 to GP13 : columns
const uint32_t row_pins = 0b00000000000000000000000000111110;
const uint32_t col_pins = 0b00000000000000000011111111000000;
#define LED_PIN 25

// pico-sdk/lib/tinyusb/src/class/hid/hid.h
uint8_t const conv_table[128][2] =  { HID_ASCII_TO_KEYCODE };

uint8_t const keymap[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, HID_KEY_KEYPAD_1, HID_KEY_KEYPAD_2, HID_KEY_KEYPAD_3, 0,
  0, 0, 0, 0, HID_KEY_KEYPAD_4, HID_KEY_KEYPAD_5, HID_KEY_KEYPAD_6, 0,
  0, 0, 0, 0, HID_KEY_KEYPAD_7, HID_KEY_KEYPAD_8, HID_KEY_KEYPAD_9, HID_KEY_BACKSPACE,
  0, 0, 0, HID_KEY_NUM_LOCK, HID_KEY_KEYPAD_0, HID_KEY_KEYPAD_DECIMAL, HID_KEY_SPACE, 0
};

char* const textmap[] = {
  0, 0, 0, 0, "TIME", "TEST", "BKG", "Home",
  "MO ASSAY", "Cobalt-57", "Gallium-67", "Technetium-99m", 0, 0, 0, "UTIL",
  "Iodine-125", "Chromium-51", "Iodine-123", "Iodine-131", 0, 0, 0, "Becquerel",
  "U2", "U4", "Xenon-133", "NUCL", 0, 0, 0, 0,
  "U3", "U5", "Thallium-201", 0, 0, 0, 0, "Enter.\n"
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

  if (x && y) return (y-1)*8 + (x-1);
  return 0;
}

void tud_task_wait(uint ms) {
  while (ms--) {
    tud_task();
    sleep_ms(1);
  }
}

void send_key(uint8_t key){
  if ( !tud_hid_ready() ) return;

  uint8_t keycode[6] = { 0 };
  keycode[0] = key;
  tud_hid_keyboard_report(1, 0, keycode);
}

void send_ascii(char chr){
  if ( !tud_hid_ready() ) return;

  uint8_t keycode[6] = { 0 };
  uint8_t modifier   = 0;
  if ( conv_table[chr][0] ) modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
  keycode[0] = conv_table[chr][1];
  tud_hid_keyboard_report(1, modifier, keycode);
}

void type(char* text) {
  char c;
  while (c = *text++) {
    send_ascii(c);

    tud_task_wait(10);
    send_key(0);
    tud_task_wait(10);
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
        if (textmap[key]) type(textmap[key]);
        else send_key(keymap[key]);

        lastkey=key;
      }
    }
  }

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
