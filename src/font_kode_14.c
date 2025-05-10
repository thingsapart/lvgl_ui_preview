/*******************************************************************************
 * Size: 13 px
 * Bpp: 1
 * Opts: --bpp 1 --size 13 --no-compress --font KodeMono-SemiBold.ttf --range 32-255 --format lvgl -o font_kode_14.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef FONT_KODE_14
#define FONT_KODE_14 1
#endif

#if FONT_KODE_14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xfc, 0x80,

    /* U+0022 "\"" */
    0x99, 0x99,

    /* U+0023 "#" */
    0x34, 0x48, 0x93, 0xf2, 0x85, 0xa, 0x7e, 0x48,
    0xb0,

    /* U+0024 "$" */
    0x10, 0x23, 0xfc, 0x8, 0x10, 0x1f, 0x81, 0x3,
    0x7, 0xf8, 0x81, 0x0,

    /* U+0025 "%" */
    0x61, 0x31, 0x81, 0x81, 0x80, 0x80, 0xc0, 0xc0,
    0xc6, 0xc3, 0x0,

    /* U+0026 "&" */
    0x7c, 0x4c, 0x78, 0x30, 0x7b, 0xcf, 0x8e, 0x8e,
    0xfa,

    /* U+0027 "'" */
    0xf0,

    /* U+0028 "(" */
    0x3, 0x6c, 0x88, 0x88, 0x88, 0xc6, 0x32,

    /* U+0029 ")" */
    0xc, 0x63, 0x11, 0x11, 0x11, 0x36, 0xc8,

    /* U+002A "*" */
    0x25, 0x7e, 0xe5, 0x0,

    /* U+002B "+" */
    0x21, 0x3e, 0x42, 0x0,

    /* U+002C "," */
    0xf5, 0x0,

    /* U+002D "-" */
    0xf8,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0xc, 0x21, 0x84, 0x10, 0xc2, 0x8, 0x61, 0x4,
    0x30, 0x0,

    /* U+0030 "0" */
    0xfa, 0x18, 0xe7, 0xb7, 0x9c, 0x61, 0x7c,

    /* U+0031 "1" */
    0x31, 0xcd, 0x4, 0x10, 0x41, 0x4, 0xfc,

    /* U+0032 "2" */
    0xfa, 0x30, 0xc6, 0x10, 0x84, 0x20, 0xfc,

    /* U+0033 "3" */
    0xfc, 0x31, 0x8c, 0x38, 0x10, 0x41, 0xfc,

    /* U+0034 "4" */
    0xc, 0x38, 0xd3, 0x2c, 0x4f, 0xc1, 0x2, 0x4,

    /* U+0035 "5" */
    0xfe, 0x8, 0x3e, 0xc, 0x10, 0x41, 0xfc,

    /* U+0036 "6" */
    0xfa, 0x8, 0x20, 0xff, 0x18, 0x61, 0xf8,

    /* U+0037 "7" */
    0xfc, 0x10, 0x84, 0x21, 0x4, 0x10, 0x40,

    /* U+0038 "8" */
    0x7f, 0x18, 0x61, 0x7b, 0x38, 0x61, 0x78,

    /* U+0039 "9" */
    0xfa, 0x38, 0x71, 0x7c, 0x10, 0x43, 0xf8,

    /* U+003A ":" */
    0xf0, 0x3c,

    /* U+003B ";" */
    0xff, 0x80, 0x36, 0x58, 0x0,

    /* U+003C "<" */
    0x19, 0x99, 0x84, 0x10, 0x40,

    /* U+003D "=" */
    0xfc, 0x0, 0x3f,

    /* U+003E ">" */
    0xc3, 0xc, 0x31, 0x11, 0x0,

    /* U+003F "?" */
    0x7d, 0x88, 0x18, 0x61, 0x82, 0x4, 0x0, 0x10,

    /* U+0040 "@" */
    0x3c, 0x42, 0x81, 0x9d, 0xa5, 0xa5, 0x9f, 0x80,
    0x40, 0x3c,

    /* U+0041 "A" */
    0x3d, 0x18, 0x61, 0xfe, 0x18, 0x61, 0x84,

    /* U+0042 "B" */
    0xfe, 0x18, 0xe6, 0xfa, 0x18, 0x61, 0xfc,

    /* U+0043 "C" */
    0xfa, 0x28, 0x20, 0x82, 0x8, 0x22, 0x78,

    /* U+0044 "D" */
    0xfa, 0x28, 0x61, 0x86, 0x18, 0x61, 0xfc,

    /* U+0045 "E" */
    0xfe, 0x8, 0x20, 0xf2, 0x8, 0x20, 0x7c,

    /* U+0046 "F" */
    0x7e, 0x8, 0x20, 0xf2, 0x8, 0x20, 0x80,

    /* U+0047 "G" */
    0xfa, 0x18, 0x20, 0x9e, 0x18, 0x61, 0xfc,

    /* U+0048 "H" */
    0x86, 0x18, 0x61, 0xfe, 0x18, 0x61, 0x84,

    /* U+0049 "I" */
    0xfc, 0x41, 0x4, 0x10, 0x41, 0x4, 0xfc,

    /* U+004A "J" */
    0xf8, 0x42, 0x10, 0x84, 0x21, 0x13, 0x80,

    /* U+004B "K" */
    0x8d, 0x32, 0xc7, 0xf, 0x12, 0x26, 0x46, 0x84,

    /* U+004C "L" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x21, 0x7c,

    /* U+004D "M" */
    0x8f, 0x7f, 0x6d, 0x86, 0x18, 0x61, 0x84,

    /* U+004E "N" */
    0xe6, 0x9b, 0x67, 0x8e, 0x18, 0x61, 0x84,

    /* U+004F "O" */
    0xfe, 0x18, 0x61, 0x86, 0x18, 0x61, 0x7c,

    /* U+0050 "P" */
    0xfa, 0x18, 0x61, 0xfe, 0x8, 0x20, 0x80,

    /* U+0051 "Q" */
    0xfa, 0x18, 0x61, 0x86, 0x18, 0x26, 0x7c, 0x0,

    /* U+0052 "R" */
    0xfa, 0x38, 0x61, 0xfa, 0x28, 0x61, 0x84,

    /* U+0053 "S" */
    0xfe, 0x28, 0x20, 0x7c, 0x10, 0x61, 0xfc,

    /* U+0054 "T" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,

    /* U+0055 "U" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x7c,

    /* U+0056 "V" */
    0x86, 0x18, 0x61, 0x86, 0x1c, 0xde, 0x30,

    /* U+0057 "W" */
    0x83, 0x6, 0xc, 0x19, 0x32, 0x64, 0xc9, 0x6c,

    /* U+0058 "X" */
    0x86, 0x18, 0x63, 0x7b, 0x18, 0x61, 0x84,

    /* U+0059 "Y" */
    0x83, 0x5, 0x19, 0x61, 0x2, 0x4, 0x8, 0x10,

    /* U+005A "Z" */
    0xfc, 0x10, 0x42, 0x79, 0x8c, 0x20, 0xfc,

    /* U+005B "[" */
    0xf8, 0x88, 0x88, 0x88, 0x88, 0x88, 0xf0,

    /* U+005C "\\" */
    0xc1, 0x4, 0x18, 0x20, 0x83, 0x4, 0x10, 0x60,
    0x82, 0x0,

    /* U+005D "]" */
    0xf1, 0x11, 0x11, 0x11, 0x11, 0x11, 0xf0,

    /* U+005E "^" */
    0x31, 0xe4, 0xb3, 0x84,

    /* U+005F "_" */
    0xfe,

    /* U+0060 "`" */
    0xc3,

    /* U+0061 "a" */
    0x7c, 0x10, 0x4f, 0x67, 0x17, 0xc0,

    /* U+0062 "b" */
    0x82, 0x8, 0x3e, 0x86, 0x18, 0x61, 0x87, 0xf0,

    /* U+0063 "c" */
    0xfa, 0x28, 0x20, 0x82, 0x27, 0x80,

    /* U+0064 "d" */
    0x4, 0x10, 0x7f, 0x86, 0x18, 0x61, 0x85, 0xf0,

    /* U+0065 "e" */
    0xfa, 0x39, 0xbc, 0x82, 0x27, 0x80,

    /* U+0066 "f" */
    0x7d, 0x14, 0x10, 0xf9, 0x4, 0x10, 0x40,

    /* U+0067 "g" */
    0xfe, 0x18, 0x63, 0xfa, 0xf, 0xe1, 0x8f, 0xe0,

    /* U+0068 "h" */
    0x82, 0x8, 0x3e, 0x86, 0x18, 0x61, 0x86, 0x10,

    /* U+0069 "i" */
    0x10, 0x40, 0x0, 0x60, 0x41, 0x4, 0x10, 0x4f,
    0xc0,

    /* U+006A "j" */
    0x20, 0x7, 0x11, 0x11, 0x13, 0xe0,

    /* U+006B "k" */
    0x82, 0x8, 0x26, 0xb3, 0xcd, 0xa2, 0x86, 0x10,

    /* U+006C "l" */
    0xe0, 0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x70,

    /* U+006D "m" */
    0xed, 0x26, 0x4c, 0x18, 0x30, 0x60, 0x80,

    /* U+006E "n" */
    0xfa, 0x18, 0x61, 0x86, 0x18, 0x40,

    /* U+006F "o" */
    0xfa, 0x18, 0x61, 0x86, 0x17, 0xc0,

    /* U+0070 "p" */
    0xfa, 0x18, 0x61, 0x86, 0x1f, 0xe0, 0x82, 0x0,

    /* U+0071 "q" */
    0xfe, 0x18, 0x61, 0x86, 0x17, 0xc1, 0x4, 0x10,

    /* U+0072 "r" */
    0xfc, 0x40, 0x81, 0x2, 0x4, 0x3e, 0x0,

    /* U+0073 "s" */
    0xfe, 0x18, 0x1f, 0x6, 0x1f, 0xc0,

    /* U+0074 "t" */
    0x41, 0x4, 0x3e, 0x41, 0x4, 0x10, 0x44, 0xf0,

    /* U+0075 "u" */
    0x86, 0x18, 0x61, 0x86, 0x17, 0xc0,

    /* U+0076 "v" */
    0x86, 0x18, 0x61, 0xcd, 0x63, 0x0,

    /* U+0077 "w" */
    0x93, 0x26, 0x4c, 0x99, 0x2f, 0xdb, 0x0,

    /* U+0078 "x" */
    0x86, 0x18, 0xde, 0xc6, 0x18, 0x40,

    /* U+0079 "y" */
    0x86, 0x18, 0x61, 0x86, 0x17, 0xc1, 0xb, 0xc0,

    /* U+007A "z" */
    0xfc, 0x31, 0x8c, 0x63, 0xf, 0xc0,

    /* U+007B "{" */
    0x39, 0x8, 0x46, 0x22, 0x8, 0x61, 0x8, 0x43,
    0x80,

    /* U+007C "|" */
    0xff, 0xf0,

    /* U+007D "}" */
    0xe1, 0x8, 0x43, 0x8, 0x22, 0x31, 0x8, 0x4e,
    0x0,

    /* U+007E "~" */
    0x66, 0xf0,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0x9f, 0x80,

    /* U+00A2 "¢" */
    0x10, 0x4f, 0xe1, 0x82, 0x8, 0x21, 0x7c, 0x41,
    0x0,

    /* U+00A3 "£" */
    0x3e, 0x8d, 0x2, 0xf, 0x88, 0x10, 0x20, 0xfe,

    /* U+00A4 "¤" */
    0x1, 0xe4, 0x92, 0x78, 0x0,

    /* U+00A5 "¥" */
    0x83, 0x5, 0x19, 0x63, 0x9f, 0xff, 0x88, 0x10,

    /* U+00A6 "¦" */
    0xf1, 0xe0,

    /* U+00A7 "§" */
    0xfc, 0x61, 0xf8, 0xc7, 0xe1, 0x8f, 0xc0,

    /* U+00A8 "¨" */
    0x90,

    /* U+00A9 "©" */
    0x79, 0x8a, 0xed, 0xfb, 0xf8, 0xde, 0x0,

    /* U+00AA "ª" */
    0x7c, 0x10, 0x4f, 0xc7, 0x17, 0xc0,

    /* U+00AB "«" */
    0x2d, 0x2d, 0x3c, 0x58, 0xa0,

    /* U+00AC "¬" */
    0xf8, 0x42, 0x10,

    /* U+00AE "®" */
    0x75, 0x6f, 0x57, 0x0,

    /* U+00AF "¯" */
    0xf0,

    /* U+00B0 "°" */
    0xf7, 0x80,

    /* U+00B1 "±" */
    0x21, 0x3e, 0x42, 0x3, 0xe0,

    /* U+00B2 "²" */
    0xe7, 0x70,

    /* U+00B3 "³" */
    0xe8, 0xf0,

    /* U+00B4 "´" */
    0x6c,

    /* U+00B6 "¶" */
    0x7f, 0xf7, 0xef, 0xdf, 0xaf, 0x4e, 0x85, 0xa,
    0x14, 0x28,

    /* U+00B7 "·" */
    0xf0,

    /* U+00B8 "¸" */
    0x9c,

    /* U+00B9 "¹" */
    0xc9, 0x70,

    /* U+00BA "º" */
    0xfa, 0x18, 0x61, 0x86, 0x17, 0xc0,

    /* U+00BB "»" */
    0xb1, 0x46, 0x9b, 0x5a, 0x40,

    /* U+00BC "¼" */
    0x20, 0xc0, 0x81, 0x7, 0x0, 0xc6, 0x30, 0xc,
    0x28, 0x30, 0x20, 0x40,

    /* U+00BD "½" */
    0x20, 0x60, 0x20, 0x20, 0x70, 0xe, 0x30, 0x4c,
    0x2, 0x4, 0x8, 0x1e,

    /* U+00BE "¾" */
    0x70, 0x41, 0xc0, 0x87, 0x0, 0xce, 0x60, 0xc,
    0x28, 0x30, 0x20, 0x40,

    /* U+00BF "¿" */
    0x10, 0x0, 0x40, 0x83, 0xc, 0x30, 0x23, 0x7c,

    /* U+00C0 "À" */
    0x10, 0x60, 0xf, 0x46, 0x18, 0x7f, 0x86, 0x18,
    0x61,

    /* U+00C1 "Á" */
    0xc, 0x60, 0xf, 0x46, 0x18, 0x7f, 0x86, 0x18,
    0x61,

    /* U+00C2 "Â" */
    0x8, 0xf0, 0xf, 0x46, 0x18, 0x7f, 0x86, 0x18,
    0x61,

    /* U+00C3 "Ã" */
    0x7e, 0x0, 0xf2, 0x28, 0x50, 0xbf, 0x42, 0x85,
    0xa, 0x10,

    /* U+00C4 "Ä" */
    0x6c, 0x7, 0xd1, 0x86, 0x1f, 0xe1, 0x86, 0x18,
    0x40,

    /* U+00C5 "Å" */
    0x18, 0xd3, 0x46, 0x3d, 0x18, 0x61, 0xfe, 0x18,
    0x61, 0x84,

    /* U+00C6 "Æ" */
    0x3f, 0x48, 0x88, 0x88, 0xfe, 0x88, 0x88, 0x88,
    0x8f,

    /* U+00C7 "Ç" */
    0xfa, 0x28, 0x20, 0x82, 0x8, 0x22, 0x78, 0x81,
    0xc,

    /* U+00C8 "È" */
    0x20, 0xc0, 0x3f, 0x82, 0x8, 0x3c, 0x82, 0x8,
    0x1f,

    /* U+00C9 "É" */
    0x11, 0xc0, 0x3f, 0x82, 0x8, 0x3c, 0x82, 0x8,
    0x1f,

    /* U+00CA "Ê" */
    0x21, 0xe0, 0xbf, 0x82, 0x8, 0x3c, 0x82, 0x8,
    0x1f,

    /* U+00CB "Ë" */
    0x68, 0xf, 0xe0, 0x82, 0xf, 0x20, 0x82, 0x7,
    0xc0,

    /* U+00CC "Ì" */
    0x30, 0x60, 0x3f, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x3f,

    /* U+00CD "Í" */
    0x18, 0xc0, 0x3f, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x3f,

    /* U+00CE "Î" */
    0x31, 0xa0, 0x3f, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x3f,

    /* U+00CF "Ï" */
    0x68, 0xf, 0xc4, 0x10, 0x41, 0x4, 0x10, 0x4f,
    0xc0,

    /* U+00D0 "Ð" */
    0x7c, 0x89, 0xa, 0x1e, 0x28, 0x50, 0xa1, 0x7e,

    /* U+00D1 "Ñ" */
    0xfc, 0xe, 0x69, 0xb6, 0x78, 0xe1, 0x86, 0x18,
    0x40,

    /* U+00D2 "Ò" */
    0x21, 0xe0, 0x3f, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5f,

    /* U+00D3 "Ó" */
    0x11, 0xe0, 0x3f, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5f,

    /* U+00D4 "Ô" */
    0x1, 0xe0, 0x3f, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5f,

    /* U+00D5 "Õ" */
    0xfc, 0xf, 0xe1, 0x86, 0x18, 0x61, 0x86, 0x17,
    0xc0,

    /* U+00D6 "Ö" */
    0x48, 0xf, 0xa1, 0x86, 0x18, 0x61, 0x86, 0x17,
    0xc0,

    /* U+00D7 "×" */
    0x9e, 0x6b,

    /* U+00D8 "Ø" */
    0xf, 0xf8, 0xe7, 0x96, 0xda, 0x69, 0xe5, 0xf4,
    0x0,

    /* U+00D9 "Ù" */
    0x20, 0xe0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5f,

    /* U+00DA "Ú" */
    0x11, 0xc0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5f,

    /* U+00DB "Û" */
    0x1, 0xe0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5f,

    /* U+00DC "Ü" */
    0x68, 0x8, 0x61, 0x86, 0x18, 0x61, 0x86, 0x17,
    0xc0,

    /* U+00DD "Ý" */
    0x18, 0xf8, 0x4, 0x18, 0x28, 0xcb, 0x8, 0x10,
    0x20, 0x40, 0x80,

    /* U+00DE "Þ" */
    0x82, 0xf, 0xa1, 0x86, 0x18, 0x61, 0xfe, 0x0,

    /* U+00DF "ß" */
    0x21, 0xec, 0xae, 0xba, 0x38, 0x61, 0x9c,

    /* U+00E0 "à" */
    0x30, 0x60, 0x1f, 0x4, 0x13, 0xd9, 0xc5, 0xf0,

    /* U+00E1 "á" */
    0x18, 0xc0, 0x1f, 0x4, 0x13, 0xd9, 0xc5, 0xf0,

    /* U+00E2 "â" */
    0x11, 0xb0, 0x1f, 0x4, 0x13, 0xd9, 0xc5, 0xf0,

    /* U+00E3 "ã" */
    0x1, 0xf0, 0x1f, 0x4, 0x13, 0xd9, 0xc5, 0xf0,

    /* U+00E4 "ä" */
    0x28, 0xa0, 0x1f, 0x4, 0x13, 0xd9, 0xc5, 0xf0,

    /* U+00E5 "å" */
    0x18, 0xa1, 0x80, 0x7c, 0x10, 0x4f, 0x67, 0x17,
    0xc0,

    /* U+00E6 "æ" */
    0x3e, 0x9, 0xa, 0x3c, 0x68, 0xc9, 0x7f,

    /* U+00E7 "ç" */
    0xfc, 0x61, 0x8, 0x45, 0xe4, 0x11, 0x80,

    /* U+00E8 "è" */
    0x60, 0xc0, 0x3e, 0x8e, 0x6f, 0x20, 0x89, 0xe0,

    /* U+00E9 "é" */
    0x31, 0x80, 0x3e, 0x8e, 0x6f, 0x20, 0x89, 0xe0,

    /* U+00EA "ê" */
    0x31, 0x60, 0x3e, 0x8e, 0x6f, 0x20, 0x89, 0xe0,

    /* U+00EB "ë" */
    0x59, 0x60, 0x3e, 0x8e, 0x6f, 0x20, 0x81, 0xe0,

    /* U+00EC "ì" */
    0x20, 0xe0, 0x0, 0x60, 0x41, 0x4, 0x10, 0x4f,
    0xc0,

    /* U+00ED "í" */
    0x0, 0xc0, 0x0, 0x60, 0x41, 0x4, 0x10, 0x4f,
    0xc0,

    /* U+00EE "î" */
    0x11, 0xa0, 0x0, 0x60, 0x41, 0x4, 0x10, 0x4f,
    0xc0,

    /* U+00EF "ï" */
    0x68, 0x0, 0x18, 0x10, 0x41, 0x4, 0x13, 0xf0,

    /* U+00F0 "ð" */
    0x1, 0xe3, 0x8a, 0x35, 0xb4, 0x71, 0x45, 0xe1,
    0x0,

    /* U+00F1 "ñ" */
    0x21, 0x70, 0x3e, 0x86, 0x18, 0x61, 0x86, 0x10,

    /* U+00F2 "ò" */
    0x60, 0x60, 0x3e, 0x86, 0x18, 0x61, 0x85, 0xf0,

    /* U+00F3 "ó" */
    0x19, 0x80, 0x3e, 0x86, 0x18, 0x61, 0x85, 0xf0,

    /* U+00F4 "ô" */
    0x31, 0x20, 0x3e, 0x86, 0x18, 0x61, 0x85, 0xf0,

    /* U+00F5 "õ" */
    0x43, 0xe0, 0x3e, 0x86, 0x18, 0x61, 0x85, 0xf0,

    /* U+00F6 "ö" */
    0x48, 0xf, 0xa1, 0x86, 0x18, 0x61, 0x7c,

    /* U+00F7 "÷" */
    0x20, 0x3e, 0x2, 0x0,

    /* U+00F8 "ø" */
    0xc, 0x20, 0xbe, 0x96, 0xda, 0x69, 0xe5, 0xf4,
    0x0,

    /* U+00F9 "ù" */
    0x60, 0x60, 0x21, 0x86, 0x18, 0x61, 0x85, 0xf0,

    /* U+00FA "ú" */
    0x19, 0x80, 0x21, 0x86, 0x18, 0x61, 0x85, 0xf0,

    /* U+00FB "û" */
    0x31, 0x20, 0x21, 0x86, 0x18, 0x61, 0x85, 0xf0,

    /* U+00FC "ü" */
    0x69, 0xa0, 0x21, 0x86, 0x18, 0x61, 0x85, 0xf0,

    /* U+00FD "ý" */
    0x19, 0xc0, 0x21, 0x86, 0x18, 0x61, 0x85, 0xf0,
    0x42, 0xf0,

    /* U+00FE "þ" */
    0x82, 0x8, 0x3e, 0x86, 0x18, 0x61, 0x87, 0xf8,
    0x20, 0x80,

    /* U+00FF "ÿ" */
    0x69, 0xa8, 0x61, 0x86, 0x18, 0x71, 0x7c, 0x10,
    0xbe
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 125, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 125, .box_w = 1, .box_h = 9, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 125, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 5, .adv_w = 125, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 14, .adv_w = 125, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 26, .adv_w = 125, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 37, .adv_w = 125, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 46, .adv_w = 125, .box_w = 1, .box_h = 4, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 47, .adv_w = 125, .box_w = 4, .box_h = 14, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 54, .adv_w = 125, .box_w = 4, .box_h = 14, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 61, .adv_w = 125, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 65, .adv_w = 125, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 69, .adv_w = 125, .box_w = 2, .box_h = 5, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 71, .adv_w = 125, .box_w = 5, .box_h = 1, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 72, .adv_w = 125, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 125, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 83, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 104, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 125, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 126, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 133, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 140, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 147, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 154, .adv_w = 125, .box_w = 2, .box_h = 7, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 125, .box_w = 3, .box_h = 11, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 161, .adv_w = 125, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 166, .adv_w = 125, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 169, .adv_w = 125, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 174, .adv_w = 125, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 182, .adv_w = 125, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 192, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 199, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 206, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 213, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 241, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 248, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 255, .adv_w = 125, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 262, .adv_w = 125, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 270, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 277, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 284, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 291, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 298, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 305, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 313, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 320, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 327, .adv_w = 125, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 335, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 342, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 349, .adv_w = 125, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 357, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 364, .adv_w = 125, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 372, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 379, .adv_w = 125, .box_w = 4, .box_h = 13, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 386, .adv_w = 125, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 396, .adv_w = 125, .box_w = 4, .box_h = 13, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 403, .adv_w = 125, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 407, .adv_w = 125, .box_w = 7, .box_h = 1, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 408, .adv_w = 125, .box_w = 4, .box_h = 2, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 409, .adv_w = 125, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 415, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 423, .adv_w = 125, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 429, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 437, .adv_w = 125, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 443, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 450, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 458, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 466, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 475, .adv_w = 125, .box_w = 4, .box_h = 11, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 481, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 489, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 497, .adv_w = 125, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 504, .adv_w = 125, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 510, .adv_w = 125, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 516, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 524, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 532, .adv_w = 125, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 539, .adv_w = 125, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 545, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 553, .adv_w = 125, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 559, .adv_w = 125, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 565, .adv_w = 125, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 572, .adv_w = 125, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 578, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 586, .adv_w = 125, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 592, .adv_w = 125, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 601, .adv_w = 125, .box_w = 1, .box_h = 12, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 603, .adv_w = 125, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 612, .adv_w = 125, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 614, .adv_w = 125, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 615, .adv_w = 125, .box_w = 1, .box_h = 9, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 617, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 626, .adv_w = 125, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 634, .adv_w = 125, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 639, .adv_w = 125, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 647, .adv_w = 125, .box_w = 1, .box_h = 11, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 649, .adv_w = 125, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 656, .adv_w = 125, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 657, .adv_w = 125, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 664, .adv_w = 125, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 670, .adv_w = 125, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 675, .adv_w = 125, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 678, .adv_w = 125, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 682, .adv_w = 125, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = 9},
    {.bitmap_index = 683, .adv_w = 125, .box_w = 3, .box_h = 3, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 685, .adv_w = 125, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 690, .adv_w = 125, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 692, .adv_w = 125, .box_w = 3, .box_h = 4, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 694, .adv_w = 125, .box_w = 4, .box_h = 2, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 695, .adv_w = 125, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 705, .adv_w = 125, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = 3},
    {.bitmap_index = 706, .adv_w = 125, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 707, .adv_w = 125, .box_w = 3, .box_h = 4, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 709, .adv_w = 125, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 715, .adv_w = 125, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 720, .adv_w = 125, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 732, .adv_w = 125, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 744, .adv_w = 125, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 756, .adv_w = 125, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 764, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 773, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 782, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 791, .adv_w = 125, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 801, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 810, .adv_w = 125, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 820, .adv_w = 125, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 829, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 838, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 847, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 856, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 865, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 874, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 883, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 892, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 901, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 910, .adv_w = 125, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 918, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 927, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 936, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 945, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 954, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 963, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 972, .adv_w = 125, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 974, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 983, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 992, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1001, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1010, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1019, .adv_w = 125, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1030, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1038, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1045, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1053, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1061, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1069, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1077, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1085, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1094, .adv_w = 125, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1101, .adv_w = 125, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1108, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1116, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1124, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1132, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1140, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1149, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1158, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1167, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1175, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1184, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1192, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1200, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1208, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1216, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1224, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1231, .adv_w = 125, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1235, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1244, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1252, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1260, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1268, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1276, .adv_w = 125, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1286, .adv_w = 125, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1296, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3}
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
        .range_start = 174, .range_length = 7, .glyph_id_start = 109,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 182, .range_length = 74, .glyph_id_start = 116,
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
    .cmap_num = 4,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};

extern const lv_font_t lv_font_montserrat_14;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t font_kode_14 = {
#else
lv_font_t font_kode_14 = {
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
    .fallback = &lv_font_montserrat_14,
#endif
    .user_data = NULL,
};



#endif /*#if FONT_KODE_14*/

