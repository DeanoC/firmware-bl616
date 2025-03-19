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


int loadmd(const char* fname) {
  DEBUG("loadmd start\n");
  FRESULT r = -1;

  // check extension .bin
  char* p = strcasestr(fname, ".bin");
  if (p == NULL) {
    overlay_status("Only .bin supported");
    goto loadmd_end;
  }

  r = f_open(fcore, fname, FA_READ);
  if (r) {
    overlay_status("Cannot open file");
    goto loadmd_end;
  }
  unsigned int off = 0, br, total = 0;
  unsigned int size = get_file_size(fname);

  // load actual ROM
  set_loading_state(1);		// enable game loading, this resets the core
  core_running = false;

  // Send rom content to core
  if ((r = f_lseek(fcore, off)) != FR_OK) {
    overlay_status("Seek failure");
    goto loadmd_close_file;
  }
  do {
    if ((r = f_read(fcore, fbuf, 1024, &br)) != FR_OK)
      break;
    send_fbuf_data(br);
    total += br;
    if ((total & 0xfff) == 0) {	// display progress every 4KB
      //              01234567890123456789012345678901
      overlay_status("%d/%dK                          ", total >> 10, size >> 10);
    }
  } while (br == 1024);

  DEBUG("loadmd: %d bytes\n", total);
  overlay_status("Success");
  core_running = true;

  overlay(0);		// turn off OSD

loadmd_close_file:
  set_loading_state(0);   // turn off game loading, this starts the core
  f_close(fcore);
loadmd_end:
  return r;
}
