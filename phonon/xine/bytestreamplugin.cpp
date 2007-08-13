/*  This file is part of the KDE project
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.

*/

#define LOG_MODULE "input_bytestream"
#define LOG_VERBOSE
/*
#define LOG
*/

#include <kdemacros.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <xine.h>
#include "bytestream.h"
extern "C" {
// xine headers use the reserved keyword this:
#define this this_xine
#include <xine/compat.h>
#include <xine/input_plugin.h>
#include <xine/xine_internal.h>
#include <xine/xineutils.h>
#include "net_buf_ctrl.h"
#include <assert.h>
#undef this

typedef struct {
    input_class_t     input_class;

    xine_t           *xine;
    config_values_t  *config;
} kbytestream_input_class_t;

typedef struct {
    input_plugin_t    input_plugin;

    xine_stream_t    *stream;
    nbc_t *nbc;
    char *mrl;
    Phonon::Xine::ByteStream *bytestream;
} kbytestream_input_plugin_t;

static uint32_t kbytestream_plugin_get_capabilities (input_plugin_t *this_gen) {
    kbytestream_input_plugin_t *that = (kbytestream_input_plugin_t *) this_gen;

    return INPUT_CAP_PREVIEW | (that->bytestream->streamSeekable() ? INPUT_CAP_SEEKABLE : 0);
}

/* remove the !__APPLE__ junk once the osx xine stuff is merged back into xine 1.2 proper */
#if ((XINE_SUB_VERSION >= 90 && XINE_MINOR_VERSION == 1 && !defined __APPLE__) || (XINE_MINOR_VERSION > 1) && XINE_MAJOR_VERSION == 1) || XINE_MAJOR_VERSION > 1
static off_t kbytestream_plugin_read (input_plugin_t *this_gen, void *buf, off_t len) {
#else
static off_t kbytestream_plugin_read (input_plugin_t *this_gen, char *buf, off_t len) {
#endif
    kbytestream_input_plugin_t *that = (kbytestream_input_plugin_t *) this_gen;
    off_t read = that->bytestream->readFromBuffer( buf, len );
    return read;
}

static buf_element_t *kbytestream_plugin_read_block (input_plugin_t *this_gen, fifo_buffer_t *fifo, off_t todo) {
    off_t                 num_bytes, total_bytes;
    kbytestream_input_plugin_t  *that = (kbytestream_input_plugin_t *) this_gen;
    buf_element_t        *buf = fifo->buffer_pool_alloc (fifo);

    buf->content = buf->mem;
    buf->type = BUF_DEMUX_BLOCK;
    total_bytes = 0;

    while (total_bytes < todo) {
        num_bytes = that->bytestream->readFromBuffer( buf->mem + total_bytes, todo-total_bytes );
        if (num_bytes <= 0) {
            buf->free_buffer (buf);
            buf = NULL;
            break;
        }
        total_bytes += num_bytes;
    }

    if( buf != NULL )
        buf->size = total_bytes;

    return buf;
}

static off_t kbytestream_plugin_seek (input_plugin_t *this_gen, off_t offset, int origin) {
    //printf( "kbytestream_plugin_seek: sizeof(off_t) = %d\n", sizeof( off_t ) );
    kbytestream_input_plugin_t *that = (kbytestream_input_plugin_t *) this_gen;
    switch( origin )
    {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            offset += that->bytestream->currentPosition();
            break;
        case SEEK_END:
            offset += that->bytestream->streamSize();
            break;
        default:
            exit(1);
    }
    //printf( "kbytestream_plugin_seek %d\n", ( int )offset );

    return that->bytestream->seekBuffer( offset );
}

static off_t kbytestream_plugin_get_current_pos (input_plugin_t *this_gen){
    kbytestream_input_plugin_t *that = (kbytestream_input_plugin_t *) this_gen;
    return that->bytestream->currentPosition();
}

static off_t kbytestream_plugin_get_length (input_plugin_t *this_gen) {
    kbytestream_input_plugin_t *that = (kbytestream_input_plugin_t *) this_gen;
    return that->bytestream->streamSize();
}

static uint32_t kbytestream_plugin_get_blocksize (input_plugin_t * /*this_gen*/) {
    return 32768;
}

#if (XINE_SUB_VERSION > 3 && XINE_MINOR_VERSION == 1) || (XINE_MINOR_VERSION > 1 && XINE_MAJOR_VERSION == 1) || XINE_MAJOR_VERSION > 1
static const char *
#else
static char *
#endif
        kbytestream_plugin_get_mrl (input_plugin_t *this_gen) {
    kbytestream_input_plugin_t *that = (kbytestream_input_plugin_t *) this_gen;

    return that->mrl;
}

static int kbytestream_plugin_get_optional_data (input_plugin_t *this_gen,
        void *data, int data_type) {
    if (data_type == INPUT_OPTIONAL_DATA_PREVIEW) {
        kbytestream_input_plugin_t *that = (kbytestream_input_plugin_t *) this_gen;
        return that->bytestream->peekBuffer(data);
    }
    return INPUT_OPTIONAL_UNSUPPORTED;
}

static void kbytestream_plugin_dispose (input_plugin_t *this_gen ) {
    kbytestream_input_plugin_t *that = (kbytestream_input_plugin_t *) this_gen;

    if (that->nbc) {
        nbc_close(that->nbc);
        that->nbc = NULL;
    }
    that->bytestream->deleteLater();
    free (that->mrl);
    free (that);
}

static int kbytestream_plugin_open (input_plugin_t *this_gen ) {
    kbytestream_input_plugin_t *that = (kbytestream_input_plugin_t *) this_gen;

    lprintf("kbytestream_plugin_open\n");

    if (kbytestream_plugin_get_length (this_gen) == 0) {
        _x_message(that->stream, XINE_MSG_FILE_EMPTY, that->mrl, NULL);
        xine_log (that->stream->xine, XINE_LOG_MSG,
                _("input_kbytestream: File empty: >%s<\n"), that->mrl);
        return 0;
    }

    return 1;
}

static void kbytestream_pause_cb(void *that_gen)
{
    kbytestream_input_plugin_t *that = reinterpret_cast<kbytestream_input_plugin_t *>(that_gen);
    that->bytestream->setPauseForBuffering(true);
}

static void kbytestream_normal_cb(void *that_gen)
{
    kbytestream_input_plugin_t *that = reinterpret_cast<kbytestream_input_plugin_t *>(that_gen);
    that->bytestream->setPauseForBuffering(false);
}

static input_plugin_t *kbytestream_class_get_instance (input_class_t *cls_gen, xine_stream_t *stream,
        const char *data) {
    /* kbytestream_input_class_t  *cls = (kbytestream_input_class_t *) cls_gen; */
    kbytestream_input_plugin_t *that;
    const char *mrl = data;

    lprintf("kbytestream_class_get_instance\n");

    if (0 != strncasecmp (mrl, "kbytestream:/", 13) )
        return NULL;

    that = (kbytestream_input_plugin_t *) xine_xmalloc (sizeof (kbytestream_input_plugin_t));
    that->stream = stream;
    that->nbc    = nbc_init(that->stream);
    nbc_set_pause_cb(that->nbc, kbytestream_pause_cb, that);
    nbc_set_normal_cb(that->nbc, kbytestream_normal_cb, that);
    that->mrl    = strdup( mrl );
    // 13 == strlen("kbytestream:/")
    assert(strlen(mrl) >= 13 + sizeof(void *) && strlen(mrl) <= 13 + 2 * sizeof(void *));
    const unsigned char *encoded = reinterpret_cast<const unsigned char*>( mrl + 13 );
    unsigned char *addrHack = reinterpret_cast<unsigned char *>(&that->bytestream);
    for (unsigned int i = 0; i < sizeof(void *); ++i, ++encoded) {
        if( *encoded == 0x01 )
        {
            ++encoded;
            switch (*encoded) {
            case 0x01:
                addrHack[i] = '\0';
                break;
            case 0x02:
                addrHack[i] = '\1';
                break;
            case 0x03:
                addrHack[i] = '#';
                break;
            case 0x04:
                addrHack[i] = '%';
                break;
            default:
                abort();
            }
        }
        else
            addrHack[i] = *encoded;
    }

    that->input_plugin.open               = kbytestream_plugin_open;
    that->input_plugin.get_capabilities   = kbytestream_plugin_get_capabilities;
    that->input_plugin.read               = kbytestream_plugin_read;
    that->input_plugin.read_block         = kbytestream_plugin_read_block;
    that->input_plugin.seek               = kbytestream_plugin_seek;
    that->input_plugin.get_current_pos    = kbytestream_plugin_get_current_pos;
    that->input_plugin.get_length         = kbytestream_plugin_get_length;
    that->input_plugin.get_blocksize      = kbytestream_plugin_get_blocksize;
    that->input_plugin.get_mrl            = kbytestream_plugin_get_mrl;
    that->input_plugin.get_optional_data  = kbytestream_plugin_get_optional_data;
    that->input_plugin.dispose            = kbytestream_plugin_dispose;
    that->input_plugin.input_class        = cls_gen;

    return &that->input_plugin;
}


/*
 * plugin class functions
 */


#if (XINE_SUB_VERSION > 3 && XINE_MINOR_VERSION == 1) || (XINE_MINOR_VERSION > 1 && XINE_MAJOR_VERSION == 1) || XINE_MAJOR_VERSION > 1
static const char *
#else
static char *
#endif
        kbytestream_class_get_description (input_class_t * /*this_gen*/) {
    return ( char* ) _("kbytestream input plugin");
}

static const char *kbytestream_class_get_identifier (input_class_t * /*this_gen*/) {
    return "kbytestream";
}

static void kbytestream_class_dispose (input_class_t *this_gen) {
    kbytestream_input_class_t  *that = (kbytestream_input_class_t *) this_gen;
    free (that);
}

extern "C" {
    void *init_kbytestream_plugin (xine_t *xine, void * /*data*/) {
        kbytestream_input_class_t  *that;

        that = (kbytestream_input_class_t *) xine_xmalloc (sizeof (kbytestream_input_class_t));

        that->xine   = xine;
        that->config = xine->config;

        that->input_class.get_instance       = kbytestream_class_get_instance;
        that->input_class.get_identifier     = kbytestream_class_get_identifier;
        that->input_class.get_description    = kbytestream_class_get_description;
        that->input_class.get_dir            = NULL;
        that->input_class.get_autoplay_list  = NULL;
        that->input_class.dispose            = kbytestream_class_dispose;
        that->input_class.eject_media        = NULL;

        return that;
    }
}
} // extern "C"

/* vim: sw=4 et
 */
