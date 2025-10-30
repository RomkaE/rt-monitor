
// VT100 Common Commands
#define CLEARSCR          "\x1B[2J\x1B[;H"    // Clear screen.
#define CLEAREOL          "\x1B[K"            // Clear end of line.
#define CLEAREOS          "\x1B[J"            // Clear end of screen.
#define CLEARLCR          "\x1B[0K"           // Clear line cursor right.
#define CLEARLCL          "\x1B[1K"           // Clear line cursor left.
#define CLEARELN          "\x1B[2K"           // Clear entire line.
#define CLEARCDW          "\x1B[0J"           // Clear cursor down.
#define CLEARCUP          "\x1B[1J"           // Clear cursor up.
#define GOTOYX            "\x1B[%.2d;%.2dH"   // Set cursor to (y, x).
#define INSERTMOD         "\x1B[4h"           // Insert mode.
#define OVERWRITEMOD      "\x1B[4l"           // Overwrite mode.
#define DELAFCURSOR       "\x1B[K"            // Erase from cursor to end of line.
#define CRLF              "\r\n"              // Carriage Return + Line Feed.

// VT100 Cursor Commands

#define CURSON            "\x1B[?25h"         // Show cursor.
#define CURSOFF           "\x1B[?25l"         // Hide cursor.


// VT100 Character Commands
#define NORMAL            "\x1B[0m"           // Normal.
#define BOLD              "\x1B[1m"           // Bold.
#define UNDERLINE         "\x1B[4m"           // Underline.
#define BLINKING          "\x1B[5m"           // Blink.
#define INVVIDEO          "\x1B[7m"           // Inverse video.

// VT100 Color Commands
#define CL_BLACK          "\033[22;30m"       // Black.
#define CL_RED            "\033[22;31m"       // Red.
#define CL_GREEN          "\033[22;32m"       // Green.
#define CL_BROWN          "\033[22;33m"       // Brown.
#define CL_BLUE           "\033[22;34m"       // Blue.
#define CL_MAGENTA        "\033[22;35m"       // Magenta.
#define CL_CYAN           "\033[22;36m"       // Cyan.
#define CL_GRAY           "\033[22;37m"       // Gray.
#define CL_DARKGRAY       "\033[01;30m"       // Dark gray.
#define CL_LIGHTRED       "\033[01;31m"       // Light red.
#define CL_LIGHTGREEN     "\033[01;32m"       // Light green.
#define CL_YELLOW         "\033[01;33m"       // Yellow.
#define CL_LIGHTBLUE      "\033[01;34m"       // Light blue.
#define CL_LIGHTMAGENTA   "\033[01;35m"       // Light magenta.
#define CL_LIGHTCYAN      "\033[01;36m"       // Light cyan.
#define CL_WHITE          "\033[01;37m"       // White.
