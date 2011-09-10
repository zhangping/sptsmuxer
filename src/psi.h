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

#ifndef __PSI_H__
#define __PSI_H__

#include <glib.h>

G_BEGIN_DECLS

#define PAT_PID 0x0
#define PSI_SECTION_HEADER_LENGTH 8
#define STREAM_TYPE_RESERVED 0x00;
#define STREAM_TYPE_VIDEO_H264 0x1b
#define STREAM_TYPE_AUDIO_AAC 0x0f

typedef struct _PsiSectionHeadInfo PsiSectionHeadInfo;
typedef struct _PsiPmtSectionInfo PsiPmtSectionInfo;
typedef struct _PsiPmtSection PsiPmtSection;

struct _PsiSectionHeadInfo {
  guint8 pointer_field;
  /* PAT or PMT Section Header, total 8 bytes
  0000 0000 .... .... .... .... .... .... .... .... .... .... .... .... .... .... Table id: 0x0
  .... .... 1... .... .... .... .... .... .... .... .... .... .... .... .... .... Section Syntax Indicator: 1
  .... .... .0.. .... .... .... .... .... .... .... .... .... .... .... .... .... '0' 
  .... .... ..11 .... .... .... .... .... .... .... .... .... .... .... .... .... Reserved
  .... .... .... 0000 0000 0100 .... .... .... .... .... .... .... .... .... .... Section Length: 4
  .... .... .... .... .... .... 0000 0000 0000 0001 .... .... .... .... .... .... Transport Stream id or Program Number 
  .... .... .... .... .... .... .... .... .... .... 11.. .... .... .... .... .... Reserved
  .... .... .... .... .... .... .... .... .... .... ..00 000. .... .... .... .... Version Number
  .... .... .... .... .... .... .... .... .... .... .... ...1 .... .... .... .... Current Next Indicator
  .... .... .... .... .... .... .... .... .... .... .... .... 0000 0000 .... .... Section Number
  .... .... .... .... .... .... .... .... .... .... .... .... .... .... 0000 0000 Last Section Number
  */
  guint8 table_id;
  guint16 section_length;
  guint16 id; /* transport_stream_id for pat or program_number for pmt */
  guint8 version_number;
  guint8 section_number;
  guint8 last_section_number;
};

struct _PsiPmtSectionInfo {
  guint16 section_length;
  guint16 program_number;
  guint16 PCR_PID;
  guint16 program_info_length;
  guint16 video_pid; /* 0 means no video stream */
  guint8 video_stream_type;
  guint16 audio_pid; /* 0 means no audio stream */
  guint8 audio_stream_type;
};

struct _PsiPmtSection {
  PsiPmtSectionInfo section_info;
  guint8 *data;
};

void psi_section_header_set (guint8 *psi, PsiSectionHeadInfo psisectionheadinfo);
void psi_pat_section_set (guint8 *pat_section, guint16 program_number, guint16 pmt_pid);
void psi_pmt_section_init (PsiPmtSection *pmt_section, guint16 program_number);
void psi_pmt_section_pcr_pid_set (PsiPmtSection *pmt_section, guint16 pcr_pid);
void psi_pmt_section_stream_add (PsiPmtSection *pmt_section, guint8 stream_type, guint16 stream_pid);

G_END_DECLS

#endif /* __PSI_H__ */
