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

#include <string.h>
#include <gst/gst.h>

#include "spts.h"

GST_DEBUG_CATEGORY_EXTERN (sptsmuxer_debug);
#define GST_CAT_DEFAULT sptsmuxer_debug

/* return a new spts tspacket */
static TsPacket *spts_tspacket_new (TsPacketHeaderInfo tspacket_header_info);

/* ts packet header initialize */
static void spts_tspacket_header_init (guint8 *tspacket, TsPacketHeaderInfo tspacket_header_info);

/* release tspacket */
static void spts_tspacket_release (TsPacket *tspacket);

/* increase continuity counter */
static void spts_tspacket_header_cc_inc (guint8 *tspacket);

static void spts_tspacket_header_set_start_unit_indicator (guint8 *tspacket, guint8 start_unit_indicator);

/* set ts packet header adaptation field control */
static void spts_tspacket_header_set_adaptation_field_control (guint8 *tspacket, guint8 adaptation_field_control);

/* set ts packet adaptation */
static guint spts_tspacket_adaptation_set (guint8 *tspacket, TsPacketAdaptationInfo adaptationinfo);

/* initialize pat of single program transport stream */
static void spts_psi_pat_init (SpTs *spts);

/* initialize pmt: ts packet header and section header */
static void spts_psi_pmt_init (SpTs *spts);

/* null packet initialize */
static void spts_null_packet_init (SpTs *spts);

/* write out psi packet, pat and pmt */
static void spts_psi_write (SpTs *spts);

/* write out pcr only packet */
static void spts_pcronly_write (SpTs *spts, GstClockTime pcr);

SpTs * spts_new (void)
{
  SpTs *spts;

  spts = g_slice_new0 (SpTs);

  spts->pcr_interval = DEFAULT_PCR_INTERVAL;
  spts->last_pcr_ts = -1;
  spts->psi_interval = 1000;

  spts_psi_pat_init (spts);
  spts_psi_pmt_init (spts);

  spts->audio_stream = NULL;
  spts->video_stream = NULL;
  spts_null_packet_init (spts);

  return spts;
}

static void spts_tspacket_header_init (guint8 *tspacket, TsPacketHeaderInfo tspacket_header_info)
{
  guint8 *buf;

  buf = tspacket;
  memset (buf, 0xff, TSPACKET_LENGTH);

  /* one byte sync */
  *buf++ = 0x47;

  /*
  two bytes
  transport_error_indicator 1 bit value is 0
  payload_unit_start_indicator 1 bit value is
  transport_priority 1 bit value is 
  PID 13 bit, value is 0
  */
  //*buf++ = 0x60 | (tspacket_header_info.PID >> 8);
  *buf++ = tspacket_header_info.payload_unit_start_indicator |
           tspacket_header_info.transport_priority |
           (tspacket_header_info.PID >> 8);
  *buf++ = 0xff & tspacket_header_info.PID;

  /*
  one byte
  transport_scrambling_control 2 bit value is 00
  adaptation_field_control 2 bit value is 01
  continuity_counter 4 bit value is 0000
  */
  *buf++ = tspacket_header_info.transport_scrambling_control |
           tspacket_header_info.adaptation_field_control |
           tspacket_header_info.continuity_counter;
}

static TsPacket *spts_tspacket_new (TsPacketHeaderInfo tspacket_header_info)
{
  TsPacket *tspacket;
  guint8 *buf;

  tspacket = g_slice_new0 (TsPacket);
  buf = tspacket->data;
  memset (buf, 0xff, TSPACKET_LENGTH);

  /* one byte sync */
  *buf++ = 0x47;

  /*
  two bytes
  transport_error_indicator 1 bit value is 0
  payload_unit_start_indicator 1 bit value is
  transport_priority 1 bit value is 
  PID 13 bit, value is 0
  */
  //*buf++ = 0x60 | (tspacket_header_info.PID >> 8);
  *buf++ = tspacket_header_info.payload_unit_start_indicator |
           tspacket_header_info.transport_priority |
           (tspacket_header_info.PID >> 8);
  *buf++ = 0xff & tspacket_header_info.PID;

  /*
  one byte
  transport_scrambling_control 2 bit value is 00
  adaptation_field_control 2 bit value is 01
  continuity_counter 4 bit value is 0000
  */
  *buf++ = tspacket_header_info.transport_scrambling_control |
           tspacket_header_info.adaptation_field_control |
           tspacket_header_info.continuity_counter;

  return tspacket;
}

static void spts_tspacket_release (TsPacket *tspacket)
{
  g_slice_free (TsPacket, tspacket);
}

static void spts_tspacket_header_set_start_unit_indicator (guint8 *tspacket, guint8 start_unit_indicator)
{
  guint8 *buf = tspacket + 1;

  *buf &= 0xbf;
  *buf |= start_unit_indicator;
}

static void spts_tspacket_header_set_adaptation_field_control (guint8 *tspacket, guint8 adaptation_field_control)
{
  guint8 *buf = tspacket + 3;

  *buf &= 0xcf;
  *buf |= adaptation_field_control;
}

static void spts_tspacket_header_cc_inc (guint8 *tspacket)
{
  guint8 *buf = tspacket + 3;
  guint8 cc = (*buf + 1) & 0x0f;

  *buf &= 0xf0;
  *buf |= cc;
}

static guint spts_tspacket_adaptation_set (guint8 *tspacket, TsPacketAdaptationInfo adaptationinfo)
{
  guint8 *buf = tspacket + TSPACKET_HEADER_LENGTH;
  guint64 pcr_base, pcr_ext;
  guint8 stuffing_bytes_length;

  GST_DEBUG ("adaptation, field_flag: %02x, length: %d", adaptationinfo.field_flag, adaptationinfo.adaptation_field_length);
  /* adaptation field length */
  *buf++ = adaptationinfo.adaptation_field_length;
  *buf++ = adaptationinfo.field_flag;

  if (adaptationinfo.field_flag & PCR_FLAG) {
    GST_DEBUG ("adaptation: pcr");
    pcr_base = adaptationinfo.pcr / 300;
    pcr_ext = adaptationinfo.pcr % 300;
    *buf++ = pcr_base >> 25;
    *buf++ = pcr_base >> 17;
    *buf++ = pcr_base >> 9;
    *buf++ = pcr_base >> 1;
    *buf++ = pcr_base << 7 | pcr_ext >> 8 | 0x7e;
    *buf++ = pcr_ext & 0xff;
  }

  /* stuffing bytes */
  if (adaptationinfo.adaptation_field_length > 0) {
    stuffing_bytes_length = adaptationinfo.adaptation_field_length - (buf - tspacket - TSPACKET_HEADER_LENGTH - 1);
    if (stuffing_bytes_length > 0) {
      memset (buf, 0xff, stuffing_bytes_length);
      buf += stuffing_bytes_length;
    }
  }

  return buf - tspacket - TSPACKET_HEADER_LENGTH;
}

static void spts_psi_pat_init (SpTs *spts)
{
  TsPacketHeaderInfo tspacket_header_info;

  tspacket_header_info.payload_unit_start_indicator = 0x40;
  tspacket_header_info.transport_priority = 0x20;
  tspacket_header_info.PID = PAT_PID;
  tspacket_header_info.transport_scrambling_control = SCRAMBLING_CTL_NOT_SCRAMBLED;
  tspacket_header_info.adaptation_field_control = ADAPTATION_FIELD_PAYLOAD_ONLY;
  tspacket_header_info.continuity_counter = 0x0;
  spts->pat = spts_tspacket_new (tspacket_header_info);
  psi_pat_section_set (spts->pat->data + TSPACKET_HEADER_LENGTH, PROGRAM_NUMBER, PMT_PID);
}

static void spts_psi_pmt_init (SpTs *spts)
{
  TsPacketHeaderInfo tspacket_header_info;

  tspacket_header_info.payload_unit_start_indicator = 0x40;
  tspacket_header_info.transport_priority = 0x20;
  tspacket_header_info.PID = PMT_PID;
  tspacket_header_info.transport_scrambling_control = SCRAMBLING_CTL_NOT_SCRAMBLED;
  tspacket_header_info.adaptation_field_control = ADAPTATION_FIELD_PAYLOAD_ONLY;
  tspacket_header_info.continuity_counter = 0x0;
  spts->pmt_tspacket = spts_tspacket_new (tspacket_header_info);
  spts->pmt_section.data = spts->pmt_tspacket->data + TSPACKET_HEADER_LENGTH;
  psi_pmt_section_init (&(spts->pmt_section), PROGRAM_NUMBER);
}

static void spts_null_packet_init (SpTs *spts)
{
  guint8 *buf;

  buf = spts->nullpacket;
  *buf++ = 0x47;
  *buf++ = 0x1f;
  *buf++ = 0xff;
  *buf++ = 0x10;
  memset (buf, 0xff, TSPACKET_LENGTH-4);
}

static void spts_psi_write (SpTs *spts)
{
  spts->write_func (spts->pat->data, TSPACKET_LENGTH, spts->write_func_data);
  spts_tspacket_header_cc_inc (spts->pat->data);
  spts->packets_counter++;
  spts->write_func (spts->pmt_tspacket->data, TSPACKET_LENGTH, spts->write_func_data);
  spts_tspacket_header_cc_inc (spts->pmt_tspacket->data);
  spts->packets_counter++;
}

void spts_add_stream (SpTs *spts, guint8 stream_type, guint16 stream_pid)
{
  SpTsStream *stream;

  stream = g_slice_new0 (SpTsStream);
  stream->tspacket_header_info.payload_unit_start_indicator = 0x40;
  stream->tspacket_header_info.transport_priority = 0x20;
  stream->tspacket_header_info.transport_scrambling_control = SCRAMBLING_CTL_NOT_SCRAMBLED;
  stream->tspacket_header_info.adaptation_field_control = ADAPTATION_FIELD_PAYLOAD_ONLY;
  stream->tspacket_header_info.continuity_counter = 0x0;
  stream->pes_header_info.pes_header_data_length = 0x05;
  stream->pes_header_info.pes_header_flag = 0x8080;
  stream->packets_counter = 0;
  stream->last_gop_packets_counter = 0;
  stream->max_gop_packets = 0;
  stream->frames_counter = 0;
  stream->last_gop_frames_counter = 0;

  if (stream_type == STREAM_TYPE_VIDEO_H264) {
    stream->pes_header_info.stream_id = STREAM_ID_VIDEO;
    stream->tspacket_header_info.PID = VIDEO_PID;
    spts->video_stream = stream;
  } else {
    stream->pes_header_info.stream_id = STREAM_ID_AUDIO;
    stream->tspacket_header_info.PID = AUDIO_PID;
    spts->audio_stream = stream;
  }

  spts_tspacket_header_init (stream->tspacket, stream->tspacket_header_info);
  //spts->audio = spts_tspacket_new (spts->audio_tspacket_header_info);
  //spts->audio->pcr_reference = FALSE;
}

static void spts_pcronly_write (SpTs *spts, GstClockTime pcr)
{
  /*should increase the video_counter and packets counter cause the pid of video and pcr is the same.*/
#if 0
  spts->packets_counter++;
  spts->video_packets_counter++;
  spts_tspacket_header_cc_inc (spts->pcr->data);
  spts->tspacket_adaptation_info.pcr = pcr; 
  spts_tspacket_adaptation_set (spts->pcr->data, spts->tspacket_adaptation_info);
  spts->write_func (spts->pcr->data, TSPACKET_LENGTH, spts->write_func_data);
#endif
}

void spts_set_write_func (SpTs *spts, SpTsMuxerWriteFunc write_func, void *write_func_data)
{
  spts->write_func = write_func;
  spts->write_func_data = write_func_data;
}

gboolean spts_write_frame (SpTs *spts, GstBuffer *buffer)
{
  GstCaps *caps;
  GstStructure *s;
  guint offset;
  SpTsStream *stream;
  TsPacketAdaptationInfo adaptation_info;
  guint tspacket_payload_size;
  guint8 *buf;

  GST_DEBUG ("spts_write_frame, size: %d, \ntimestampe: %" GST_TIME_FORMAT ", duration: %d",
              GST_BUFFER_SIZE (buffer),
              GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)),
              GST_BUFFER_DURATION (buffer)); 

  memset (&adaptation_info, 0, sizeof(TsPacketAdaptationInfo));
  caps =  gst_buffer_get_caps (buffer);
  s = gst_caps_get_structure (caps, 0);
  if (gst_structure_has_name (s, "video/x-h264")) {
    stream = spts->video_stream;
    stream->frames_counter++;
    if (!GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
      /* IDR found and GOP complete, insert null packet if necessary, inster psi */
      spts->write_func (spts->nullpacket, TSPACKET_LENGTH, spts->write_func_data);
      spts_psi_write (spts);
      GST_DEBUG ("gop : %lld; frame: %lld; packets counter: %lld; max gop: %lld", spts->gop_counter, stream->frames_counter - stream->last_gop_frames_counter, stream->packets_counter - stream->last_gop_packets_counter, stream->max_gop_packets);
      if (spts->gop_counter != 0) {
        GST_DEBUG ("packets per gop: %lld, max: %lld", stream->packets_counter/(spts->gop_counter), stream->max_gop_packets);
        GST_DEBUG ("audio packets per gop %lld, max: %lld", spts->audio_stream->packets_counter/spts->gop_counter, spts->audio_stream->max_gop_packets);
      }
      spts->gop_counter++;
      stream->last_gop_frames_counter = stream->frames_counter;
      if (stream->packets_counter - stream->last_gop_packets_counter > stream->max_gop_packets)
        stream->max_gop_packets = stream->packets_counter - stream->last_gop_packets_counter;
      stream->last_gop_packets_counter = stream->packets_counter;
      if (spts->audio_stream->packets_counter - spts->audio_stream->last_gop_packets_counter > spts->audio_stream->max_gop_packets)
        spts->audio_stream->max_gop_packets = spts->audio_stream->packets_counter - spts->audio_stream->last_gop_packets_counter;
      spts->audio_stream->last_gop_packets_counter = spts->audio_stream->packets_counter;
    }
  } else if (gst_structure_has_name (s, "audio/mpeg")) {
    if (spts->packets_counter == 0) {
      spts_psi_write (spts);
    }
    stream = spts->audio_stream;
  }

  /* unit start indicator is true */
  spts_tspacket_header_set_start_unit_indicator (stream->tspacket, 0x40);
  stream->pes_header_info.pts = GSTTIME_TO_MPEGTIME (GST_BUFFER_TIMESTAMP (buffer));
  stream->pes_header_info.pes_packet_length = GST_BUFFER_SIZE (buffer);
  buf = GST_BUFFER_DATA (buffer);

  offset = TSPACKET_HEADER_LENGTH;
  if (stream->pcr_reference) {
    GST_DEBUG ("adaptation pcr");
    spts_tspacket_header_set_adaptation_field_control (stream->tspacket, ADAPTATION_FIELD_ADAPTATION_AND_PAYLOAD);
    adaptation_info.field_flag = PCR_FLAG;
    if (stream->pes_header_info.pts < 10250) { /* fixme: ? */
      adaptation_info.pcr = 0;
    } else {
      adaptation_info.pcr = (stream->pes_header_info.pts - 10250) * 300;
    }
    adaptation_info.adaptation_field_length = 0x7; /* fixme 7? */
    /* calculate stuffing bytes length first,
       playload_size - adaptation_with_pcr_only_length - pes_header_length - packet_size
    */
    if (TSPACKET_PAYLOAD_LENGTH > adaptation_info.adaptation_field_length + 1 + pes_header_length (stream->pes_header_info) + stream->pes_header_info.pes_packet_length) {
      adaptation_info.adaptation_field_length = TSPACKET_PAYLOAD_LENGTH - pes_header_length (stream->pes_header_info) - stream->pes_header_info.pes_packet_length - 1;
    }
    offset += spts_tspacket_adaptation_set (stream->tspacket, adaptation_info);
    offset += pes_header_set (stream->tspacket + offset, stream->pes_header_info);
    tspacket_payload_size = TSPACKET_LENGTH - offset;
    memcpy (stream->tspacket + offset, buf, tspacket_payload_size);
    spts->write_func (stream->tspacket, TSPACKET_LENGTH, spts->write_func_data);
    buf += tspacket_payload_size;
    stream->pes_header_info.pes_packet_length -= tspacket_payload_size;
  } else {
    /* fixme: pcr only packet */
    if (TSPACKET_PAYLOAD_LENGTH  > pes_header_length (stream->pes_header_info) + stream->pes_header_info.pes_packet_length) {
      spts_tspacket_header_set_adaptation_field_control (stream->tspacket, ADAPTATION_FIELD_ADAPTATION_AND_PAYLOAD);
      adaptation_info.field_flag = 0x00;
      adaptation_info.adaptation_field_length = TSPACKET_PAYLOAD_LENGTH - pes_header_length (stream->pes_header_info) - stream->pes_header_info.pes_packet_length - 1;
      offset += spts_tspacket_adaptation_set (stream->tspacket, adaptation_info);
    } else {
      spts_tspacket_header_set_adaptation_field_control (stream->tspacket, ADAPTATION_FIELD_PAYLOAD_ONLY);
    }
    offset += pes_header_set (stream->tspacket + offset, stream->pes_header_info);
    tspacket_payload_size = TSPACKET_LENGTH - offset;
    memcpy (stream->tspacket + offset, buf, tspacket_payload_size);
    spts->write_func (stream->tspacket, TSPACKET_LENGTH, spts->write_func_data);
    buf += tspacket_payload_size;
    stream->pes_header_info.pes_packet_length -= tspacket_payload_size;
  }

  stream->packets_counter++;
  //stream->video_packets_counter++;
  spts_tspacket_header_cc_inc (stream->tspacket);
 
  spts_tspacket_header_set_adaptation_field_control (stream->tspacket, ADAPTATION_FIELD_PAYLOAD_ONLY);
  spts_tspacket_header_set_start_unit_indicator (stream->tspacket, 0x00);

  tspacket_payload_size = TSPACKET_PAYLOAD_LENGTH;
  while (stream->pes_header_info.pes_packet_length > 0) {
    if (stream->pes_header_info.pes_packet_length < tspacket_payload_size ) {
      spts_tspacket_header_set_adaptation_field_control (stream->tspacket, ADAPTATION_FIELD_ADAPTATION_AND_PAYLOAD);
      /* set adaptation field with stuffing bytes first, then es */
      adaptation_info.field_flag = 0x00;
      adaptation_info.adaptation_field_length = tspacket_payload_size - stream->pes_header_info.pes_packet_length - 1;
      spts_tspacket_adaptation_set (stream->tspacket, adaptation_info);
      offset = adaptation_info.adaptation_field_length + 1 + TSPACKET_HEADER_LENGTH;
      memcpy (stream->tspacket + offset, buf, stream->pes_header_info.pes_packet_length);
      spts->write_func (stream->tspacket, TSPACKET_LENGTH, spts->write_func_data);
      stream->pes_header_info.pes_packet_length = 0;
    } else {
      memcpy (stream->tspacket + TSPACKET_HEADER_LENGTH, buf, tspacket_payload_size);
      spts->write_func (stream->tspacket, TSPACKET_LENGTH, spts->write_func_data);
      stream->pes_header_info.pes_packet_length -= tspacket_payload_size;
      buf += tspacket_payload_size;
    }
    stream->packets_counter++;
    //spts->video_packets_counter++;
    spts_tspacket_header_cc_inc (stream->tspacket);
    tspacket_payload_size = TSPACKET_PAYLOAD_LENGTH;
  }

  return TRUE;
}

gboolean spts_release (SpTs *spts)
{
  spts_tspacket_release (spts->pat);
  spts_tspacket_release (spts->pmt_tspacket);
  g_slice_free (SpTs, spts);

  return TRUE;
}
