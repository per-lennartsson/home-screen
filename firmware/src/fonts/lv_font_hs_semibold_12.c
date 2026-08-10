/*******************************************************************************
 * Size: 12 px
 * Bpp: 1
 * Opts: --font /Users/perlennartsson/git/home-screen/tools/fonts/src/Montserrat-SemiBold.ttf --size 12 --bpp 1 --no-kerning -r 0x20-0x7E,0xA0-0xFF --format lvgl --lv-font-name lv_font_hs_semibold_12 --lv-include lvgl.h -o /Users/perlennartsson/git/home-screen/firmware/src/fonts/lv_font_hs_semibold_12.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef LV_FONT_HS_SEMIBOLD_12
#define LV_FONT_HS_SEMIBOLD_12 1
#endif

#if LV_FONT_HS_SEMIBOLD_12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0xf3, 0xc0,

    /* U+0022 "\"" */
    0xb6, 0x80,

    /* U+0023 "#" */
    0x22, 0x26, 0xff, 0x24, 0x24, 0x24, 0xff, 0x44,
    0x44,

    /* U+0024 "$" */
    0x10, 0x21, 0xf7, 0xad, 0x1e, 0x1f, 0xf, 0x17,
    0xbd, 0xf0, 0x80,

    /* U+0025 "%" */
    0x61, 0x24, 0xc9, 0x22, 0x50, 0x6d, 0x82, 0x91,
    0x24, 0xc9, 0x21, 0x80,

    /* U+0026 "&" */
    0x38, 0x6c, 0x6c, 0x38, 0x30, 0xeb, 0xc6, 0xc6,
    0x7f,

    /* U+0027 "'" */
    0xe0,

    /* U+0028 "(" */
    0x2d, 0x6d, 0xb6, 0xd9, 0xb2,

    /* U+0029 ")" */
    0x9b, 0x36, 0xdb, 0x6f, 0x68,

    /* U+002A "*" */
    0x27, 0xcd, 0xf2, 0x0,

    /* U+002B "+" */
    0x30, 0xc3, 0x3f, 0x30, 0xc0,

    /* U+002C "," */
    0xfe,

    /* U+002D "-" */
    0xe0,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x18, 0x84, 0x62, 0x11, 0x88, 0x46, 0x21, 0x0,

    /* U+0030 "0" */
    0x38, 0xdb, 0x1e, 0x3c, 0x78, 0xf1, 0xb6, 0x38,

    /* U+0031 "1" */
    0xf3, 0x33, 0x33, 0x33, 0x30,

    /* U+0032 "2" */
    0x7a, 0x30, 0xc3, 0x18, 0xe7, 0x18, 0xfc,

    /* U+0033 "3" */
    0xfc, 0x63, 0x8, 0x38, 0x30, 0xe3, 0xf8,

    /* U+0034 "4" */
    0x18, 0x18, 0x30, 0x20, 0x6c, 0xcc, 0xff, 0xc,
    0xc,

    /* U+0035 "5" */
    0x7c, 0x81, 0x2, 0x7, 0xc0, 0xc1, 0x83, 0x7c,

    /* U+0036 "6" */
    0x3c, 0xc3, 0x7, 0xee, 0x78, 0xf1, 0xa3, 0x3c,

    /* U+0037 "7" */
    0xff, 0x8b, 0x30, 0x60, 0x83, 0x6, 0x8, 0x30,

    /* U+0038 "8" */
    0x7d, 0x8f, 0x1e, 0x33, 0x98, 0xf1, 0xe3, 0x7c,

    /* U+0039 "9" */
    0x79, 0x8b, 0x1e, 0x37, 0xe0, 0xc1, 0x86, 0x78,

    /* U+003A ":" */
    0xf0, 0x3c,

    /* U+003B ";" */
    0xf0, 0x3f, 0x80,

    /* U+003C "<" */
    0x4, 0x77, 0x20, 0x70, 0x70,

    /* U+003D "=" */
    0xfc, 0x0, 0x3f,

    /* U+003E ">" */
    0x3, 0x3, 0x83, 0x3b, 0x0,

    /* U+003F "?" */
    0x7a, 0x30, 0xc7, 0x18, 0xc0, 0xc, 0x30,

    /* U+0040 "@" */
    0x1f, 0x4, 0x11, 0x1, 0x6f, 0xdb, 0x1b, 0x63,
    0x6c, 0x6d, 0x8d, 0xdf, 0xc8, 0x0, 0x84, 0xf,
    0x80,

    /* U+0041 "A" */
    0x18, 0xe, 0x5, 0x6, 0xc2, 0x63, 0x19, 0xfc,
    0x82, 0xc1, 0x80,

    /* U+0042 "B" */
    0xfc, 0xc6, 0xc6, 0xc6, 0xfc, 0xc3, 0xc3, 0xc3,
    0xfe,

    /* U+0043 "C" */
    0x3e, 0x61, 0xe0, 0xc0, 0xc0, 0xc0, 0xc0, 0x61,
    0x3e,

    /* U+0044 "D" */
    0xfc, 0xc6, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc6,
    0xfc,

    /* U+0045 "E" */
    0xff, 0x83, 0x6, 0xf, 0xd8, 0x30, 0x60, 0xfe,

    /* U+0046 "F" */
    0xff, 0x83, 0x6, 0xf, 0xd8, 0x30, 0x60, 0xc0,

    /* U+0047 "G" */
    0x3e, 0x63, 0xc0, 0xc0, 0xc3, 0xc3, 0xc3, 0x63,
    0x3e,

    /* U+0048 "H" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3,
    0xc3,

    /* U+0049 "I" */
    0xff, 0xff, 0xc0,

    /* U+004A "J" */
    0x7c, 0x30, 0xc3, 0xc, 0x30, 0xd3, 0x78,

    /* U+004B "K" */
    0xc2, 0xc6, 0xcc, 0xd8, 0xf8, 0xe8, 0xcc, 0xc6,
    0xc3,

    /* U+004C "L" */
    0xc3, 0xc, 0x30, 0xc3, 0xc, 0x30, 0xfc,

    /* U+004D "M" */
    0xc1, 0xe0, 0xf8, 0xfc, 0x7d, 0x5e, 0xaf, 0x27,
    0x93, 0xc1, 0x80,

    /* U+004E "N" */
    0xc3, 0xe3, 0xe3, 0xf3, 0xdb, 0xcf, 0xc7, 0xc7,
    0xc3,

    /* U+004F "O" */
    0x3e, 0x31, 0xb0, 0x78, 0x3c, 0x1e, 0xf, 0x6,
    0xc6, 0x3e, 0x0,

    /* U+0050 "P" */
    0xfd, 0x8f, 0x1e, 0x3c, 0x7f, 0xb0, 0x60, 0xc0,

    /* U+0051 "Q" */
    0x3e, 0x31, 0xb0, 0x78, 0x3c, 0x1e, 0xf, 0xe,
    0xee, 0x3e, 0x6, 0x41, 0xe0,

    /* U+0052 "R" */
    0xfd, 0x8f, 0x1e, 0x3c, 0x7f, 0x32, 0x66, 0xc6,

    /* U+0053 "S" */
    0x7d, 0x8b, 0x7, 0x87, 0xc3, 0xc1, 0xc3, 0xfc,

    /* U+0054 "T" */
    0xfe, 0x30, 0x60, 0xc1, 0x83, 0x6, 0xc, 0x18,

    /* U+0055 "U" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66,
    0x3c,

    /* U+0056 "V" */
    0xc1, 0xa0, 0x98, 0xcc, 0x42, 0x61, 0xb0, 0x50,
    0x38, 0x18, 0x0,

    /* U+0057 "W" */
    0xc3, 0xa, 0x18, 0xd9, 0xc6, 0xcf, 0x22, 0x49,
    0x1e, 0x58, 0xf3, 0x83, 0x1c, 0x18, 0x60,

    /* U+0058 "X" */
    0x43, 0x66, 0x34, 0x1c, 0x18, 0x3c, 0x36, 0x66,
    0xc3,

    /* U+0059 "Y" */
    0xc3, 0x42, 0x66, 0x34, 0x3c, 0x18, 0x18, 0x18,
    0x18,

    /* U+005A "Z" */
    0xfe, 0xc, 0x30, 0xc1, 0x6, 0x18, 0x60, 0xfe,

    /* U+005B "[" */
    0xfb, 0x6d, 0xb6, 0xdb, 0x6e,

    /* U+005C "\\" */
    0x84, 0x30, 0x84, 0x30, 0x84, 0x30, 0x84, 0x30,

    /* U+005D "]" */
    0xed, 0xb6, 0xdb, 0x6d, 0xbe,

    /* U+005E "^" */
    0x21, 0x94, 0xac, 0xc4,

    /* U+005F "_" */
    0xfc,

    /* U+0060 "`" */
    0x42,

    /* U+0061 "a" */
    0x78, 0x30, 0x5f, 0xc7, 0x37, 0x40,

    /* U+0062 "b" */
    0xc1, 0x83, 0x7, 0xee, 0xf8, 0xf1, 0xe3, 0xef,
    0xf8,

    /* U+0063 "c" */
    0x3c, 0xcb, 0x6, 0xc, 0xc, 0x8f, 0x0,

    /* U+0064 "d" */
    0x6, 0xc, 0x1b, 0xfe, 0xf8, 0xf1, 0xe3, 0xee,
    0xfc,

    /* U+0065 "e" */
    0x38, 0x8b, 0x1f, 0xfc, 0x8, 0x8f, 0x0,

    /* U+0066 "f" */
    0x36, 0x6f, 0x66, 0x66, 0x66,

    /* U+0067 "g" */
    0x7f, 0xdf, 0x1e, 0x3c, 0x7d, 0xdf, 0x83, 0x4e,
    0xf8,

    /* U+0068 "h" */
    0xc1, 0x83, 0x7, 0xee, 0x78, 0xf1, 0xe3, 0xc7,
    0x8c,

    /* U+0069 "i" */
    0xf3, 0xff, 0xf0,

    /* U+006A "j" */
    0x33, 0x3, 0x33, 0x33, 0x33, 0x33, 0xe0,

    /* U+006B "k" */
    0xc1, 0x83, 0x6, 0x2c, 0xdb, 0x3e, 0x7c, 0xcd,
    0x8c,

    /* U+006C "l" */
    0xff, 0xff, 0xf0,

    /* U+006D "m" */
    0xfb, 0xb3, 0x3c, 0xcf, 0x33, 0xcc, 0xf3, 0x3c,
    0xcc,

    /* U+006E "n" */
    0xfd, 0xcf, 0x1e, 0x3c, 0x78, 0xf1, 0x80,

    /* U+006F "o" */
    0x38, 0xdb, 0x1e, 0x3c, 0x6d, 0x8e, 0x0,

    /* U+0070 "p" */
    0xfd, 0xdf, 0x1e, 0x3c, 0x7d, 0xff, 0x60, 0xc1,
    0x80,

    /* U+0071 "q" */
    0x7f, 0xdf, 0x1e, 0x3c, 0x7d, 0xdf, 0x83, 0x6,
    0xc,

    /* U+0072 "r" */
    0xfc, 0xcc, 0xcc, 0xc0,

    /* U+0073 "s" */
    0x7f, 0xe, 0x3f, 0x1e, 0x3f, 0x80,

    /* U+0074 "t" */
    0x66, 0xf6, 0x66, 0x66, 0x30,

    /* U+0075 "u" */
    0xcf, 0x3c, 0xf3, 0xcf, 0x37, 0xc0,

    /* U+0076 "v" */
    0xc6, 0x89, 0x13, 0x62, 0x87, 0xc, 0x0,

    /* U+0077 "w" */
    0xc4, 0x69, 0xc9, 0x29, 0x35, 0x63, 0xb8, 0x63,
    0xc, 0x60,

    /* U+0078 "x" */
    0x44, 0xd8, 0xe0, 0x83, 0x8d, 0xb1, 0x0,

    /* U+0079 "y" */
    0xc7, 0x89, 0x13, 0x62, 0x85, 0xe, 0x8, 0x31,
    0xc0,

    /* U+007A "z" */
    0xf8, 0xcc, 0x46, 0x63, 0xe0,

    /* U+007B "{" */
    0x36, 0x66, 0x66, 0xc6, 0x66, 0x66, 0x30,

    /* U+007C "|" */
    0xff, 0xff, 0xff, 0xc0,

    /* U+007D "}" */
    0xc6, 0x66, 0x66, 0x36, 0x66, 0x66, 0xc0,

    /* U+007E "~" */
    0xe6, 0x70,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xf3, 0xff, 0xc0,

    /* U+00A2 "¢" */
    0x10, 0x47, 0xf5, 0xd3, 0x4d, 0x5f, 0x10, 0x40,

    /* U+00A3 "£" */
    0x1e, 0x61, 0x83, 0xf, 0xcc, 0x18, 0x30, 0xfe,

    /* U+00A4 "¤" */
    0x0, 0x7e, 0x66, 0x42, 0x42, 0x42, 0x66, 0x7e,
    0x0,

    /* U+00A5 "¥" */
    0x40, 0x98, 0x63, 0x30, 0x48, 0x1e, 0xf, 0xc0,
    0xc0, 0xfc, 0xc, 0x0,

    /* U+00A6 "¦" */
    0xff, 0x0, 0x3f, 0xc0,

    /* U+00A7 "§" */
    0x7f, 0xc, 0x1e, 0xcf, 0x37, 0x86, 0x1b, 0xc0,

    /* U+00A8 "¨" */
    0xf0,

    /* U+00A9 "©" */
    0x3e, 0x31, 0xb7, 0x74, 0x9a, 0xd, 0x27, 0x76,
    0xc6, 0x3e, 0x0,

    /* U+00AA "ª" */
    0xf1, 0xff,

    /* U+00AB "«" */
    0x2d, 0xad, 0x1a, 0x2c,

    /* U+00AC "¬" */
    0xfc, 0x30, 0xc3,

    /* U+00AE "®" */
    0x3e, 0x31, 0xbf, 0x74, 0x9b, 0xcd, 0x47, 0x96,
    0xc6, 0x3e, 0x0,

    /* U+00AF "¯" */
    0xf0,

    /* U+00B0 "°" */
    0x69, 0x96,

    /* U+00B1 "±" */
    0x30, 0xcf, 0xcc, 0x30, 0x0, 0x3f,

    /* U+00B2 "²" */
    0xf1, 0x24, 0xf0,

    /* U+00B3 "³" */
    0x78, 0x8e, 0x17, 0x0,

    /* U+00B4 "´" */
    0x50,

    /* U+00B5 "µ" */
    0xc7, 0x8f, 0x1e, 0x3c, 0x79, 0xff, 0xe0, 0xc1,
    0x80,

    /* U+00B6 "¶" */
    0x7e, 0xe5, 0xcb, 0x91, 0x22, 0x44, 0x89, 0x12,
    0x24, 0x48,

    /* U+00B7 "·" */
    0xf0,

    /* U+00B8 "¸" */
    0x47, 0x80,

    /* U+00B9 "¹" */
    0xc9, 0x2e,

    /* U+00BA "º" */
    0x69, 0x96,

    /* U+00BB "»" */
    0x59, 0xa2, 0x5a, 0x58,

    /* U+00BC "¼" */
    0xc1, 0x8, 0x41, 0x8, 0x22, 0x4, 0xc9, 0xd3,
    0x4, 0x50, 0x9f, 0x20, 0x40,

    /* U+00BD "½" */
    0xc1, 0x8, 0x41, 0x8, 0x22, 0xe, 0xdc, 0x10,
    0xc4, 0x11, 0x8c, 0x23, 0xe0,

    /* U+00BE "¾" */
    0xf0, 0x82, 0x10, 0x23, 0x1, 0x20, 0x14, 0x4e,
    0x4c, 0x8, 0xa1, 0x1f, 0x10, 0x20,

    /* U+00BF "¿" */
    0x30, 0xc0, 0xc, 0x63, 0x8c, 0x31, 0x78,

    /* U+00C0 "À" */
    0x30, 0xc, 0x0, 0x3, 0x1, 0xc0, 0xa0, 0xd8,
    0x4c, 0x63, 0x3f, 0x90, 0x58, 0x30,

    /* U+00C1 "Á" */
    0x6, 0x4, 0x0, 0x3, 0x1, 0xc0, 0xa0, 0xd8,
    0x4c, 0x63, 0x3f, 0x90, 0x58, 0x30,

    /* U+00C2 "Â" */
    0x8, 0xa, 0x0, 0x3, 0x1, 0xc0, 0xa0, 0xd8,
    0x4c, 0x63, 0x3f, 0x90, 0x58, 0x30,

    /* U+00C3 "Ã" */
    0x34, 0x16, 0x0, 0x3, 0x1, 0xc1, 0xa0, 0xd0,
    0x4c, 0x62, 0x3f, 0xb0, 0x58, 0x30,

    /* U+00C4 "Ä" */
    0x1e, 0x0, 0x0, 0xc0, 0x30, 0x1e, 0x4, 0x83,
    0x30, 0xcc, 0x7f, 0x98, 0x64, 0x8,

    /* U+00C5 "Å" */
    0x8, 0xa, 0x5, 0x1, 0x0, 0x80, 0xe0, 0x50,
    0x6c, 0x22, 0x31, 0x9f, 0xc8, 0x2c, 0x18,

    /* U+00C6 "Æ" */
    0x7, 0xf0, 0xf0, 0xb, 0x1, 0xb0, 0x13, 0xf3,
    0x30, 0x7f, 0x6, 0x30, 0xc3, 0xf0,

    /* U+00C7 "Ç" */
    0x3e, 0x62, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0x62,
    0x3e, 0x8, 0x8, 0x18,

    /* U+00C8 "È" */
    0x60, 0x60, 0x7, 0xfc, 0x18, 0x30, 0x7e, 0xc1,
    0x83, 0x7, 0xf0,

    /* U+00C9 "É" */
    0xc, 0x30, 0x7, 0xfc, 0x18, 0x30, 0x7e, 0xc1,
    0x83, 0x7, 0xf0,

    /* U+00CA "Ê" */
    0x10, 0x50, 0x7, 0xfc, 0x18, 0x30, 0x7e, 0xc1,
    0x83, 0x7, 0xf0,

    /* U+00CB "Ë" */
    0x78, 0xf, 0xf0, 0xc3, 0xf, 0xf0, 0xc3, 0xf,
    0xc0,

    /* U+00CC "Ì" */
    0xc3, 0x3, 0x33, 0x33, 0x33, 0x33,

    /* U+00CD "Í" */
    0x3c, 0xc, 0xcc, 0xcc, 0xcc, 0xcc,

    /* U+00CE "Î" */
    0x31, 0xe0, 0xc, 0x30, 0xc3, 0xc, 0x30, 0xc3,
    0xc,

    /* U+00CF "Ï" */
    0xf0, 0x66, 0x66, 0x66, 0x66, 0x60,

    /* U+00D0 "Ð" */
    0x7e, 0x31, 0x98, 0x6c, 0x3f, 0x9b, 0xd, 0x86,
    0xc6, 0x7e, 0x0,

    /* U+00D1 "Ñ" */
    0x34, 0x2c, 0x0, 0xc3, 0xe3, 0xe3, 0xf3, 0xdb,
    0xcf, 0xc7, 0xc7, 0xc3,

    /* U+00D2 "Ò" */
    0x30, 0x4, 0x0, 0x7, 0xc6, 0x36, 0xf, 0x7,
    0x83, 0xc1, 0xe0, 0xd8, 0xc7, 0xc0,

    /* U+00D3 "Ó" */
    0x6, 0x4, 0x0, 0x7, 0xc6, 0x36, 0xf, 0x7,
    0x83, 0xc1, 0xe0, 0xd8, 0xc7, 0xc0,

    /* U+00D4 "Ô" */
    0x8, 0xa, 0x0, 0x7, 0xc6, 0x36, 0xf, 0x7,
    0x83, 0xc1, 0xe0, 0xd8, 0xc7, 0xc0,

    /* U+00D5 "Õ" */
    0x1c, 0x16, 0x0, 0x7, 0xc6, 0x36, 0xf, 0x7,
    0x83, 0xc1, 0xe0, 0xd8, 0xc7, 0xc0,

    /* U+00D6 "Ö" */
    0x36, 0x0, 0xf, 0x8c, 0x6c, 0x1e, 0xf, 0x7,
    0x83, 0xc1, 0xb1, 0x8f, 0x80,

    /* U+00D7 "×" */
    0xdb, 0x88, 0xac, 0x80,

    /* U+00D8 "Ø" */
    0x2, 0x1f, 0x19, 0xd8, 0xbc, 0xde, 0x4f, 0x67,
    0xa3, 0x73, 0x1f, 0x8, 0x0,

    /* U+00D9 "Ù" */
    0x20, 0x10, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0x66, 0x3c,

    /* U+00DA "Ú" */
    0x4, 0x8, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0x66, 0x3c,

    /* U+00DB "Û" */
    0x18, 0x3c, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0x66, 0x3c,

    /* U+00DC "Ü" */
    0x3c, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0x66, 0x3c,

    /* U+00DD "Ý" */
    0xc, 0x10, 0xc3, 0x42, 0x66, 0x34, 0x3c, 0x18,
    0x18, 0x18, 0x18,

    /* U+00DE "Þ" */
    0xc1, 0xfb, 0x1e, 0x3c, 0x78, 0xff, 0x60, 0xc0,

    /* U+00DF "ß" */
    0x79, 0x9b, 0x36, 0x6c, 0xdb, 0x31, 0xe3, 0xc7,
    0xb8,

    /* U+00E0 "à" */
    0x60, 0x40, 0x1e, 0xc, 0x17, 0xf1, 0xcd, 0xd0,

    /* U+00E1 "á" */
    0x8, 0x40, 0x1e, 0xc, 0x17, 0xf1, 0xcd, 0xd0,

    /* U+00E2 "â" */
    0x31, 0xa0, 0x1e, 0xc, 0x17, 0xf1, 0xcd, 0xd0,

    /* U+00E3 "ã" */
    0x29, 0x60, 0x1e, 0xc, 0x37, 0xf3, 0xcd, 0xf0,

    /* U+00E4 "ä" */
    0x78, 0x0, 0x1e, 0xc, 0x37, 0xf3, 0xcd, 0xf0,

    /* U+00E5 "å" */
    0x31, 0x23, 0x0, 0x79, 0x30, 0x5f, 0xc7, 0x37,
    0xc0,

    /* U+00E6 "æ" */
    0xff, 0x81, 0x88, 0x31, 0xbf, 0xfc, 0xc1, 0x98,
    0x9d, 0xf0,

    /* U+00E7 "ç" */
    0x39, 0x9c, 0x30, 0xc1, 0x93, 0x84, 0x10, 0xc0,

    /* U+00E8 "è" */
    0x20, 0x20, 0x1, 0xc4, 0x58, 0xff, 0xe0, 0x44,
    0x78,

    /* U+00E9 "é" */
    0xc, 0x20, 0x1, 0xc4, 0x58, 0xff, 0xe0, 0x44,
    0x78,

    /* U+00EA "ê" */
    0x38, 0xd8, 0x1, 0xc4, 0x58, 0xff, 0xe0, 0x44,
    0x78,

    /* U+00EB "ë" */
    0x78, 0x0, 0x1, 0xc4, 0xd0, 0xff, 0xc0, 0x44,
    0x78,

    /* U+00EC "ì" */
    0x63, 0x3, 0x33, 0x33, 0x33,

    /* U+00ED "í" */
    0x6c, 0xc, 0xcc, 0xcc, 0xcc,

    /* U+00EE "î" */
    0x69, 0x6, 0x66, 0x66, 0x66,

    /* U+00EF "ï" */
    0xf0, 0x6, 0x66, 0x66, 0x66,

    /* U+00F0 "ð" */
    0x20, 0x39, 0xe0, 0x27, 0xf9, 0xf1, 0xe3, 0xc4,
    0xf0,

    /* U+00F1 "ñ" */
    0x34, 0xb0, 0x7, 0xee, 0x78, 0xf1, 0xe3, 0xc7,
    0x8c,

    /* U+00F2 "ò" */
    0x20, 0x20, 0x1, 0xc6, 0xd8, 0xf1, 0xe3, 0x6c,
    0x70,

    /* U+00F3 "ó" */
    0x8, 0x20, 0x1, 0xc6, 0xd8, 0xf1, 0xe3, 0x6c,
    0x70,

    /* U+00F4 "ô" */
    0x10, 0x50, 0x1, 0xc6, 0xd8, 0xf1, 0xe3, 0x6c,
    0x70,

    /* U+00F5 "õ" */
    0x38, 0x70, 0x1, 0xc6, 0xd8, 0xf1, 0xe3, 0x6c,
    0x70,

    /* U+00F6 "ö" */
    0x2c, 0x0, 0x1, 0xc6, 0x58, 0x70, 0xe1, 0x64,
    0x70,

    /* U+00F7 "÷" */
    0x30, 0xc0, 0x3f, 0x0, 0xc3, 0x0,

    /* U+00F8 "ø" */
    0xc, 0x71, 0xb6, 0xbd, 0x7e, 0xdb, 0x1c, 0x40,

    /* U+00F9 "ù" */
    0x60, 0x80, 0x33, 0xcf, 0x3c, 0xf3, 0xcd, 0xf0,

    /* U+00FA "ú" */
    0x18, 0x40, 0x33, 0xcf, 0x3c, 0xf3, 0xcd, 0xf0,

    /* U+00FB "û" */
    0x31, 0xe0, 0x33, 0xcf, 0x3c, 0xf3, 0xcd, 0xf0,

    /* U+00FC "ü" */
    0x78, 0x0, 0x33, 0xcf, 0x3c, 0xf3, 0xcd, 0xf0,

    /* U+00FD "ý" */
    0x18, 0x20, 0x6, 0x3c, 0x48, 0x9b, 0x14, 0x28,
    0x70, 0x41, 0x8e, 0x0,

    /* U+00FE "þ" */
    0xc1, 0x83, 0x7, 0xee, 0xf8, 0xf1, 0xe3, 0xef,
    0xfb, 0x6, 0xc, 0x0,

    /* U+00FF "ÿ" */
    0x3c, 0x0, 0x0, 0x43, 0x62, 0x26, 0x24, 0x3c,
    0x1c, 0x18, 0x8, 0x10, 0xf0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 53, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 53, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 79, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 6, .adv_w = 137, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 15, .adv_w = 121, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 26, .adv_w = 165, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 38, .adv_w = 136, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 47, .adv_w = 42, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 48, .adv_w = 67, .box_w = 3, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 53, .adv_w = 67, .box_w = 3, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 58, .adv_w = 80, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 62, .adv_w = 113, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 67, .adv_w = 47, .box_w = 2, .box_h = 4, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 68, .adv_w = 74, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 69, .adv_w = 47, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 71, .box_w = 5, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 78, .adv_w = 129, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 86, .adv_w = 73, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 91, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 98, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 105, .adv_w = 130, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 114, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 122, .adv_w = 120, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 117, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 138, .adv_w = 125, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 146, .adv_w = 120, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 154, .adv_w = 47, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 47, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 159, .adv_w = 113, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 164, .adv_w = 113, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 167, .adv_w = 113, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 172, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 179, .adv_w = 199, .box_w = 11, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 196, .adv_w = 144, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 207, .adv_w = 146, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 216, .adv_w = 138, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 225, .adv_w = 159, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 129, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 242, .adv_w = 122, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 250, .adv_w = 148, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 259, .adv_w = 156, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 268, .adv_w = 61, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 271, .adv_w = 101, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 278, .adv_w = 140, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 287, .adv_w = 115, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 294, .adv_w = 183, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 305, .adv_w = 156, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 314, .adv_w = 162, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 325, .adv_w = 139, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 333, .adv_w = 162, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 346, .adv_w = 140, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 354, .adv_w = 121, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 362, .adv_w = 116, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 151, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 379, .adv_w = 140, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 390, .adv_w = 220, .box_w = 13, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 405, .adv_w = 133, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 414, .adv_w = 127, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 423, .adv_w = 127, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 431, .adv_w = 67, .box_w = 3, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 436, .adv_w = 71, .box_w = 5, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 444, .adv_w = 67, .box_w = 3, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 449, .adv_w = 113, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 453, .adv_w = 96, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 454, .adv_w = 115, .box_w = 4, .box_h = 2, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 455, .adv_w = 117, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 461, .adv_w = 132, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 470, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 477, .adv_w = 132, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 486, .adv_w = 119, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 493, .adv_w = 71, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 498, .adv_w = 133, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 507, .adv_w = 132, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 516, .adv_w = 55, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 519, .adv_w = 57, .box_w = 4, .box_h = 13, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 526, .adv_w = 122, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 55, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 538, .adv_w = 202, .box_w = 10, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 547, .adv_w = 132, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 554, .adv_w = 124, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 561, .adv_w = 132, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 570, .adv_w = 132, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 579, .adv_w = 81, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 583, .adv_w = 99, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 589, .adv_w = 81, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 594, .adv_w = 131, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 600, .adv_w = 111, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 607, .adv_w = 176, .box_w = 11, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 617, .adv_w = 110, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 624, .adv_w = 111, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 633, .adv_w = 102, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 638, .adv_w = 71, .box_w = 4, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 645, .adv_w = 58, .box_w = 2, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 649, .adv_w = 71, .box_w = 4, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 656, .adv_w = 113, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 658, .adv_w = 53, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 659, .adv_w = 53, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 662, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 670, .adv_w = 126, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 678, .adv_w = 134, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 687, .adv_w = 138, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 699, .adv_w = 58, .box_w = 2, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 703, .adv_w = 98, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 711, .adv_w = 115, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 712, .adv_w = 152, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 723, .adv_w = 78, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 725, .adv_w = 103, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 729, .adv_w = 113, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 732, .adv_w = 152, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 743, .adv_w = 115, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 744, .adv_w = 80, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 746, .adv_w = 113, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 752, .adv_w = 83, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 755, .adv_w = 83, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 759, .adv_w = 115, .box_w = 3, .box_h = 2, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 760, .adv_w = 132, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 769, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 779, .adv_w = 55, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 780, .adv_w = 115, .box_w = 3, .box_h = 3, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 782, .adv_w = 83, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 784, .adv_w = 81, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 786, .adv_w = 103, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 790, .adv_w = 199, .box_w = 11, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 803, .adv_w = 199, .box_w = 11, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 816, .adv_w = 199, .box_w = 12, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 830, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 837, .adv_w = 144, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 851, .adv_w = 144, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 865, .adv_w = 144, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 879, .adv_w = 144, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 893, .adv_w = 144, .box_w = 10, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 907, .adv_w = 144, .box_w = 9, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 922, .adv_w = 204, .box_w = 12, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 936, .adv_w = 138, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 948, .adv_w = 129, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 959, .adv_w = 129, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 970, .adv_w = 129, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 981, .adv_w = 129, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 990, .adv_w = 61, .box_w = 4, .box_h = 12, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 996, .adv_w = 61, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1002, .adv_w = 61, .box_w = 6, .box_h = 12, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1011, .adv_w = 61, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1017, .adv_w = 161, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1028, .adv_w = 156, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1040, .adv_w = 162, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1054, .adv_w = 162, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1068, .adv_w = 162, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1082, .adv_w = 162, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1096, .adv_w = 162, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1109, .adv_w = 113, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1113, .adv_w = 162, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1126, .adv_w = 151, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1138, .adv_w = 151, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1150, .adv_w = 151, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1162, .adv_w = 151, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1173, .adv_w = 127, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1184, .adv_w = 139, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1192, .adv_w = 131, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1201, .adv_w = 117, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1209, .adv_w = 117, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1217, .adv_w = 117, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1225, .adv_w = 117, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1233, .adv_w = 117, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1241, .adv_w = 117, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1250, .adv_w = 191, .box_w = 11, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1260, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1268, .adv_w = 119, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1277, .adv_w = 119, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1286, .adv_w = 119, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1295, .adv_w = 119, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1304, .adv_w = 55, .box_w = 4, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1309, .adv_w = 55, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1314, .adv_w = 55, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1319, .adv_w = 55, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1324, .adv_w = 116, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1333, .adv_w = 132, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1342, .adv_w = 124, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1351, .adv_w = 124, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1360, .adv_w = 124, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1369, .adv_w = 124, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1378, .adv_w = 124, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1387, .adv_w = 113, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 1393, .adv_w = 124, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1401, .adv_w = 131, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1409, .adv_w = 131, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1417, .adv_w = 131, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1425, .adv_w = 131, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1433, .adv_w = 111, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1445, .adv_w = 132, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1457, .adv_w = 111, .box_w = 8, .box_h = 13, .ofs_x = -1, .ofs_y = -3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 160, .range_length = 13, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 174, .range_length = 82, .glyph_id_start = 109,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 3,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lv_font_hs_semibold_12 = {
#else
lv_font_t lv_font_hs_semibold_12 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 16,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if LV_FONT_HS_SEMIBOLD_12*/

