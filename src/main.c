#include "bsp/board_api.h"
#include "tusb.h"
#include <string.h>
#include <stdbool.h>
#include <ctype.h> // Include for tolower()
#include "main_state.h" // Include our new header

//--------------------------------------------------------------------+
// Main Application State
//--------------------------------------------------------------------+
// This is the one true definition of the global state variable.
operating_mode_t current_mode = MODE_MSC_WITH_BASIC_REPL;

//--------------------------------------------------------------------+
// REPL and Task Declarations
//--------------------------------------------------------------------+
void basic_repl_task(void);
void full_repl_task(void);
void str_to_lower(char* str);

/*------------- MAIN -------------*/
int main(void) {
  board_init();

  tusb_rhport_init_t dev_init = {
    .role = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO
  };
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  if (board_init_after_tusb) {
    board_init_after_tusb();
  }

  while (1) {
    tud_task(); // tinyusb device task

    // Call the correct task based on the current operating mode
    switch (current_mode) {
      case MODE_MSC_WITH_BASIC_REPL:
        basic_repl_task();
        break;
      case MODE_FULL_REPL:
        full_repl_task();
        break;
    }
  }
}

//--------------------------------------------------------------------+
// Helper function to convert a string to lowercase
//--------------------------------------------------------------------+
void str_to_lower(char* str) {
  for (int i = 0; str[i]; i++) {
    // Cast the result of tolower() back to char to satisfy the compiler.
    // Also cast the input to unsigned char as is standard practice for ctype functions.
    str[i] = (char) tolower((unsigned char) str[i]);
  }
}

//--------------------------------------------------------------------+
// Basic REPL Task (Only listens for the switch-to-REPL command)
//--------------------------------------------------------------------+
void basic_repl_task(void) {
  if (!tud_cdc_available()) return;

  static char command_buffer[64];
  static uint32_t buffer_pos = 0;

  int32_t c = tud_cdc_read_char();
  if (c == -1) return;

  if (c == '\r') {
    command_buffer[buffer_pos] = '\0';
    str_to_lower(command_buffer); // Convert to lowercase

    if (strcmp(command_buffer, "repl") == 0) { // Compare with lowercase
      current_mode = MODE_FULL_REPL;
      board_delay(10);
      tud_cdc_write_str("\r\n--- Switched to Full REPL Mode ---\r\n");
      tud_cdc_write_str("Drive is now unmounted.\r\n> ");
      tud_cdc_write_flush();
    } else if (buffer_pos > 0) {
      tud_cdc_write_str("\r\nSend 'repl' to enter maintenance mode.\r\n");
      tud_cdc_write_flush();
    }
    buffer_pos = 0;
  } else if (c >= ' ' && c <= '~') {
    if (buffer_pos < sizeof(command_buffer) - 1) {
      command_buffer[buffer_pos++] = (char) c;
    }
  }
}

//--------------------------------------------------------------------+
// Full REPL Task (The main command-line interface)
//--------------------------------------------------------------------+
void full_repl_task(void) {
  if (!tud_cdc_available()) return;

  static char command_buffer[64];
  static uint32_t buffer_pos = 0;

  int32_t c = tud_cdc_read_char();
  if (c == -1) return;

  if (c == '\r') {
    tud_cdc_write_str("\r\n");
    command_buffer[buffer_pos] = '\0';
    str_to_lower(command_buffer); // Convert to lowercase

    if (strcmp(command_buffer, "mount") == 0) {
      current_mode = MODE_MSC_WITH_BASIC_REPL;
      board_delay(10);
      tud_cdc_write_str("\r\n--- Switched to MSC Mode ---\r\n");
      tud_cdc_write_str("Drive is now mounted.\r\n");
      tud_cdc_write_flush();
    } else if (strcmp(command_buffer, "help") == 0) {
      tud_cdc_write_str("Full REPL Commands:\r\n");
      tud_cdc_write_str("  help   - Show this message\r\n");
      tud_cdc_write_str("  status - Show system status\r\n");
      tud_cdc_write_str("  mount  - Return to MSC mode\r\n");
    } else if (strcmp(command_buffer, "status") == 0) {
      tud_cdc_write_str("System Status: OK\r\n");
    } else if (buffer_pos > 0) {
      tud_cdc_write_str("Unknown command in REPL mode.\r\n");
    }

    buffer_pos = 0;
    tud_cdc_write_str("> ");
    tud_cdc_write_flush();
  } else if (c == '\b' || c == 127) {
    if (buffer_pos > 0) { buffer_pos--; tud_cdc_write_str("\b \b"); }
  } else if (c >= ' ' && c <= '~') {
    if (buffer_pos < sizeof(command_buffer) - 1) {
      command_buffer[buffer_pos++] = (char)c;
      tud_cdc_write_char((char)c);
    }
  }
  tud_cdc_write_flush();
}

//--------------------------------------------------------------------+
// Core USB Device callbacks
//--------------------------------------------------------------------+
void tud_mount_cb(void) {}
void tud_umount_cb(void) {}
void tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; }
void tud_resume_cb(void) {}
