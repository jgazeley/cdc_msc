/*  usb_descriptors.c ─ CDC-only version
 *
 *  Based on TinyUSB composite example.
 *  MSC interface has been completely disabled.
 */

#include "bsp/board_api.h"
#include "tusb.h"

/*------------------------------------------------------------------*/
/*  Device Descriptor
 *------------------------------------------------------------------*/

#define USB_VID   0xCafe
#define USB_BCD   0x0200

/* Product-ID helper: CDC=bit0, MSC=bit1, HID=bit2 … */
#define _PID_MAP(itf, n)   ( (CFG_TUD_##itf) << (n) )
#define USB_PID  (0x4000 | _PID_MAP(CDC,0) | _PID_MAP(MSC,1) | \
                            _PID_MAP(HID,2) | _PID_MAP(MIDI,3) | \
                            _PID_MAP(VENDOR,4) )

tusb_desc_device_t const desc_device = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = USB_BCD,

  // IAD required for CDC
  .bDeviceClass       = TUSB_CLASS_MISC,
  .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol    = MISC_PROTOCOL_IAD,

  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = USB_VID,
  .idProduct          = USB_PID,
  .bcdDevice          = 0x0100,

  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,

  .bNumConfigurations = 0x01
};

uint8_t const *tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}

/*------------------------------------------------------------------*/
/*  Configuration Descriptor
 *------------------------------------------------------------------*/

enum {
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
  ITF_NUM_TOTAL                /* == 2 interfaces */
};

/* Endpoint numbers (unchanged) */
#define EPNUM_CDC_NOTIF   0x81
#define EPNUM_CDC_OUT     0x02
#define EPNUM_CDC_IN      0x82

/* Configuration length: config + CDC only */
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

/* ---------------- Full-speed ---------------- */
uint8_t const desc_fs_configuration[] = {
  /* Config number, interface count, string-index, total length, attribute, power */
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

  /* CDC: ITF num, string-index, EP notify, EP out, EP in, EP size */
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF,
                     8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

#if TUD_OPT_HIGH_SPEED
/* ---------------- High-speed ---------------- */
uint8_t const desc_hs_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF,
                     8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 512),
};

/* other-speed & qualifier descriptors (unchanged) */
static uint8_t desc_other_speed_config[CONFIG_TOTAL_LEN];

tusb_desc_device_qualifier_t const desc_device_qualifier = {
  .bLength            = sizeof(tusb_desc_device_qualifier_t),
  .bDescriptorType    = TUSB_DESC_DEVICE_QUALIFIER,
  .bcdUSB             = USB_BCD,
  .bDeviceClass       = TUSB_CLASS_MISC,
  .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol    = MISC_PROTOCOL_IAD,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .bNumConfigurations = 0x01,
  .bReserved          = 0x00
};

uint8_t const *tud_descriptor_device_qualifier_cb(void)
{
  return (uint8_t const *) &desc_device_qualifier;
}

uint8_t const *tud_descriptor_other_speed_configuration_cb(uint8_t index)
{
  (void) index;
  memcpy(desc_other_speed_config,
         (tud_speed_get() == TUSB_SPEED_HIGH) ?
            desc_fs_configuration : desc_hs_configuration,
         CONFIG_TOTAL_LEN);

  desc_other_speed_config[1] = TUSB_DESC_OTHER_SPEED_CONFIG;
  return desc_other_speed_config;
}
#endif /* HS */

/* Select FS or HS descriptor at runtime */
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index;
#if TUD_OPT_HIGH_SPEED
  return (tud_speed_get() == TUSB_SPEED_HIGH) ?
            desc_hs_configuration : desc_fs_configuration;
#else
  return desc_fs_configuration;
#endif
}

/*------------------------------------------------------------------*/
/*  String Descriptors
 *------------------------------------------------------------------*/

enum {
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
};

/* Array of pointer to string descriptors */
char const *string_desc_arr[] = {
  (const char[]){ 0x09, 0x04 },  /* 0: English (0x0409) */
  "TinyUSB",                     /* 1: Manufacturer */
  "TinyUSB Device",              /* 2: Product     */
  NULL,                          /* 3: Serial (generated) */
  "TinyUSB CDC"                  /* 4: CDC Interface */
};

static uint16_t _desc_str[32+1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;
  uint8_t chr_count;

  if (index == STRID_LANGID) {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else if (index == STRID_SERIAL) {
    chr_count = (uint8_t) board_usb_get_serial(_desc_str + 1, 32);
  } else {
    /* Out of bounds / unsupported */
    if (index >= sizeof(string_desc_arr)/sizeof(string_desc_arr[0]) ||
        string_desc_arr[index] == NULL) return NULL;

    const char *str = string_desc_arr[index];
    chr_count       = (uint8_t) strlen(str);
    if (chr_count > 32) chr_count = 32;

    for (uint8_t i = 0; i < chr_count; i++) _desc_str[1+i] = str[i];
  }

  /* First 2 bytes: length & descriptor type */
  _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return _desc_str;
}
