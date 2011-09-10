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

#include "sptsmuxer.h"

GST_DEBUG_CATEGORY (sptsmuxer_debug);
#define GST_CAT_DEFAULT sptsmuxer_debug

/* sptsmuxer args */
enum
{
  PROP_0,
  PROP_MUXRATE,
  PUSHBUF_SIZE
};

guint8 *pushbuf = NULL;

/*
 * the capabilities of the inputs and outputs.
 * describe the real formats here.
 */
static GstStaticPadTemplate video_sink_factory =
    GST_STATIC_PAD_TEMPLATE ("video_%d",
                              GST_PAD_SINK,
                              GST_PAD_REQUEST,
                              GST_STATIC_CAPS ("video/x-h264;")
    );

static GstStaticPadTemplate audio_sink_factory =
    GST_STATIC_PAD_TEMPLATE ("audio_%d",
                              GST_PAD_SINK,
                              GST_PAD_REQUEST,
                              GST_STATIC_CAPS ("audio/mpeg, "
                                               "mpegversion = (int) { 1, 2, 4 };")
    );

static GstStaticPadTemplate src_factory =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpegts, "
                     "systemstream = (boolean) true, "
                     "packetsize = (int) { 188 }")
    );

GST_BOILERPLATE (SpTsMuxer, sptsmuxer, GstElement, GST_TYPE_ELEMENT);

static void sptsmuxer_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void sptsmuxer_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static gboolean sptsmuxer_push_packet_cb (guint8 * data, guint len, void *write_func_data);
static GstFlowReturn sptsmuxer_collected (GstCollectPads * pads, SpTsMuxer * muxer);
static GstPad *sptsmuxer_request_new_pad (GstElement *element, GstPadTemplate *templ, const gchar *name);
static gboolean sptsmuxer_srcpad_setcaps (SpTsMuxer *muxer);
static GstStateChangeReturn sptsmuxer_change_state (GstElement *element, GstStateChange transition);

/* GObject vmethod implementations */
static void
sptsmuxer_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "MPEG Transport Stream Single Program muxer",
    "Codec/Muxer",
    "Multiplexes element streams into an single program MPEG Transport Stream",
    "zhangping <<dqzhangp@163.com>>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&video_sink_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&audio_sink_factory));
}

/* initialize the sptsmuxer's class */
static void
sptsmuxer_class_init (SpTsMuxerClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = sptsmuxer_set_property;
  gobject_class->get_property = sptsmuxer_get_property;

  gstelement_class->request_new_pad = sptsmuxer_request_new_pad;
  gstelement_class->change_state = sptsmuxer_change_state;

  g_object_class_install_property (gobject_class, PROP_MUXRATE,
      g_param_spec_ulong ("muxrate", "Muxrate", "bitrate of the stream",
          0,  G_MAXULONG, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PUSHBUF_SIZE,
      g_param_spec_uint ("pushbufsize", "PushBufferSize", "Push Buffer Size, n means n*188 bytes",
          0,  G_MAXUINT, 7, G_PARAM_READWRITE));
}

/*
 * initialize the new element
 * instantiate pads and add them to element
 */
static void
sptsmuxer_init (SpTsMuxer * muxer, SpTsMuxerClass * gclass)
{
  muxer->collect = gst_collect_pads_new ();
  gst_collect_pads_set_function (muxer->collect,
                                (GstCollectPadsFunction) GST_DEBUG_FUNCPTR(sptsmuxer_collected), muxer);

  muxer->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  gst_pad_use_fixed_caps (muxer->srcpad);
  gst_element_add_pad (GST_ELEMENT (muxer), muxer->srcpad);

  muxer->first = TRUE;
  muxer->spts = spts_new ();
  spts_set_write_func (muxer->spts, sptsmuxer_push_packet_cb, muxer);
  muxer->numvideosinkpads = 0;
  muxer->numaudiosinkpads = 0;
}

static void
sptsmuxer_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  SpTsMuxer *muxer = SPTSMUXER (object);

  switch (prop_id) {
    case PROP_MUXRATE:
      muxer->spts->muxrate = g_value_get_ulong (value);
      break;
    case PUSHBUF_SIZE:
      muxer->pushbufsize = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
sptsmuxer_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  SpTsMuxer *muxer = SPTSMUXER (object);

  switch (prop_id) {
    case PROP_MUXRATE:
      g_value_set_ulong (value, muxer->spts->muxrate);
      break;
    case PUSHBUF_SIZE:
      g_value_set_uint (value, muxer->pushbufsize);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* Called when the SpTs has prepared a packet for output. Return FALSE on error and TRUE on success */
static gboolean sptsmuxer_push_packet_cb (guint8 * data, guint len, void *write_func_data)
{
  GstBuffer *buf;
  SpTsMuxer *muxer = (SpTsMuxer *)write_func_data;

  GST_DEBUG_OBJECT (muxer, "push packet, length: %d, data:%02x %02x %02x %02x", len, *data, *(data+1), *(data+2), *(data+3));
  g_assert (len == TSPACKET_LENGTH);

  if (pushbuf == NULL) {
    GST_DEBUG ("Initialize push buffer %d", muxer->pushbufsize);
    muxer->pushbufused = 0;
    pushbuf = g_malloc (muxer->pushbufsize * TSPACKET_LENGTH);
    if (pushbuf == NULL) {
      GST_ERROR ("g_malloc error");
      return FALSE;
    }
  }
  /* fixme: len shoule be 188 */

  if (muxer->pushbufused < muxer->pushbufsize) {
    memcpy (pushbuf + muxer->pushbufused * TSPACKET_LENGTH, data, len);
    muxer->pushbufused++;
  } 

  if (muxer->pushbufused == muxer->pushbufsize) {
    buf = gst_buffer_new_and_alloc (muxer->pushbufsize * TSPACKET_LENGTH);
    memcpy (GST_BUFFER_DATA (buf), pushbuf, muxer->pushbufsize * TSPACKET_LENGTH);
    if (G_UNLIKELY (buf == NULL)) {
      return FALSE;
    }
    gst_pad_push (muxer->srcpad, buf);
    muxer->pushbufused = 0;
  }

  return TRUE;
}

static GstFlowReturn sptsmuxer_collected (GstCollectPads * pads, SpTsMuxer * muxer)
{
  GSList *collect;
  GstCollectData *next_collectdata;
  GstBuffer *buffer;
  GstClockTime next_time;

  GST_DEBUG_OBJECT (muxer, "collectd");

  if (muxer->first) {
    if (!sptsmuxer_srcpad_setcaps (muxer)) {
      GST_DEBUG_OBJECT (muxer, "Failed to send new segment");
    }

    /* set pcr pid. audio stream only, pcr pid == audio pid; if video steam exist, pcr pid == video pid */
    if (muxer->spts->pmt_section.section_info.video_pid != 0) {
      psi_pmt_section_pcr_pid_set (&(muxer->spts->pmt_section), muxer->spts->pmt_section.section_info.video_pid);
      muxer->spts->video_stream->pcr_reference = TRUE;
    } else if (muxer->spts->pmt_section.section_info.audio_pid != 0) {
      psi_pmt_section_pcr_pid_set (&(muxer->spts->pmt_section), muxer->spts->pmt_section.section_info.audio_pid);
      muxer->spts->audio_stream->pcr_reference = TRUE;
    } 

    muxer->first = FALSE;
  }

  next_collectdata = NULL;
  next_time = GST_CLOCK_TIME_NONE;
  for (collect = muxer->collect->data; collect; collect = g_slist_next (collect)) {
    buffer = gst_collect_pads_peek (muxer->collect, (GstCollectData *) collect->data);
    if (buffer == NULL)
      continue;

    if (next_collectdata == NULL) {
      next_collectdata = ((GstCollectData *)collect->data);
      next_time = GST_BUFFER_TIMESTAMP (buffer);
      gst_buffer_unref (buffer);
      if (!GST_CLOCK_TIME_IS_VALID (next_time))
        break;
      continue;
    }

    if (GST_BUFFER_TIMESTAMP (buffer) < next_time) {
      next_collectdata = ((GstCollectData *)collect->data);
      next_time = GST_BUFFER_TIMESTAMP (buffer);
    }
    gst_buffer_unref (buffer);
  }

  if (next_collectdata != NULL) {
    buffer = gst_collect_pads_pop (muxer->collect, next_collectdata);
    spts_write_frame (muxer->spts, buffer);

    gst_buffer_unref (buffer);
  }

  return GST_FLOW_OK;
}

static GstPad *sptsmuxer_request_new_pad (GstElement *element, GstPadTemplate *templ, const gchar *name)
{
  SpTsMuxer *muxer = SPTSMUXER(element);
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (element);
  GstPad *pad = NULL;
  gchar *pad_name = NULL;
  SpTsMuxerCollectData *collect_data = NULL;

  if (templ == gst_element_class_get_pad_template (klass, "video_%d")) {
    pad_name = g_strdup_printf ("video_%d", muxer->numvideosinkpads++);
    spts_add_stream (muxer->spts, STREAM_TYPE_VIDEO_H264, VIDEO_PID);
    psi_pmt_section_stream_add (&(muxer->spts->pmt_section), STREAM_TYPE_VIDEO_H264, VIDEO_PID);
  } else if (templ == gst_element_class_get_pad_template (klass, "audio_%d")) {
    pad_name = g_strdup_printf ("audio_%d", muxer->numaudiosinkpads++);
    spts_add_stream (muxer->spts, STREAM_TYPE_AUDIO_AAC, AUDIO_PID);
    psi_pmt_section_stream_add (&(muxer->spts->pmt_section), STREAM_TYPE_AUDIO_AAC, AUDIO_PID);
  } else {
    GST_WARNING ("ffmux: unknown pad template!");
  }
  GST_DEBUG ("new pad, name: %s", pad_name);
  pad = gst_pad_new_from_template (templ, pad_name);
  g_free (pad_name);  

  collect_data = (SpTsMuxerCollectData *) gst_collect_pads_add_pad (muxer->collect, pad, sizeof (SpTsMuxerCollectData));
  if (collect_data == NULL) {
    GST_ERROR_OBJECT (element, "Internal error, could not add pad to collect pads");
    return NULL;
  }

  if (G_UNLIKELY (!gst_element_add_pad (element, pad))) {
    GST_ERROR_OBJECT (element, "Internal error, could not add pad to element");
    return NULL;
  }

  return pad;
}

static gboolean sptsmuxer_srcpad_setcaps (SpTsMuxer *muxer)
{
  GstEvent *newsegment;
  GstCaps *caps;

  newsegment = gst_event_new_new_segment (FALSE, 1.0, GST_FORMAT_BYTES, 0, -1, 0);
  caps = gst_caps_new_simple ("video/mpegts", "systemstream",
      G_TYPE_BOOLEAN, TRUE,
      "packetsize", G_TYPE_INT,
      MP2T_PACKET_LENGTH, NULL);
  gst_pad_set_caps (muxer->srcpad, caps);

  if (!gst_pad_push_event (muxer->srcpad, newsegment)) {
    GST_WARNING_OBJECT (muxer, "New segment event was not handled");
    return FALSE;
  }

  return TRUE;
}

static GstStateChangeReturn sptsmuxer_change_state (GstElement *element, GstStateChange transition)
{
  SpTsMuxer *muxer = SPTSMUXER (element);
  GstStateChangeReturn ret;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_collect_pads_start (muxer->collect);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_collect_pads_stop (muxer->collect);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  return ret;
}

static gboolean plugin_init (GstPlugin * sptsmuxer)
{
  /* debug category for fltering log messages */
  GST_DEBUG_CATEGORY_INIT (sptsmuxer_debug, "sptsmuxer",
      0, "Single Program MPEG Transport Stream muxer");

  return gst_element_register (sptsmuxer, "sptsmuxer", GST_RANK_NONE,
      TYPE_SPTSMUXER);
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "sptsmuxer",
    "Single Program MPEG Transport Stream muxer",
    plugin_init,
    VERSION,
    "LGPL",
    "sptsmuxer",
    "http://blog.sina.com/dqzhangp"
)
