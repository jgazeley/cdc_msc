/*
 * TinyUSB CDC-only demo
 *  → 5-item CLI (item 1 prints, item 2 blinks LED)
 *  → Works without pico_stdio_usb
 */

#include "bsp/board_api.h"
#include "tusb.h"
#include <string.h>

/*------------------------------------------------------------------*/
/*  Blink-status timing                                              */
/*------------------------------------------------------------------*/
enum {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED     = 1000,
  BLINK_SUSPENDED   = 2500,
};
static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

/*------------------------------------------------------------------*/
/*  Tiny printf-less helpers                                         */
/*------------------------------------------------------------------*/
/* --- helpers ---------------------------------------------------- */
static void cdc_write_all(const char *buf, uint32_t len)
{
  while (len)
  {
    /* queue as much as TinyUSB will take this frame */
    uint32_t n = tud_cdc_write(buf, len);
    buf += n;
    len -= n;
    tud_cdc_write_flush();      // push immediately

    /* if buffer full, let USB task run until space opens */
    while (len && !tud_cdc_write_available())
      tud_task();
  }
}

/* wrappers to mimic puts / printf */
static void send(const char *s)                       { cdc_write_all(s, (uint32_t)strlen(s)); }
static void sendf(const char *fmt, char key, const char *txt)
{
  char buf[64];
  int n = snprintf(buf, sizeof buf, fmt, key, txt);
  cdc_write_all(buf, (uint32_t)n);
}

/*------------------------------------------------------------------*/
/*  Menu actions                                                     */
/*------------------------------------------------------------------*/
static void action_greet(void)       { send("\n[1] Hello from n64-pico!\r\n"); }
static void action_blink_once(void)  {
  board_led_on();  board_delay(150); board_led_off();
  send("\n[2] LED blinked.\r\n");
}
static void action_stub(void)        { send("\n(Not implemented yet)\r\n"); }

/* Key, description, callback */
typedef void (*menu_fn_t)(void);
typedef struct { char key; const char *txt; menu_fn_t fn; } menu_t;

static const menu_t menu[] = {
  { '1', "Print greeting" , action_greet      },
  { '2', "Blink LED once" , action_blink_once },
  { '3', "Reserved"       , action_stub       },
  { '4', "Reserved"       , action_stub       },
  { '5', "Reserved"       , action_stub       },
};
#define MENU_COUNT (sizeof menu / sizeof menu[0])

/*------------------------------------------------------------------*/
/*  Forward decls                                                    */
/*------------------------------------------------------------------*/
static void led_blinking_task(void);
static void menu_task(void);
static void show_menu(void);

/*------------------------------------------------------------------*/
/*  MAIN                                                             */
/*------------------------------------------------------------------*/
int main(void)
{
  board_init();

  /* TinyUSB device init */
  tusb_rhport_init_t dev_init = {
    .role  = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO
  };
  tusb_init(BOARD_TUD_RHPORT, &dev_init);
  if (board_init_after_tusb) board_init_after_tusb();

  while (1) {
    tud_task();           /* USB device stack */
    led_blinking_task();
    menu_task();
  }
}

/*------------------------------------------------------------------*/
/*  Menu implementation                                              */
/*------------------------------------------------------------------*/
static bool prev_connected = false;

static void show_menu(void)
{
  send("\n========== n64-pico CLI ==========\r\n");
  for (size_t i = 0; i < MENU_COUNT; ++i)
    sendf("[%c] %s\r\n", menu[i].key, menu[i].txt);
  send("----------------------------------\r\n> ");
}

static void menu_task(void)
{
  bool now_connected = tud_cdc_connected();
  if (now_connected && !prev_connected) show_menu();
  prev_connected = now_connected;
  if (!now_connected) return;

  char ch;
  while (tud_cdc_available()) {
    tud_cdc_read(&ch, 1);

    bool handled = false;
    for (size_t i = 0; i < MENU_COUNT; ++i) {
      if (ch == menu[i].key) {
        menu[i].fn();
        handled = true;
        break;
      }
    }
    if (!handled && ch != '\r' && ch != '\n')
      send("\n? unknown choice\r\n");

    show_menu();                   /* refresh list */
  }
}

/*------------------------------------------------------------------*/
/*  TinyUSB CDC callbacks (unused)                                   */
/*------------------------------------------------------------------*/
void tud_cdc_rx_cb(uint8_t itf)  { (void) itf; }
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{ (void) itf; (void) dtr; (void) rts; }

/*------------------------------------------------------------------*/
/*  LED blink-status task                                            */
/*------------------------------------------------------------------*/
static void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  if (board_millis() - start_ms < blink_interval_ms) return;
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = !led_state;
}

/*------------------------------------------------------------------*/
/*  USB mount/resume hooks                                           */
/*------------------------------------------------------------------*/
void tud_mount_cb(void)      { blink_interval_ms = BLINK_MOUNTED;     }
void tud_umount_cb(void)     { blink_interval_ms = BLINK_NOT_MOUNTED; }
void tud_suspend_cb(bool r)  { (void) r; blink_interval_ms = BLINK_SUSPENDED; }
void tud_resume_cb(void)
{
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}
