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

#include "psi.h"
#include "crc.h"

GST_DEBUG_CATEGORY_EXTERN (sptsmuxer_debug);
#define GST_CAT_DEFAULT sptsmuxer_debug

void psi_section_header_set (guint8 *psi, PsiSectionHeadInfo psisectionheadinfo)
{
  guint8 *buf;

  buf = psi;
   /* pointer_field */
  *buf++ = 0;

  /* table_id, one bytes */
  *buf++ = psisectionheadinfo.table_id;

  /*
  two bytes
  section_syntax_indicator 1 bit
  0 1 bit
  reserved 2 bits
  section_length 12 bits
  */
  *buf++ = 0xb0 | (psisectionheadinfo.section_length >> 8);
  *buf++ = 0xff & psisectionheadinfo.section_length;

  /* transport_stream_id, two bytes */
  *buf++ = psisectionheadinfo.id >> 8;
  *buf++ = 0xff & psisectionheadinfo.id;

  /*
  one bytes
  reserved 2 bits
  version_number 5 bits
  current_next_indicator 1 bit
  */
  *buf++ = 0xc0 | (psisectionheadinfo.version_number << 1) | 0x01;

  /*
  two bytes
  section_number one bytes
  last_section_number one bytes
  */
  *buf++ = psisectionheadinfo.section_number;
  *buf++ = psisectionheadinfo.last_section_number;
}

void psi_pat_section_set (guint8 *pat_section, guint16 program_number, guint16 pmt_pid)
{
  PsiSectionHeadInfo psi_section_header_info;
  guint8 *buf = pat_section;
  guint32 crc;

  psi_section_header_info.table_id = 0;
  psi_section_header_info.section_length = 0x0d;
  psi_section_header_info.id = 0x01;
  psi_section_header_info.version_number = 0x0;
  psi_section_header_info.section_number = 0x0;
  psi_section_header_info.last_section_number = 0x0;
  psi_section_header_set (buf, psi_section_header_info);

   buf += PSI_SECTION_HEADER_LENGTH + 1; /* plus one byte pointer field */
  /* program_number two bytes */
  *buf++ = program_number >> 8;
  *buf++ = 0xff & program_number;

  /*
  two bytes
  reserved 3 bits
  program_map_PID 13 bits
  */
  *buf++ = 0xe0 | pmt_pid >> 8;
  *buf++ = 0xff & pmt_pid;

  /* crc 4 bytes */
  crc = calc_crc32 (pat_section + 1 /* 1 byte pointer_field*/, buf - pat_section - 1);
  *buf++ = (crc >> 24) & 0xff;
  *buf++ = (crc >> 16) & 0xff;
  *buf++ = (crc >> 8) & 0xff;
  *buf++ = crc & 0xff;
}

void psi_pmt_section_init (PsiPmtSection *pmt_section, guint16 program_number)
{
  PsiSectionHeadInfo psi_section_header_info;
  guint8 *buf = pmt_section->data;

  psi_section_header_info.table_id = 0x02;
  psi_section_header_info.section_length = 0x0d;
  psi_section_header_info.id = program_number;
  psi_section_header_info.version_number = 0x0;
  psi_section_header_info.section_number = 0x0;
  psi_section_header_info.last_section_number = 0x0;
  psi_section_header_set (buf, psi_section_header_info);
 
  buf += PSI_SECTION_HEADER_LENGTH + 1 /* plus one byte pointer field */;
  /*
  two bytes
  reserved 3 bits
  PCR_PID 13 bits
  */
  *buf++ = 0xe0;
  *buf++ = 0x00;

  /*
  two bytes
  reserved 4 bits
  program_info_length 12 bits
  */
  *buf++ = 0xf0;
  *buf++ = 0x00;

  pmt_section->section_info.section_length = 0x0d;
  pmt_section->section_info.program_number = program_number;
  pmt_section->section_info.program_info_length = 0;
  pmt_section->section_info.PCR_PID = 0;
  pmt_section->section_info.video_pid = 0;
  pmt_section->section_info.video_stream_type = STREAM_TYPE_RESERVED;
  pmt_section->section_info.audio_pid = 0;
  pmt_section->section_info.audio_stream_type = STREAM_TYPE_RESERVED;
}

void psi_pmt_section_pcr_pid_set (PsiPmtSection *pmt_section, guint16 pcr_pid)
{
  guint8 *buf = pmt_section->data + PSI_SECTION_HEADER_LENGTH + 1;
  guint32 crc;

  GST_DEBUG ("psi_pmt_section_pcr_pid_set %d", pcr_pid);

  pmt_section->section_info.PCR_PID = pcr_pid;
  /*
  two bytes
  reserved 3 bits
  PCR_PID 13 bits
  */
  *buf++ = 0xe0 | pcr_pid >> 8;
  *buf++ = 0xff & pcr_pid;

  /* recalculate CRC */
  crc = calc_crc32 (pmt_section->data + 1 /* one bytes pointer_field*/, pmt_section->section_info.section_length - 1);
  buf = pmt_section->data + pmt_section->section_info.section_length;
  *buf++ = (crc >> 24) & 0xff;
  *buf++ = (crc >> 16) & 0xff;
  *buf++ = (crc >> 8) & 0xff;
  *buf++ = crc & 0xff;
}

void psi_pmt_section_stream_add (PsiPmtSection *pmt_section, guint8 stream_type, guint16 stream_pid)
{
  guint8 *buf;
  guint32 crc;

  GST_DEBUG ("psi_pmt_section_stream_set type:%d, pid:%d", stream_type, stream_pid);

  /* reset section length */
  pmt_section->section_info.section_length += 5;
  buf = pmt_section->data + 2;
  *buf++ = 0xb0 | (pmt_section->section_info.section_length >> 8);
  *buf++ = 0xff & pmt_section->section_info.section_length;

  buf = pmt_section->data + pmt_section->section_info.section_length - 5;
  switch (stream_type) {
  case STREAM_TYPE_VIDEO_H264:
    *buf++ = STREAM_TYPE_VIDEO_H264;
    pmt_section->section_info.video_pid = stream_pid;
    pmt_section->section_info.video_stream_type = stream_type;
    break;
  case STREAM_TYPE_AUDIO_AAC:
    *buf++ = STREAM_TYPE_AUDIO_AAC;
    pmt_section->section_info.audio_pid = stream_pid;
    pmt_section->section_info.audio_stream_type = stream_type;
    break;
  }
  *buf++ = 0xe0 | (stream_pid >> 8);
  *buf++ = 0xff & stream_pid;
  *buf++ = 0xf0;
  *buf++ = 0;

  /* calculate CRC */
  crc = calc_crc32 (pmt_section->data + 1 /* one bytes pointer_field*/, pmt_section->section_info.section_length - 1);
  *buf++ = (crc >> 24) & 0xff;
  *buf++ = (crc >> 16) & 0xff;
  *buf++ = (crc >> 8) & 0xff;
  *buf++ = crc & 0xff;
}

