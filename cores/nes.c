#include <stdarg.h>
#include <string.h>

#include "ff.h"

#include "utils.h"


extern uint32_t get_file_size(const char* fname);
extern void send_fbuf_data(int len);
extern void set_loading_state(int state);
extern bool core_running;
extern FIL* fcore;
extern BYTE* fbuf;

// Load a NES ROM
// return 0 if successful
int loadnes( const char* fname) {
  int r = 1;
  DEBUG("loadnes start\n");

  // check extension .nes
  char* p = strcasestr(fname, ".nes");
  if (p == NULL) {
    overlay_status("Only .nes supported");
    goto loadnes_end;
  }

  r = f_open(fcore, fname, FA_READ);
  if (r) {
    overlay_status("Cannot open file");
    goto loadnes_end;
  }
  unsigned int off = 0, br, total = 0;
  unsigned int size = get_file_size(fname);

  // load actual ROM
  set_loading_state(1);
  core_running = false;

  // Send rom content
  if ((r = f_lseek(fcore, off)) != FR_OK) {
    overlay_status("Seek failure");
    goto loadnes_snes_end;
  }


  do {
    if ((r = f_read(fcore, fbuf, 1024 /*BLOCK_SIZE*/, &br)) != FR_OK)
      break;
    // start rom loading command
    send_fbuf_data(br);
    taskYIELD();                // allow gamepad polling to run
    total += br;
    if ((total & 0xfff) == 0) {	// display progress every 4KB
      //              01234567890123456789012345678901
      overlay_status("%d/%dK                          ", total >> 10, size >> 10);
    }
  } while (br == 1024 /*BLOCK_SIZE*/);

  DEBUG("loadnes: %d bytes\n", total);
  overlay_status("Success");
  core_running = true;

  overlay(0);		// turn off OSD

loadnes_snes_end:
  set_loading_state(0);   // turn off game loading, this starts the core
  f_close(fcore);
loadnes_end:
  return r;
}
