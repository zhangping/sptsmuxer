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

#ifndef __SPTSMUXER_H__
#define __SPTSMUXER_H__

#include <gst/gst.h>
#include <gst/base/gstcollectpads.h>

#include "spts.h"

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define TYPE_SPTSMUXER \
  (sptsmuxer_get_type())
#define SPTSMUXER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),TYPE_SPTSMUXER,SpTsMuxer))
#define SPTSMUXER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),TYPE_SPTSMUXER,SpTsMuxerClass))

typedef struct _SpTsMuxer      SpTsMuxer;
typedef struct _SpTsMuxerClass SpTsMuxerClass;
typedef struct _SpTsMuxerCollectData SpTsMuxerCollectData;

struct _SpTsMuxer
{
  GstElement element;

  GstPad *srcpad;
  guint numvideosinkpads;
  guint numaudiosinkpads;

  GstCollectPads *collect;

  gboolean first;

  guint pushbufsize; /* size of push buffer */
  guint pushbufused; /* current push buffer fill size */
  SpTs *spts;
};

struct _SpTsMuxerClass 
{
  GstElementClass parent_class;
};

struct _SpTsMuxerCollectData
{
  GstCollectData collect; /* extend the CollectData */
};

GType sptsmuxer_get_type (void);

#define MP2T_PACKET_LENGTH	188

G_END_DECLS

#endif /* __SPTSMUXER_H__ */
