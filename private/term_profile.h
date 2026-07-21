
#ifndef RTMON_TERM_PROFILE_H_
#define RTMON_TERM_PROFILE_H_

#include "terminal.h"

/*
 * Terminal capability profile.
 *
 * terminal.h lists the full VT100 vocabulary, but not every sink understands
 * all of it. The SEGGER RTT Viewer in particular accepts only a small subset -
 * see the RTT_CTRL_* block in SEGGER_RTT.h:
 *
 *   "\x1B[0m"       reset
 *   "\x1B[2J"       clear screen AND move the cursor to the top left
 *   "\x1B[2;3Xm"    text colour, normal intensity
 *   "\x1B[1;3Xm"    text colour, bright
 *   "\x1B[24;4Xm"   background colour
 *
 * Everything else is mis-parsed. Single-parameter attributes such as BOLD
 * ("\x1B[1m") come out invisible - the text reaches the buffer and can still be
 * recovered by selecting it with the mouse, but nothing is drawn. Cursor
 * addressing (GOTOYX) and the partial erases (CLEAREOL, CLEAREOS) are not
 * understood at all.
 *
 * So rtmon.c speaks in intent - begin a frame, emphasise, end a frame - and the
 * profile below turns that into whatever the active sink actually supports.
 * This is resolved at build time; the sink is fixed for a given firmware image.
 *
 * The default is ANSI, which is the behaviour every existing port had before
 * this header existed. Only a project that needs otherwise has to set
 * RTMON_TERM_PROFILE in its rtmon_config.h.
 */

#define RTMON_TERM_ANSI          0
#define RTMON_TERM_SEGGER_RTT    1

#ifndef RTMON_TERM_PROFILE
  #define RTMON_TERM_PROFILE     RTMON_TERM_ANSI
#endif

#if RTMON_TERM_PROFILE == RTMON_TERM_SEGGER_RTT

  /*
   * No screen management at all - the frame is simply appended and scrolls.
   *
   * This is not laziness, it is forced. The Viewer's virtual terminals (the
   * 0xFF,'0'+id switch) exist only on up-buffer 0, so a monitor that wants its
   * own tab has to share that buffer with whatever else logs there. And the
   * one erase the Viewer does understand, "\x1B[2J", is a display-level
   * command: it is NOT scoped to the current virtual terminal and wipes the
   * other tabs' content along with ours. Using it here erased the application
   * log once a second.
   *
   * Cursor addressing (GOTOYX) and the partial erases would have allowed a
   * redraw in place without clearing, but the Viewer does not parse them.
   * So on RTT the table scrolls. Use the ANSI profile on a real terminal to
   * get the in-place dashboard back.
   */
  #define TERM_LINE_PREFIX       ""
  #define TERM_FRAME_BEGIN       ""
  #define TERM_FRAME_END         ""
  /* RTT_CTRL_TEXT_BRIGHT_WHITE, the two-parameter form the Viewer parses. */
  #define TERM_EMPH_ON           "\x1B[1;37m"
  /* RTT_CTRL_RESET. */
  #define TERM_EMPH_OFF          "\x1B[0m"

#elif RTMON_TERM_PROFILE == RTMON_TERM_ANSI

  /* A real terminal can erase per line and per frame tail, which lets the table
   * be redrawn in place without the flicker of a full clear. */
  #define TERM_LINE_PREFIX       CLEAREOL
  #define TERM_FRAME_BEGIN       "\x1B[H"
  #define TERM_FRAME_END         CLEAREOS
  #define TERM_EMPH_ON           BOLD
  #define TERM_EMPH_OFF          NORMAL

#else
  #error "Unsupported RTMON_TERM_PROFILE"
#endif

/* Both sinks tolerate CR+LF, and minicom on a raw tty needs the CR. */
#define TERM_EOL                 CRLF

#endif /* RTMON_TERM_PROFILE_H_ */
