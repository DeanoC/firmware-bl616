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

bool gba_bios_loaded;
bool gba_missing_bios_warned;

// check if gba_bios.bin is present in the root directory
// if not, warn user, if present, load it
void gba_load_bios() {
  if (gba_bios_loaded | gba_missing_bios_warned) return;

  DEBUG("gba_load_bios start\n");
  FILINFO fno;
  if (f_stat("usb:gba/gba_bios.bin", &fno) != FR_OK) {
    overlay_message("Cannot find /gba_bios.bin\n"
      "Using open source BIOS\n"
      "Expect low compatibility", 1);
    gba_missing_bios_warned = 1;
    return;
  }

  int r = 1;
  unsigned br;
  if (f_open(fcore, "usb:gba/gba_bios.bin", FA_READ) != FR_OK) {
    overlay_message("Cannot open /gba/gba_bios.bin", 1);
    return;
  }
  set_loading_state(4);
  do {
    if ((r = f_read(fcore, fbuf, 1024, &br)) != FR_OK)
      break;
    send_fbuf_data(br);
  } while (br == 1024);

  f_close(fcore);
  gba_bios_loaded = 1;
  DEBUG("gba_load_bios end\n");
}

int loadgba(const char* fname) {
  DEBUG("loadgba start\n");
  FRESULT r = -1;

  // check extension .gba
  char* p = strcasestr(fname, ".gba");
  if (p == NULL) {
    overlay_status("Only .gba supported");
    goto loadgba_end;
  }

  unsigned int size = get_file_size(fname);

  r = f_open(fcore, fname, FA_READ);
  if (r) {
    overlay_status("Cannot open file");
    goto loadgba_end;
  }
  unsigned int off = 0, br, total = 0;

  // load actual ROM
  set_loading_state(1);		// enable game loading, this resets GBA
  core_running = false;

  // Send rom content to gba
  if ((r = f_lseek(fcore, off)) != FR_OK) {
    overlay_status("Seek failure");
    goto loadgba_close;
  }
  // int detect = 0; // 1: past 'EEPR', 2: past 'FLAS', 3: past 'SRAM'
  // gba_backup_type = GBA_BACKUP_NONE;
  do {
    if ((r = f_read(fcore, fbuf, 1024, &br)) != FR_OK)
      break;

    send_fbuf_data(br);
    // TODO: do backup type detection

    total += br;
    if ((total & 0xffff) == 0) {	// display progress every 64KB
      //              01234567890123456789012345678901
      overlay_status("%d/%dK                          ", total >> 10, size >> 10);
    }
  } while (br == 1024);

  DEBUG("loadgba: %d bytes rom sent.\n", total);

  gba_load_bios();

  overlay_status("Success");
  core_running = true;

  overlay(0);		// turn off OSD

loadgba_close:
  set_loading_state(0);   // turn off game loading, this starts the core
  f_close(fcore);
loadgba_end:
  return r;
}