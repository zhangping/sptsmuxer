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

#ifndef __SPTS_H__
#define __SPTS_H__

#include <glib.h>
#include "psi.h"
#include "pes.h"

G_BEGIN_DECLS

#define PROGRAM_NUMBER 0x16
#define PMT_PID 0x20
#define VIDEO_PID 0x0040
#define AUDIO_PID 0X0041

#define CLOCK_BASE 90000LL /* 90kHz */
#define SYS_CLOCK_FREQ (27000000L) /* 27MHz */
#define DEFAULT_PCR_INTERVAL (SYS_CLOCK_FREQ / 25) /* 40 ms */

/* adaptation field flag */
//#define DISCONTINUITY_INDICATOR 0x80
//#define RANDOM_ACCESS_INDICATOR 0x40
//#define ELEMENTARY_STREAM_PRIORITY_INDICATOR 0x20
#define PCR_FLAG 0x10
//#define OPCR_FLAG 0x08
//#define SPLICING_POINT_FLAG 0x04
//#define TRANSPORT_PRIVATE_DATA_FLAG 0x02
//#define ADAPTATION_FIELD_EXTENSION_FLAG 0x01
#define TSPACKET_HEADER_LENGTH 4
#define TSPACKET_PAYLOAD_LENGTH 184
#define TSPACKET_LENGTH 188
#define SCRAMBLING_CTL_NOT_SCRAMBLED 0
#define ADAPTATION_FIELD_PAYLOAD_ONLY 0x1 << 4
#define ADAPTATION_FIELD_ADAPTATION_ONLY 0x1 << 5
#define ADAPTATION_FIELD_ADAPTATION_AND_PAYLOAD 0x3 << 4

#define GSTTIME_TO_MPEGTIME(time) (gst_util_uint64_scale ((time), CLOCK_BASE, GST_SECOND))

typedef struct _TsPacket TsPacket;
typedef struct _TsPacketHeaderInfo TsPacketHeaderInfo;
typedef struct _TsPacketAdaptationInfo TsPacketAdaptationInfo;
typedef struct _SpTsStream SpTsStream;
typedef struct _SpTs SpTs;

typedef gboolean (*SpTsMuxerWriteFunc) (guint8 *data, guint len, void *user_data);

struct _TsPacketHeaderInfo {
  /* TS Packet Header, total four bytes
  0100 0111 .... .... .... .... .... .... Sync Byte: 0x47
  .... .... 0... .... .... .... .... .... Transport Error Indicator: 0
  .... .... .1.. .... .... .... .... .... Payload Unit start Indicator: 0
  .... .... ..0. .... .... .... .... .... Transport Priority: 0
  .... .... ...0 0000 0000 0000 .... .... PAT PID: 0x00
  .... .... .... .... .... .... 00.. .... Transport Scrambling Control: Not scrambled: 00
  .... .... .... .... .... .... ..01 .... Adaptation field control, No adaptation_field, payload only: 01
  .... .... .... .... .... .... .... 0000 Continuity Counter: 0000
  */
  guint8 transport_error_indicator;
  guint8 payload_unit_start_indicator;
  guint8 transport_priority;
  guint16 PID;
  guint8 transport_scrambling_control;
  guint8 adaptation_field_control;
  guint8 continuity_counter;
};

struct _TsPacket {
  gboolean pcr_reference;
  guint8 data[TSPACKET_LENGTH];
};

struct _TsPacketAdaptationInfo {
  /* Adaptation Field Flag
  x... .... discontinuity_indicator
  .x.. .... random_access_indicator
  ..x. .... elementary_stream_priority_indicator
  ...x .... PCR_flag
  .... x... OPCR_flag
  .... .x.. splicing_point_flag
  .... ..x. transport_private_data_flag
  .... ...x adaptation_field_extension_flag
  */
  guint8 field_flag;
  /* adaptation field length include stuffing_bytes. 0 means no stuffing_bytes */
  guint8 adaptation_field_length;
  GstClockTime pcr;
};

struct _SpTsStream {
  TsPacketHeaderInfo tspacket_header_info;
  guint8 tspacket[TSPACKET_LENGTH];
  gboolean pcr_reference; /* is the stream pcr reference? */
  guint64 packets_counter; /* counter of ts packets of the stream */
  guint64 last_gop_packets_counter; /* counter of ts packets from begain to last gop */
  guint64 max_gop_packets; /* max gop ts packets */
  guint64 frames_counter; /* frames counter of the stream */
  guint64 last_gop_frames_counter; /* counter of frames from begain to last gop */
  PESHeaderInfo pes_header_info;
};

/* single program transport stream data structure */
struct _SpTs {
  guint16 transport_id;

  /* pmt and pat is together */
  TsPacket *pat;
  TsPacket *pmt_tspacket;
  guint8 nullpacket[TSPACKET_LENGTH];
  PsiPmtSection pmt_section;
  SpTsStream *audio_stream;
  SpTsStream *video_stream;
  gint64 last_psi_ts; /* time stamp of the last pat and pmt. */
  guint64 packets_counter; /* all packets counter */
  guint64 gop_counter; /* gop counter */

  gint64 last_pcr_ts; /* time stamp of the last pcr. */
  guint pcr_interval; /* default is 40ms, the same as dvb standard, conversion to ts packets counter according to bitrate */
  guint psi_interval; /* per gop, conversion to ts packets counter according to bitrate */

  SpTsMuxerWriteFunc write_func;
  void *write_func_data;

  guint muxrate; /* CBR mux rate, shall be integer multiple of 188(bytes)*8(bits) = 1504, 0 means VBR */
};

/* create new single program transport stream */
SpTs * spts_new (void);

/* add audio or video stream to transport stream */
void spts_add_stream (SpTs *spts, guint8 stream_type, guint16 stream_pid);

/* set the write function and write function data */
void spts_set_write_func (SpTs *spts, SpTsMuxerWriteFunc write_func, void *write_func_data);

/* write a frame of elementary stream */
gboolean spts_write_frame (SpTs *spts, GstBuffer *buffer);

/* free single program transport stream */
gboolean spts_release (SpTs *spts);

G_END_DECLS

#endif /* __SPTS_H__ */
