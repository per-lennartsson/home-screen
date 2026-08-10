/*******************************************************************************
 * Size: 12 px
 * Bpp: 1
 * Opts: --font /Users/perlennartsson/git/home-screen/tools/fonts/src/Montserrat-Regular.ttf --size 12 --bpp 1 --no-kerning -r 0x20-0x7E,0xA0-0xFF --format lvgl --lv-font-name lv_font_hs_regular_12 --lv-include lvgl.h -o /Users/perlennartsson/git/home-screen/firmware/src/fonts/lv_font_hs_regular_12.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef LV_FONT_HS_REGULAR_12
#define LV_FONT_HS_REGULAR_12 1
#endif

#if LV_FONT_HS_REGULAR_12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xf9,

    /* U+0022 "\"" */
    0xb6, 0x80,

    /* U+0023 "#" */
    0x22, 0x24, 0x7f, 0x24, 0x24, 0xff, 0x24, 0x24,

    /* U+0024 "$" */
    0x20, 0x87, 0xa8, 0xa3, 0x83, 0x89, 0xa7, 0xe2,
    0x8,

    /* U+0025 "%" */
    0x62, 0x4a, 0x25, 0xd, 0x61, 0x48, 0xa4, 0x92,
    0x86,

    /* U+0026 "&" */
    0x38, 0x91, 0x21, 0x5, 0x31, 0xa1, 0x3d,

    /* U+0027 "'" */
    0xe0,

    /* U+0028 "(" */
    0x5a, 0xaa, 0x50,

    /* U+0029 ")" */
    0xa5, 0x55, 0xa0,

    /* U+002A "*" */
    0x23, 0x9c, 0x40,

    /* U+002B "+" */
    0x21, 0x3e, 0x42, 0x0,

    /* U+002C "," */
    0xe0,

    /* U+002D "-" */
    0xe0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x1, 0x12, 0x22, 0x44, 0x88, 0x80,

    /* U+0030 "0" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x51, 0x1c,

    /* U+0031 "1" */
    0xe4, 0x92, 0x49,

    /* U+0032 "2" */
    0x7a, 0x10, 0x41, 0x8, 0xc4, 0x3f,

    /* U+0033 "3" */
    0xfc, 0x21, 0xe, 0xc, 0x18, 0xde,

    /* U+0034 "4" */
    0x8, 0x10, 0x20, 0x20, 0x44, 0xff, 0x4, 0x4,

    /* U+0035 "5" */
    0x7d, 0x8, 0x3e, 0xc, 0x18, 0x5e,

    /* U+0036 "6" */
    0x39, 0x8, 0x2e, 0xc6, 0x14, 0x5e,

    /* U+0037 "7" */
    0xfe, 0x10, 0x82, 0x10, 0x42, 0x8,

    /* U+0038 "8" */
    0x79, 0xa, 0x13, 0xcc, 0x70, 0x71, 0xbe,

    /* U+0039 "9" */
    0x7a, 0x28, 0x61, 0x7c, 0x10, 0x9c,

    /* U+003A ":" */
    0x84,

    /* U+003B ";" */
    0x87,

    /* U+003C "<" */
    0x1f, 0x30, 0x70,

    /* U+003D "=" */
    0xf8, 0x3e,

    /* U+003E ">" */
    0xc1, 0xc7, 0xc0,

    /* U+003F "?" */
    0x7a, 0x10, 0x42, 0x10, 0x40, 0x4,

    /* U+0040 "@" */
    0x1f, 0xc, 0x19, 0x7b, 0x58, 0xda, 0xb, 0x41,
    0x6c, 0x6a, 0xf6, 0x60, 0x3, 0xe0,

    /* U+0041 "A" */
    0x8, 0xc, 0x5, 0x4, 0x82, 0x23, 0xf1, 0x5,
    0x2,

    /* U+0042 "B" */
    0xfd, 0x6, 0xf, 0xe8, 0x30, 0x60, 0xfe,

    /* U+0043 "C" */
    0x3c, 0x60, 0x80, 0x80, 0x80, 0x80, 0x60, 0x3e,

    /* U+0044 "D" */
    0xfc, 0x86, 0x81, 0x81, 0x81, 0x81, 0x86, 0xfc,

    /* U+0045 "E" */
    0xfe, 0x8, 0x3e, 0x82, 0x8, 0x3f,

    /* U+0046 "F" */
    0xfe, 0x8, 0x20, 0xfa, 0x8, 0x20,

    /* U+0047 "G" */
    0x3e, 0x61, 0x80, 0x80, 0x81, 0x81, 0x61, 0x3e,

    /* U+0048 "H" */
    0x83, 0x6, 0xf, 0xf8, 0x30, 0x60, 0xc1,

    /* U+0049 "I" */
    0xff,

    /* U+004A "J" */
    0x78, 0x42, 0x10, 0x86, 0x2e,

    /* U+004B "K" */
    0x85, 0x12, 0x45, 0xd, 0x11, 0x21, 0x41,

    /* U+004C "L" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x3f,

    /* U+004D "M" */
    0x80, 0xe0, 0xf0, 0xb4, 0x59, 0x4c, 0xa6, 0x23,
    0x1,

    /* U+004E "N" */
    0x83, 0x86, 0x8c, 0x99, 0x31, 0x61, 0xc1,

    /* U+004F "O" */
    0x3e, 0x31, 0xa0, 0x30, 0x18, 0xc, 0x5, 0x8c,
    0x7c,

    /* U+0050 "P" */
    0xfd, 0xe, 0xc, 0x18, 0x7f, 0xa0, 0x40,

    /* U+0051 "Q" */
    0x3e, 0x31, 0xa0, 0x30, 0x18, 0xc, 0x5, 0x8c,
    0x7c, 0x4, 0x1, 0xc0,

    /* U+0052 "R" */
    0xfd, 0xe, 0xc, 0x18, 0x7f, 0x21, 0x41,

    /* U+0053 "S" */
    0x7a, 0x8, 0x10, 0x38, 0x18, 0x5e,

    /* U+0054 "T" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8,

    /* U+0055 "U" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x71, 0xbe,

    /* U+0056 "V" */
    0x81, 0x41, 0x42, 0x22, 0x24, 0x14, 0x18, 0x8,

    /* U+0057 "W" */
    0x42, 0xa, 0x18, 0x91, 0x44, 0x4a, 0x22, 0x8a,
    0x14, 0x50, 0x62, 0x82, 0x8,

    /* U+0058 "X" */
    0x42, 0x24, 0x24, 0x18, 0x18, 0x24, 0x44, 0x42,

    /* U+0059 "Y" */
    0x82, 0x89, 0x11, 0x41, 0x2, 0x4, 0x8,

    /* U+005A "Z" */
    0xfe, 0x8, 0x20, 0x82, 0x8, 0x10, 0x7f,

    /* U+005B "[" */
    0xea, 0xaa, 0xb0,

    /* U+005C "\\" */
    0x8, 0x88, 0x44, 0x22, 0x21, 0x10,

    /* U+005D "]" */
    0xd5, 0x55, 0x70,

    /* U+005E "^" */
    0x23, 0x15, 0x28, 0x80,

    /* U+005F "_" */
    0xfc,

    /* U+0060 "`" */
    0x90,

    /* U+0061 "a" */
    0x70, 0x5f, 0x18, 0xbc,

    /* U+0062 "b" */
    0x82, 0xb, 0xb3, 0x86, 0x1c, 0xee,

    /* U+0063 "c" */
    0x7b, 0x18, 0x20, 0xc5, 0xe0,

    /* U+0064 "d" */
    0x4, 0x17, 0x73, 0x86, 0x1c, 0xdd,

    /* U+0065 "e" */
    0x7a, 0x1f, 0xe0, 0xc1, 0xe0,

    /* U+0066 "f" */
    0x74, 0xf4, 0x44, 0x44,

    /* U+0067 "g" */
    0x7b, 0x8e, 0xc, 0x1c, 0x6f, 0x41, 0xbe,

    /* U+0068 "h" */
    0x82, 0xb, 0xb1, 0x86, 0x18, 0x61,

    /* U+0069 "i" */
    0xbf,

    /* U+006A "j" */
    0x20, 0x92, 0x49, 0x3c,

    /* U+006B "k" */
    0x82, 0x8, 0xa4, 0xa3, 0x48, 0xa1,

    /* U+006C "l" */
    0xff,

    /* U+006D "m" */
    0xb9, 0xd8, 0xc6, 0x10, 0xc2, 0x18, 0x43, 0x8,
    0x40,

    /* U+006E "n" */
    0xbb, 0x18, 0x61, 0x86, 0x10,

    /* U+006F "o" */
    0x7b, 0x38, 0x61, 0xcd, 0xe0,

    /* U+0070 "p" */
    0xbb, 0x38, 0x61, 0xce, 0xe8, 0x20,

    /* U+0071 "q" */
    0x77, 0x38, 0x61, 0xcd, 0xd0, 0x41,

    /* U+0072 "r" */
    0xba, 0x49, 0x0,

    /* U+0073 "s" */
    0x74, 0x20, 0xf0, 0xf8,

    /* U+0074 "t" */
    0x44, 0xf4, 0x44, 0x47,

    /* U+0075 "u" */
    0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+0076 "v" */
    0x85, 0x14, 0x8a, 0x30, 0x40,

    /* U+0077 "w" */
    0x84, 0x63, 0x14, 0xa9, 0x4a, 0x32, 0x88, 0x40,

    /* U+0078 "x" */
    0x44, 0xa3, 0xc, 0x49, 0x10,

    /* U+0079 "y" */
    0x85, 0x14, 0x8a, 0x30, 0x42, 0x38,

    /* U+007A "z" */
    0xf8, 0x88, 0x84, 0x7c,

    /* U+007B "{" */
    0x69, 0x28, 0x92, 0x4c,

    /* U+007C "|" */
    0xff, 0xc0,

    /* U+007D "}" */
    0xc9, 0x22, 0x92, 0x58,

    /* U+007E "~" */
    0xea, 0x60,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xbe,

    /* U+00A2 "¢" */
    0x10, 0x47, 0xf4, 0x92, 0x4d, 0x1f, 0x10, 0x40,

    /* U+00A3 "£" */
    0x3c, 0xc5, 0x2, 0xf, 0x88, 0x10, 0x7f,

    /* U+00A4 "¤" */
    0x0, 0x5d, 0x22, 0x41, 0x41, 0x22, 0x5d, 0x0,

    /* U+00A5 "¥" */
    0x41, 0x11, 0x8, 0x82, 0x87, 0xf0, 0x41, 0xfc,
    0x10,

    /* U+00A6 "¦" */
    0xf3, 0xc0,

    /* U+00A7 "§" */
    0x7c, 0x20, 0xe8, 0xc5, 0xc1, 0xf, 0x80,

    /* U+00A8 "¨" */
    0xa0,

    /* U+00A9 "©" */
    0x3e, 0x3f, 0xac, 0x34, 0x1a, 0xd, 0x85, 0xfc,
    0x7c,

    /* U+00AA "ª" */
    0xf7, 0x97,

    /* U+00AB "«" */
    0x5a, 0xa5,

    /* U+00AC "¬" */
    0xf8, 0x42,

    /* U+00AE "®" */
    0x3e, 0x3f, 0xa8, 0xb4, 0x5b, 0xcd, 0x25, 0x8c,
    0x7c,

    /* U+00AF "¯" */
    0xe0,

    /* U+00B0 "°" */
    0x69, 0x96,

    /* U+00B1 "±" */
    0x27, 0xc8, 0x40, 0x7c,

    /* U+00B2 "²" */
    0xf1, 0x24, 0xf0,

    /* U+00B3 "³" */
    0x78, 0x8c, 0x17, 0x80,

    /* U+00B4 "´" */
    0x70,

    /* U+00B5 "µ" */
    0x86, 0x18, 0x61, 0x8f, 0xd8, 0x20,

    /* U+00B6 "¶" */
    0x7e, 0xe5, 0xcb, 0x91, 0x22, 0x44, 0x89, 0x12,
    0x24,

    /* U+00B7 "·" */
    0x80,

    /* U+00B8 "¸" */
    0x9c,

    /* U+00B9 "¹" */
    0xc9, 0x2e,

    /* U+00BA "º" */
    0x69, 0x96,

    /* U+00BB "»" */
    0x51, 0x4a, 0xa0,

    /* U+00BC "¼" */
    0xc1, 0x8, 0x41, 0x10, 0x22, 0x4e, 0x90, 0x24,
    0x84, 0xf9, 0x2,

    /* U+00BD "½" */
    0xc2, 0x10, 0x84, 0x41, 0x2f, 0xe8, 0x44, 0x22,
    0x10, 0x8f,

    /* U+00BE "¾" */
    0x78, 0x81, 0x10, 0x39, 0x0, 0xa2, 0x74, 0x40,
    0x4a, 0x8, 0xf1, 0x2,

    /* U+00BF "¿" */
    0x20, 0x2, 0x8, 0x42, 0x8, 0x5e,

    /* U+00C0 "À" */
    0x10, 0x4, 0x2, 0x3, 0x1, 0x41, 0x20, 0x88,
    0xfc, 0x41, 0x40, 0x80,

    /* U+00C1 "Á" */
    0x4, 0x4, 0x2, 0x3, 0x1, 0x41, 0x20, 0x88,
    0xfc, 0x41, 0x40, 0x80,

    /* U+00C2 "Â" */
    0x18, 0x12, 0x2, 0x3, 0x1, 0x41, 0x20, 0x88,
    0xfc, 0x41, 0x40, 0x80,

    /* U+00C3 "Ã" */
    0x14, 0x16, 0x2, 0x3, 0x1, 0x41, 0x20, 0x88,
    0xfc, 0x41, 0x40, 0x80,

    /* U+00C4 "Ä" */
    0x14, 0x0, 0x2, 0x3, 0x1, 0x41, 0x20, 0x88,
    0xfc, 0x41, 0x40, 0x80,

    /* U+00C5 "Å" */
    0x1c, 0xa, 0x7, 0x1, 0x1, 0x40, 0xa0, 0x88,
    0x44, 0x7f, 0x20, 0xa0, 0x20,

    /* U+00C6 "Æ" */
    0x7, 0xf0, 0xa0, 0xa, 0x1, 0x3e, 0x22, 0x3,
    0xe0, 0x42, 0x4, 0x3f,

    /* U+00C7 "Ç" */
    0x3e, 0x62, 0x80, 0x80, 0x80, 0x80, 0x62, 0x3e,
    0x10, 0x18,

    /* U+00C8 "È" */
    0x20, 0x4f, 0xe0, 0x83, 0xe8, 0x20, 0x83, 0xf0,

    /* U+00C9 "É" */
    0x10, 0x8f, 0xe0, 0x83, 0xe8, 0x20, 0x83, 0xf0,

    /* U+00CA "Ê" */
    0x31, 0x2f, 0xe0, 0x83, 0xe8, 0x20, 0x83, 0xf0,

    /* U+00CB "Ë" */
    0x28, 0xf, 0xe0, 0x83, 0xe8, 0x20, 0x83, 0xf0,

    /* U+00CC "Ì" */
    0x44, 0x92, 0x49, 0x24,

    /* U+00CD "Í" */
    0x52, 0x49, 0x24, 0x90,

    /* U+00CE "Î" */
    0x74, 0x48, 0x42, 0x10, 0x84, 0x21, 0x0,

    /* U+00CF "Ï" */
    0xa1, 0x24, 0x92, 0x48,

    /* U+00D0 "Ð" */
    0x7e, 0x21, 0x90, 0x3e, 0x14, 0xa, 0x5, 0xc,
    0xfc,

    /* U+00D1 "Ñ" */
    0x38, 0xb2, 0xe, 0x1a, 0x32, 0x64, 0xc5, 0x87,
    0x4,

    /* U+00D2 "Ò" */
    0x10, 0x4, 0xf, 0x8c, 0x68, 0xc, 0x6, 0x3,
    0x1, 0x63, 0x1f, 0x0,

    /* U+00D3 "Ó" */
    0x4, 0x4, 0xf, 0x8c, 0x68, 0xc, 0x6, 0x3,
    0x1, 0x63, 0x1f, 0x0,

    /* U+00D4 "Ô" */
    0x8, 0xa, 0xf, 0x8c, 0x68, 0xc, 0x6, 0x3,
    0x1, 0x63, 0x1f, 0x0,

    /* U+00D5 "Õ" */
    0x1a, 0x16, 0xf, 0x8c, 0x68, 0xc, 0x6, 0x3,
    0x1, 0x63, 0x1f, 0x0,

    /* U+00D6 "Ö" */
    0x24, 0x0, 0xf, 0x8c, 0x68, 0xc, 0x6, 0x3,
    0x1, 0x63, 0x1f, 0x0,

    /* U+00D7 "×" */
    0x8b, 0x99, 0x20,

    /* U+00D8 "Ø" */
    0x2, 0x1f, 0x19, 0xd0, 0x98, 0x8c, 0x86, 0x42,
    0xc6, 0x3e, 0x20, 0x0,

    /* U+00D9 "Ù" */
    0x20, 0x22, 0xc, 0x18, 0x30, 0x60, 0xc1, 0xc6,
    0xf8,

    /* U+00DA "Ú" */
    0x8, 0x22, 0xc, 0x18, 0x30, 0x60, 0xc1, 0xc6,
    0xf8,

    /* U+00DB "Û" */
    0x10, 0x52, 0xc, 0x18, 0x30, 0x60, 0xc1, 0xc6,
    0xf8,

    /* U+00DC "Ü" */
    0x28, 0x2, 0xc, 0x18, 0x30, 0x60, 0xc1, 0xc6,
    0xf8,

    /* U+00DD "Ý" */
    0x8, 0x22, 0xa, 0x24, 0x45, 0x4, 0x8, 0x10,
    0x20,

    /* U+00DE "Þ" */
    0x81, 0xfa, 0x1c, 0x18, 0x30, 0xff, 0x40,

    /* U+00DF "ß" */
    0x7b, 0x18, 0x66, 0x86, 0x18, 0x6e,

    /* U+00E0 "à" */
    0x41, 0x1c, 0x17, 0xc6, 0x2f,

    /* U+00E1 "á" */
    0x11, 0x1c, 0x17, 0xc6, 0x2f,

    /* U+00E2 "â" */
    0x32, 0x1c, 0x17, 0xc6, 0x2f,

    /* U+00E3 "ã" */
    0x70, 0x1c, 0x17, 0xc6, 0x2f,

    /* U+00E4 "ä" */
    0x50, 0x1c, 0x17, 0xc6, 0x2f,

    /* U+00E5 "å" */
    0x73, 0x80, 0xe0, 0xbe, 0x31, 0x78,

    /* U+00E6 "æ" */
    0x7b, 0xc0, 0x85, 0xff, 0xc2, 0x8, 0xa0, 0xf7,
    0x80,

    /* U+00E7 "ç" */
    0x7b, 0x8, 0x20, 0xc1, 0xe0, 0xc,

    /* U+00E8 "è" */
    0x20, 0x7, 0xa1, 0xfe, 0xc, 0x1e,

    /* U+00E9 "é" */
    0x10, 0x7, 0xa1, 0xfe, 0xc, 0x1e,

    /* U+00EA "ê" */
    0x30, 0x7, 0xa1, 0xfe, 0xc, 0x1e,

    /* U+00EB "ë" */
    0x50, 0x7, 0xa1, 0xfe, 0xc, 0x1e,

    /* U+00EC "ì" */
    0x44, 0x92, 0x49,

    /* U+00ED "í" */
    0x52, 0x49, 0x24,

    /* U+00EE "î" */
    0xe1, 0x24, 0x92,

    /* U+00EF "ï" */
    0xa1, 0x24, 0x92,

    /* U+00F0 "ð" */
    0x28, 0xc0, 0x9d, 0x8e, 0x18, 0x5e,

    /* U+00F1 "ñ" */
    0x78, 0xb, 0xb1, 0x86, 0x18, 0x61,

    /* U+00F2 "ò" */
    0x20, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F3 "ó" */
    0x10, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F4 "ô" */
    0x30, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F5 "õ" */
    0x70, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F6 "ö" */
    0x28, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F7 "÷" */
    0x20, 0x3e, 0x2, 0x0,

    /* U+00F8 "ø" */
    0x9, 0xed, 0xe9, 0xa7, 0xb7, 0x90,

    /* U+00F9 "ù" */
    0x20, 0x8, 0x61, 0x86, 0x18, 0xdd,

    /* U+00FA "ú" */
    0x10, 0x8, 0x61, 0x86, 0x18, 0xdd,

    /* U+00FB "û" */
    0x30, 0x8, 0x61, 0x86, 0x18, 0xdd,

    /* U+00FC "ü" */
    0x48, 0x8, 0x61, 0x86, 0x18, 0xdd,

    /* U+00FD "ý" */
    0x10, 0x8, 0x51, 0x48, 0xa3, 0x4, 0x23, 0x80,

    /* U+00FE "þ" */
    0x82, 0xb, 0xb3, 0x86, 0x1c, 0xee, 0x82, 0x0,

    /* U+00FF "ÿ" */
    0x68, 0x8, 0x51, 0x48, 0xa3, 0x4, 0x23, 0x80
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 50, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 50, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2, .adv_w = 72, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 4, .adv_w = 134, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 12, .adv_w = 118, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 21, .adv_w = 159, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 37, .adv_w = 39, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 38, .adv_w = 63, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 41, .adv_w = 63, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 44, .adv_w = 74, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 47, .adv_w = 110, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 51, .adv_w = 41, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 52, .adv_w = 73, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 53, .adv_w = 41, .box_w = 1, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 54, .adv_w = 64, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 60, .adv_w = 127, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 67, .adv_w = 69, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 109, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 76, .adv_w = 108, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 127, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 109, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 117, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 102, .adv_w = 113, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 108, .adv_w = 122, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 115, .adv_w = 117, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 121, .adv_w = 41, .box_w = 1, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 122, .adv_w = 41, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 123, .adv_w = 110, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 126, .adv_w = 110, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 128, .adv_w = 110, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 131, .adv_w = 109, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 137, .adv_w = 198, .box_w = 11, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 151, .adv_w = 138, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 160, .adv_w = 145, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 136, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 175, .adv_w = 159, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 183, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 189, .adv_w = 122, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 195, .adv_w = 148, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 203, .adv_w = 156, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 210, .adv_w = 58, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 211, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 216, .adv_w = 137, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 223, .adv_w = 113, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 229, .adv_w = 183, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 238, .adv_w = 156, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 245, .adv_w = 161, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 254, .adv_w = 138, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 261, .adv_w = 161, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 273, .adv_w = 139, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 286, .adv_w = 110, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 293, .adv_w = 152, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 300, .adv_w = 134, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 308, .adv_w = 213, .box_w = 13, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 321, .adv_w = 126, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 329, .adv_w = 122, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 336, .adv_w = 125, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 343, .adv_w = 61, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 346, .adv_w = 64, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 352, .adv_w = 61, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 355, .adv_w = 111, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 359, .adv_w = 96, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 360, .adv_w = 115, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 361, .adv_w = 113, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 365, .adv_w = 130, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 371, .adv_w = 108, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 376, .adv_w = 130, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 382, .adv_w = 116, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 387, .adv_w = 65, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 391, .adv_w = 132, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 398, .adv_w = 130, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 404, .adv_w = 52, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 405, .adv_w = 53, .box_w = 3, .box_h = 10, .ofs_x = -1, .ofs_y = -2},
    {.bitmap_index = 409, .adv_w = 115, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 415, .adv_w = 52, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 416, .adv_w = 204, .box_w = 11, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 425, .adv_w = 130, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 430, .adv_w = 120, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 435, .adv_w = 130, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 441, .adv_w = 130, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 447, .adv_w = 77, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 450, .adv_w = 94, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 454, .adv_w = 78, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 458, .adv_w = 129, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 463, .adv_w = 104, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 468, .adv_w = 169, .box_w = 10, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 476, .adv_w = 103, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 481, .adv_w = 104, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 487, .adv_w = 98, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 491, .adv_w = 64, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 495, .adv_w = 57, .box_w = 1, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 497, .adv_w = 64, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 501, .adv_w = 110, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 503, .adv_w = 50, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 504, .adv_w = 50, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 505, .adv_w = 108, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 513, .adv_w = 122, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 520, .adv_w = 134, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 528, .adv_w = 133, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 537, .adv_w = 57, .box_w = 1, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 539, .adv_w = 94, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 546, .adv_w = 115, .box_w = 3, .box_h = 1, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 547, .adv_w = 155, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 556, .adv_w = 77, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 558, .adv_w = 92, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 560, .adv_w = 110, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 562, .adv_w = 155, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 571, .adv_w = 115, .box_w = 3, .box_h = 1, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 572, .adv_w = 80, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 574, .adv_w = 110, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 578, .adv_w = 83, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 581, .adv_w = 83, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 585, .adv_w = 115, .box_w = 3, .box_h = 2, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 586, .adv_w = 130, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 592, .adv_w = 121, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 601, .adv_w = 48, .box_w = 1, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 602, .adv_w = 115, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 603, .adv_w = 83, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 605, .adv_w = 79, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 607, .adv_w = 92, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 610, .adv_w = 198, .box_w = 11, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 621, .adv_w = 198, .box_w = 10, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 631, .adv_w = 198, .box_w = 12, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 643, .adv_w = 109, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 649, .adv_w = 138, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 661, .adv_w = 138, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 673, .adv_w = 138, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 685, .adv_w = 138, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 697, .adv_w = 138, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 709, .adv_w = 138, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 722, .adv_w = 198, .box_w = 12, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 734, .adv_w = 136, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 744, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 752, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 760, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 768, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 776, .adv_w = 58, .box_w = 3, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 780, .adv_w = 58, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 784, .adv_w = 58, .box_w = 5, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 791, .adv_w = 58, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 795, .adv_w = 160, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 804, .adv_w = 156, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 813, .adv_w = 161, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 825, .adv_w = 161, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 837, .adv_w = 161, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 849, .adv_w = 161, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 861, .adv_w = 161, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 873, .adv_w = 110, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 876, .adv_w = 161, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 888, .adv_w = 152, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 897, .adv_w = 152, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 906, .adv_w = 152, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 915, .adv_w = 152, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 924, .adv_w = 122, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 933, .adv_w = 138, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 940, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 946, .adv_w = 113, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 951, .adv_w = 113, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 956, .adv_w = 113, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 961, .adv_w = 113, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 966, .adv_w = 113, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 971, .adv_w = 113, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 977, .adv_w = 189, .box_w = 11, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 986, .adv_w = 108, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 992, .adv_w = 116, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 998, .adv_w = 116, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1004, .adv_w = 116, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1010, .adv_w = 116, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1016, .adv_w = 52, .box_w = 3, .box_h = 8, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1019, .adv_w = 52, .box_w = 3, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1022, .adv_w = 52, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1025, .adv_w = 52, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1028, .adv_w = 113, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1034, .adv_w = 130, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1040, .adv_w = 120, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1046, .adv_w = 120, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1052, .adv_w = 120, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1058, .adv_w = 120, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1064, .adv_w = 120, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1070, .adv_w = 110, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 1074, .adv_w = 120, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1080, .adv_w = 129, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1086, .adv_w = 129, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1092, .adv_w = 129, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1098, .adv_w = 129, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1104, .adv_w = 104, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1112, .adv_w = 130, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1120, .adv_w = 104, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2}
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
const lv_font_t lv_font_hs_regular_12 = {
#else
lv_font_t lv_font_hs_regular_12 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 14,          /*The maximum line height required by the font*/
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



#endif /*#if LV_FONT_HS_REGULAR_12*/

