#ifndef SCREEN_INFO
#define SCREEN_INFO
#include <Base.h>

typedef struct {
    UINT8  orig_x;           /* 0x00 */
    UINT8  orig_y;           /* 0x01 */
    UINT16 ext_mem_k;        /* 0x02 */
    UINT16 orig_video_page;  /* 0x04 */
    UINT8  orig_video_mode;  /* 0x06 */
    UINT8  orig_video_cols;  /* 0x07 */
    UINT8  flags;            /* 0x08 */
    UINT8  unused2;          /* 0x09 */
    UINT16 orig_video_ega_bx;/* 0x0a */
    UINT16 unused3;          /* 0x0c */
    UINT8  orig_video_lines; /* 0x0e */
    UINT8  orig_video_isVGA; /* 0x0f */
    UINT16 orig_video_points;/* 0x10 */
    
    /* VESA graphic mode -- linear frame buffer */
    UINT16 lfb_width;        /* 0x12 */
    UINT16 lfb_height;       /* 0x14 */
    UINT16 lfb_depth;        /* 0x16 */
    UINT32 lfb_base;         /* 0x18 */
    UINT32 lfb_size;         /* 0x1c */
    UINT16 cl_magic;         /* 0x20 */
    UINT16 cl_offset;        /* 0x22 */
    UINT16 lfb_linelength;   /* 0x24 */
    UINT8  red_size;         /* 0x26 */
    UINT8  red_pos;          /* 0x27 */
    UINT8  green_size;       /* 0x28 */
    UINT8  green_pos;        /* 0x29 */
    UINT8  blue_size;        /* 0x2a */
    UINT8  blue_pos;         /* 0x2b */
    UINT8  rsvd_size;        /* 0x2c */
    UINT8  rsvd_pos;         /* 0x2d */
    UINT16 vesapm_seg;       /* 0x2e */
    UINT16 vesapm_off;       /* 0x30 */
    UINT16 pages;            /* 0x32 */
    UINT16 vesa_attributes;  /* 0x34 */
    UINT32 capabilities;     /* 0x36 */
    UINT8  _reserved[6];     /* 0x3a */
} screen_info;

#endif