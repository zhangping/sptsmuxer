/*
 * Copyright (C) 2011 zhangping <dqzhangp@163.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __PES_H__
#define __PES_H__

#include <glib.h>

G_BEGIN_DECLS

/* stream id, used in pes header */
#define STREAM_ID_AUDIO 0xc0
#define STREAM_ID_VIDEO 0xe0

/* PES Header flags */
#define PES_SCRAMBLING_CONTROL_NOT_SCRAMBLED 0x0000
#define PES_PRIORITY_1 0x0000

typedef enum ElementaryStreamType ElementaryStreamType;
typedef struct _PESHeaderInfo PESHeaderInfo;

/* Elementary Stream Type, used in pmt table */
enum ElementaryStreamType {
  ELEMENT_STREAM_TYPE_RESERVED = 0x00,

  /* video stream type */
  ELEMENT_STREAM_TYPE_VIDEO_H264 = 0x1b,

  /* audio stream type */
  ELEMENT_STREAM_TYPE_AUDIO_MPEG1 = 0x03,
  ELEMENT_STREAM_TYPE_AUDIO_MPEG2 = 0x04,
  ELEMENT_STREAM_TYPE_AUDIO_AAC = 0x0f
};

struct _PESHeaderInfo {
  guint8 stream_id;
  guint pes_packet_length;
  /* PES Header Flag
  10.. .... .... .... 10
  ..xx .... .... .... PES_scrambling_control 
  .... x... .... .... PES_priority
  .... .x.. .... .... data_alignment_indicator
  .... ..x. .... .... copyright
  .... ...x .... .... original_or_copy
  .... .... xx.. .... PTS_DTS_flag
  .... .... ..x. .... ESCR_flag
  .... .... ...x .... ES_rate_flag
  .... .... .... x... DSM_trick_mode_flag
  .... .... .... .x.. additional_copy_info_flag
  .... .... .... ..x. PES_CRC_flag
  .... .... .... ...x PES_extension_flag
  */
  guint16 pes_header_flag;
  guint8 pes_header_data_length;
  guint64 pts;
  guint64 dts;
};

guint pes_header_set (guint8 *pesheader, PESHeaderInfo pesheaderinfo);

guint pes_header_length (PESHeaderInfo pes_header_info);

G_END_DECLS

#endif /* __PES_H__ */
