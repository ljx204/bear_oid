#ifndef __MEDIAL_PLAYER_INTERNAL_H__
#define __MEDIAL_PLAYER_INTERNAL_H__
#include "oid_common.h"

typedef struct demux_t  demux_t;
typedef struct input_thread_t input_thread_t;
typedef struct lib_media_t lib_media_t;
typedef struct es_format_t es_format_t;
typedef struct audio_format_t audio_format_t;
typedef struct input_thread_t input_thread_t;
typedef struct input_item_t input_item_t;
typedef struct audio_output audio_output_t;
typedef audio_format_t audio_sample_format_t;
typedef struct input_resource_t input_resource_t;
typedef struct block_t      block_t;

/**
 * This defines private core storage for an input.
 */
typedef struct input_thread_private_t input_thread_private_t;

/* input_source_t: gathers all information per input source */
typedef struct
{
    demux_t  *p_demux; /**< Demux plugin instance */

    /* Title infos for that input */

    /* Properties */
    bool b_can_pause;
    bool b_can_pace_control;
    bool b_can_rate_control;
    bool b_can_stream_record;
    bool b_rescale_ts;

    /* */
    int64_t i_pts_delay;

    bool       b_eof;   /* eof of demuxer */

} input_source_t;


/** Private input fields */
struct input_thread_private_t
{
    /* Global properties */
    double      f_fps;
    int         i_state;
    bool        b_can_pause;
    bool        b_can_rate_control;
    bool        b_can_pace_control;

    /* Current state */
    bool        b_recording;
    int         i_rate;

    /* Playtime configuration and state */
    int64_t     i_start;    /* :start-time,0 by default */
    int64_t     i_stop;     /* :stop-time, 0 if none */
    int64_t     i_run;      /* :run-time, 0 if none */
    int64_t     i_time;     /* Current time */
    bool        b_fast_seek;/* :input-fast-seek */

    /* Output */
 //   bool            b_out_pace_control; /* XXX Move it ot es_sout ? */
 //   sout_instance_t *p_sout;            /* Idem ? */
 //   es_out_t        *p_es_out;
 //   es_out_t        *p_es_out_display;

    /* Title infos FIXME multi-input (not easy) ? */
 //   int          i_title;
 //   input_title_t **title;

 //   int i_title_offset;
 //   int i_seekpoint_offset;

    /* User bookmarks FIXME won't be easy with multiples input */
 //   seekpoint_t bookmark;
 //   int         i_bookmark;
 //   seekpoint_t **pp_bookmark;

    /* Input attachment */
 //   int i_attachment;
 //   input_attachment_t **attachment;
 //   demux_t **attachment_demux;

    /* Main input properties */

    /* Input item */
    input_item_t   *p_item;

    /* Main source */
    input_source_t input;
#if 0
    /* Slave sources (subs, and others) */
    int            i_slave;
    input_source_t **slave;

    /* Resources */
    input_resource_t *p_resource;
    input_resource_t *p_resource_private;

    /* Stats counters */
    struct {
        counter_t *p_read_packets;
        counter_t *p_read_bytes;
        counter_t *p_input_bitrate;
        counter_t *p_demux_read;
        counter_t *p_demux_bitrate;
        counter_t *p_demux_corrupted;
        counter_t *p_demux_discontinuity;
        counter_t *p_decoded_audio;
        counter_t *p_decoded_video;
        counter_t *p_decoded_sub;
        counter_t *p_sout_sent_packets;
        counter_t *p_sout_sent_bytes;
        counter_t *p_sout_send_bitrate;
        counter_t *p_played_abuffers;
        counter_t *p_lost_abuffers;
        counter_t *p_displayed_pictures;
        counter_t *p_lost_pictures;
        vlc_mutex_t counters_lock;
    } counters;
#endif
    /* Buffer of pending actions */
     oid_mutex_t lock_control;
     oid_cond_t  wait_control;
  //  int i_control;
  //  input_control_t control[INPUT_CONTROL_FIFO_SIZE];

    bool b_abort;
    bool is_running;
    oid_thread_t thread;
};

/**
 * Main structure representing an input thread. This structure is mostly
 * private. The only public fields are READ-ONLY. You must use the helpers
 * to modify them
 */
struct input_thread_t
{
  //  VLC_COMMON_MEMBERS

    bool b_error;
    bool b_eof;
    bool b_preparsing;
    bool b_dead;

    /* All other data is input_thread is PRIVATE. You can't access it
     * outside of src/input */
    input_thread_private_t *p;
};



typedef void (*block_free_t) (block_t *);

struct block_t
{
    block_t    *p_next;

    uint8_t    *p_buffer; /**< Payload start */
    size_t      i_buffer; /**< Payload length */
    uint8_t    *p_start; /**< Buffer start */
    size_t      i_size; /**< Buffer total size */

    uint32_t    i_flags;
    unsigned    i_nb_samples; /* Used for audio */

    mtime_t     i_pts;
    mtime_t     i_dts;
    mtime_t     i_length;

    /* Rudimentary support for overloading block (de)allocation. */
    block_free_t pf_release;
};



/** Audio output object */
struct audio_output
{
  //  VLC_COMMON_MEMBERS

    struct aout_sys_t *sys; /**< Private data for callbacks */

    int (*start)(audio_output_t *, audio_sample_format_t *fmt);
    /**< Starts a new stream (mandatory, cannot be NULL).
      * \param fmt input stream sample format upon entry,
      *            output stream sample format upon return [IN/OUT]
      * \return VLC_SUCCESS on success, non-zero on failure
      * \note No other stream may be already started when called.
      */
    void (*stop)(audio_output_t *);
    /**< Stops the existing stream (optional, may be NULL).
      * \note A stream must have been started when called.
      */
    int (*time_get)(audio_output_t *, mtime_t *delay);
    /**< Estimates playback buffer latency (optional, may be NULL).
      * \param delay pointer to the delay until the next sample to be written
      *              to the playback buffer is rendered [OUT]
      * \return 0 on success, non-zero on failure or lack of data
      * \note A stream must have been started when called.
      */
    void (*play)(audio_output_t *, block_t *);
    /**< Queues a block of samples for playback (mandatory, cannot be NULL).
      * \note A stream must have been started when called.
      */
    void (*pause)( audio_output_t *, bool pause, mtime_t date);
    /**< Pauses or resumes playback (optional, may be NULL).
      * \param pause pause if true, resume from pause if false
      * \param date timestamp when the pause or resume was requested
      * \note A stream must have been started when called.
      */
    void (*flush)( audio_output_t *, bool wait);
    /**< Flushes or drains the playback buffers (mandatory, cannot be NULL).
      * \param wait true to wait for playback of pending buffers (drain),
      *             false to discard pending buffers (flush)
      * \note A stream must have been started when called.
      */
    int (*volume_set)(audio_output_t *, float volume);
    /**< Changes playback volume (optional, may be NULL).
      * \param volume requested volume (0. = mute, 1. = nominal)
      * \note The volume is always a positive number.
      * \warning A stream may or may not have been started when called.
      */
    int (*mute_set)(audio_output_t *, bool mute);
    /**< Changes muting (optinal, may be NULL).
      * \param mute true to mute, false to unmute
      * \warning A stream may or may not have been started when called.
      */
    int (*device_select)(audio_output_t *, const char *id);
    /**< Selects an audio output device (optional, may be NULL).
      * \param id nul-terminated device unique identifier.
      * \return 0 on success, non-zero on failure.
      * \warning A stream may or may not have been started when called.
      */
    struct {
        void (*volume_report)(audio_output_t *, float);
        void (*mute_report)(audio_output_t *, bool);
        void (*policy_report)(audio_output_t *, bool);
        void (*device_report)(audio_output_t *, const char *);
        void (*hotplug_report)(audio_output_t *, const char *, const char *);
        int (*gain_request)(audio_output_t *, float);
        void (*restart_request)(audio_output_t *, unsigned);
    } event;
};


/**
 * Note the order of libvlc_state_t enum must match exactly the order of
 * \see mediacontrol_PlayerStatus, \see input_state_e enums,
 * and VideoLAN.LibVLC.State (at bindings/cil/src/media.cs).
 *
 * Expected states by web plugins are:
 * IDLE/CLOSE=0, OPENING=1, BUFFERING=2, PLAYING=3, PAUSED=4,
 * STOPPING=5, ENDED=6, ERROR=7
 */
typedef enum lib_state_t
{
    libvlc_NothingSpecial=0,
    libvlc_Opening,
    libvlc_Buffering,
    libvlc_Playing,
    libvlc_Paused,
    libvlc_Stopped,
    libvlc_Ended,
    libvlc_Error
} lib_state_t;

struct audio_format_t
{
    vlc_fourcc_t i_format;                          /**< audio format fourcc */
    unsigned int i_rate;                              /**< audio sample-rate */

    /* Describes the channels configuration of the samples (ie. number of
     * channels which are available in the buffer, and positions). */
    uint16_t     i_physical_channels;

    /* Describes from which original channels, before downmixing, the
     * buffer is derived. */
    uint32_t     i_original_channels;

    /* Optional - for A/52, SPDIF and DTS types : */
    /* Bytes used by one compressed frame, depends on bitrate. */
    unsigned int i_bytes_per_frame;

    /* Number of sampleframes contained in one compressed frame. */
    unsigned int i_frame_length;
    /* Please note that it may be completely arbitrary - buffers are not
     * obliged to contain a integral number of so-called "frames". It's
     * just here for the division :
     * buffer_size = i_nb_samples * i_bytes_per_frame / i_frame_length
     */

    /* FIXME ? (used by the codecs) */
    unsigned     i_bitspersample;
    unsigned     i_blockalign;
    uint8_t      i_channels; /* must be <=32 */
};

struct es_format_t
{
    int             i_cat;              /**< ES category @see es_format_category_e */
 //   vlc_fourcc_t    i_codec;            /**< FOURCC value as used in vlc */
 //   vlc_fourcc_t    i_original_fourcc;  /**< original FOURCC from the container */

    int             i_id;       /**< es identifier, where means
                                    -1: let the core mark the right id
                                    >=0: valid id */
    int             i_group;    /**< group identifier, where means:
                                    -1 : standalone
                                    >= 0 then a "group" (program) is created
                                        for each value */
    int             i_priority; /**< priority, where means:
                                    -2 : mean not selectable by the users
                                    -1 : mean not selected by default even
                                         when no other stream
                                    >=0: priority */

  //  char            *psz_language;        /**< human readible language name */
  //  char            *psz_description;     /**< human readible description of language */
  //  int             i_extra_languages;    /**< length in bytes of extra language data pointer */
 //   extra_languages_t *p_extra_languages; /**< extra language data needed by some decoders */

    audio_format_t  audio;    /**< description of audio format */
   // audio_replay_gain_t audio_replay_gain; /*< audio replay gain information */
  //  video_format_t video;     /**< description of video format */
  //  subs_format_t  subs;      /**< description of subtitle format */

    unsigned int   i_bitrate; /**< bitrate of this ES */
    int      i_profile;       /**< codec specific information (like real audio flavor, mpeg audio layer, h264 profile ...) */
    int      i_level;         /**< codec specific information: indicates maximum restrictions on the stream (resolution, bitrate, codec features ...) */

    bool     b_packetized;  /**< whether the data is packetized (ie. not truncated) */
    int     i_extra;        /**< length in bytes of extra data pointer */
    void    *p_extra;       /**< extra data needed by some decoders or muxers */

};

struct input_item_t
{
    int        i_id;                 /**< Identifier of the item */

    char       *psz_name;            /**< text describing this item */
    char       *psz_uri;             /**< mrl of this item */

    int        i_options;            /**< Number of input options */
    char       **ppsz_options;       /**< Array of input options */
    uint8_t    *optflagv;            /**< Some flags of input options */
    unsigned   optflagc;

    mtime_t    i_duration;           /**< Duration in microseconds */


//    int        i_categories;         /**< Number of info categories */
//    info_category_t **pp_categories; /**< Pointer to the first info category */

    int         i_es;                /**< Number of es format descriptions */
    es_format_t **es;                /**< Es formats */

//    input_stats_t *p_stats;          /**< Statistics */
    int           i_nb_played;       /**< Number of times played */

  //  vlc_meta_t *p_meta;

 //   int         i_epg;               /**< Number of EPG entries */
 //   vlc_epg_t   **pp_epg;            /**< EPG entries */

 //   vlc_event_manager_t event_manager;

    oid_mutex_t lock;                 /**< Lock for the item */

    uint8_t     i_type;              /**< Type (file, disc, ... see input_item_type_e) */
    bool        b_fixed_name;        /**< Can the interface change the name ?*/
    bool        b_error_when_reading;/**< Error When Reading */
};

struct lib_media_t
{
//    libvlc_event_manager_t * p_event_manager;
    input_item_t      *p_input_item;
    int                i_refcount;
//    libvlc_instance_t *p_libvlc_instance;
    lib_state_t     state;
 //   VLC_FORWARD_DECLARE_OBJECT(libvlc_media_list_t*) p_subitems; /* A media descriptor can have Sub items. This is the only dependancy we really have on media_list */
    void *p_user_data;

    oid_cond_t parsed_cond;
    oid_mutex_t parsed_lock;

    bool is_parsed;
    bool has_asked_preparse;
};


struct input_resource_t
{
  //  atomic_uint    refs;

  //  vlc_object_t   *p_parent;

    /* This lock is used to serialize request and protect
     * our variables */
    oid_mutex_t    lock;

    /* */
    input_thread_t *p_input;

  //  sout_instance_t *p_sout;
  //  vout_thread_t   *p_vout_free;

    /* This lock is used to protect vout resources access (for hold)
     * It is a special case because of embed video (possible deadlock
     * between vout window request and vout holds in some(qt4) interface)
     */
  //  vlc_mutex_t    lock_hold;

    /* You need lock+lock_hold to write to the following variables and
     * only lock or lock_hold to read them */

  //  vout_thread_t   **pp_vout;
  //  int             i_vout;

    bool            b_aout_busy;
    audio_output_t *p_aout;
};

struct lib_media_player_t
{

    int                i_refcount;
    oid_mutex_t        object_lock;

    struct
    {
        input_thread_t   *p_thread;
        input_resource_t *p_resource;
        oid_mutex_t       lock;
    } input;

   // struct libvlc_instance_t * p_libvlc_instance; /* Parent instance */
    lib_media_t * p_md; /* current media descriptor */
   // lib_event_manager_t * p_event_manager;
    lib_state_t state;
};

typedef struct input_item_owner
{
    input_item_t item;
    atomic_uint refs;
} input_item_owner_t;

enum input_item_type_e
{
    ITEM_TYPE_UNKNOWN,
    ITEM_TYPE_FILE,
    ITEM_TYPE_DIRECTORY,
    ITEM_TYPE_DISC,
    ITEM_TYPE_CDDA,
    ITEM_TYPE_CARD,
    ITEM_TYPE_NET,
    ITEM_TYPE_PLAYLIST,
    ITEM_TYPE_NODE,

    /* This one is not a real type but the number of input_item types. */
    ITEM_TYPE_NUMBER
};

#define input_item_New( a,b ) input_item_NewExt( a, b, 0, NULL, 0, -1 )


#endif
