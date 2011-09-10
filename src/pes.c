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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "pes.h"

guint pes_header_set (guint8 *pesheader, PESHeaderInfo pesheaderinfo)
{
  guint8 *buf = pesheader;
  guint64 val;

  /* start code */
  *buf++ = 0x00;
  *buf++ = 0x00;
  *buf++ = 0x01;
  
  *buf++ = pesheaderinfo.stream_id;

  /* pes packet length */
  if (pesheaderinfo.stream_id == STREAM_ID_VIDEO) {
    /* video stream, set 0 */
    *buf++ = 0;
    *buf++ = 0;
  } else {
    pesheaderinfo.pes_packet_length += 8;
    *buf++ = pesheaderinfo.pes_packet_length >> 8  & 0xff;
    *buf++ = pesheaderinfo.pes_packet_length & 0xff;
  }

  *buf++ = pesheaderinfo.pes_header_flag >> 8 & 0xff;
  *buf++ = pesheaderinfo.pes_header_flag & 0xff;

  *buf++ = pesheaderinfo.pes_header_data_length;

  *buf++ = 0x20 | (((pesheaderinfo.pts >> 30) & 0x07) << 1) | 0x01;
  val = (((pesheaderinfo.pts >> 15) & 0x7ffff) << 1) | 1;
  *buf++ = val >> 8;
  *buf++ = val;
  val = (((pesheaderinfo.pts) & 0x7fff) << 1) | 1;
  *buf++ = val >> 8;
  *buf++ = val;

  return buf - pesheader;
}

/*fixme: is this function is necessary */
guint pes_header_length (PESHeaderInfo pes_header_info)
{
  return 14;
}
