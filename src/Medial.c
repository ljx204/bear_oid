#include"Media_player_internal.h"

#if 0
void input_item_SetURI( input_item_t *p_i, const char *psz_uri )
{
    assert( psz_uri );
#ifndef NDEBUG
    if( !strstr( psz_uri, "://" )
     || strchr( psz_uri, ' ' ) || strchr( psz_uri, '"' ) )
        fprintf( stderr, "Warning: %s(\"%s\"): file path instead of URL.\n",
                 __func__, psz_uri );
#endif
    oid_mutex_lock( &p_i->lock );
    free( p_i->psz_uri );
    p_i->psz_uri = strdup( psz_uri );

    p_i->i_type = GuessType( p_i );

    if( p_i->psz_name )
        ;
    else
    if( p_i->i_type == ITEM_TYPE_FILE || p_i->i_type == ITEM_TYPE_DIRECTORY )
    {
        const char *psz_filename = strrchr( p_i->psz_uri, '/' );

        if( psz_filename && *psz_filename == '/' )
            psz_filename++;
        if( psz_filename && *psz_filename )
            p_i->psz_name = strdup( psz_filename );

        /* Make the name more readable */
        if( p_i->psz_name )
        {
            decode_URI( p_i->psz_name );
            EnsureUTF8( p_i->psz_name );
        }
    }
    else
    {   /* Strip login and password from title */
        int r;
        vlc_url_t url;

        vlc_UrlParse( &url, psz_uri, 0 );
        if( url.psz_protocol )
        {
            if( url.i_port > 0 )
                r=asprintf( &p_i->psz_name, "%s://%s:%d%s", url.psz_protocol,
                          url.psz_host, url.i_port,
                          url.psz_path ? url.psz_path : "" );
            else
                r=asprintf( &p_i->psz_name, "%s://%s%s", url.psz_protocol,
                          url.psz_host ? url.psz_host : "",
                          url.psz_path ? url.psz_path : "" );
        }
        else
        {
            if( url.i_port > 0 )
                r=asprintf( &p_i->psz_name, "%s:%d%s", url.psz_host, url.i_port,
                          url.psz_path ? url.psz_path : "" );
            else
                r=asprintf( &p_i->psz_name, "%s%s", url.psz_host,
                          url.psz_path ? url.psz_path : "" );
        }
        vlc_UrlClean( &url );
        if( -1==r )
            p_i->psz_name=NULL; /* recover from undefined value */
    }

    oid_mutex_unlock( &p_i->lock );
}

#endif

void input_item_SetName( input_item_t *p_item, const char *psz_name )
{
    oid_mutex_lock( &p_item->lock );

    free( p_item->psz_name );
    p_item->psz_name = strdup( psz_name );

    oid_mutex_unlock( &p_item->lock );
}

input_item_t *
input_item_NewWithType( const char *psz_uri, const char *psz_name,
                        int i_options, const char *const *ppsz_options,
                        unsigned flags, mtime_t duration, int type )
{
//    static atomic_uint last_input_id = ATOMIC_VAR_INIT(0);

    input_item_owner_t *owner = calloc( 1, sizeof( *owner ) );
    if( unlikely(owner == NULL) )
        return NULL;

  //  atomic_init( &owner->refs, 1 );

    input_item_t *p_input = &owner->item;
  //  vlc_event_manager_t * p_em = &p_input->event_manager;

 //   p_input->i_id = atomic_fetch_add(&last_input_id, 1);
    oid_mutex_init( &p_input->lock );

    p_input->psz_name = NULL;
    if( psz_name )
        input_item_SetName( p_input, psz_name );

    p_input->psz_uri = NULL;
    if( psz_uri )
        input_item_SetURI( p_input, psz_uri );
    else
        p_input->i_type = ITEM_TYPE_UNKNOWN;

//    TAB_INIT( p_input->i_options, p_input->ppsz_options );
//    p_input->optflagc = 0;
//    p_input->optflagv = NULL;
//    for( int i = 0; i < i_options; i++ )
//        input_item_AddOption( p_input, ppsz_options[i], flags );

    p_input->i_duration = duration;
//    TAB_INIT( p_input->i_categories, p_input->pp_categories );
//    TAB_INIT( p_input->i_es, p_input->es );
//    p_input->p_stats = NULL;
    p_input->i_nb_played = 0;
//    p_input->p_meta = NULL;
//    TAB_INIT( p_input->i_epg, p_input->pp_epg );

//    vlc_event_manager_init( p_em, p_input );
//    vlc_event_manager_register_event_type( p_em, vlc_InputItemMetaChanged );
//    vlc_event_manager_register_event_type( p_em, vlc_InputItemSubItemAdded );
//    vlc_event_manager_register_event_type( p_em, vlc_InputItemSubItemTreeAdded );
//    vlc_event_manager_register_event_type( p_em, vlc_InputItemDurationChanged );
//    vlc_event_manager_register_event_type( p_em, vlc_InputItemPreparsedChanged );
//    vlc_event_manager_register_event_type( p_em, vlc_InputItemNameChanged );
//    vlc_event_manager_register_event_type( p_em, vlc_InputItemInfoChanged );
//    vlc_event_manager_register_event_type( p_em, vlc_InputItemErrorWhenReadingChanged );

    if( type != ITEM_TYPE_UNKNOWN )
        p_input->i_type = type;
    p_input->b_fixed_name = false;
    p_input->b_error_when_reading = false;
    return p_input;
}



input_item_t *input_item_NewExt( const char *psz_uri,
                                 const char *psz_name,
                                 int i_options,
                                 const char *const *ppsz_options,
                                 unsigned i_option_flags,
                                 mtime_t i_duration )
{
    return input_item_NewWithType( psz_uri, psz_name,
                                  i_options, ppsz_options, i_option_flags,
                                  i_duration, ITEM_TYPE_UNKNOWN );
}

/**************************************************************************
 * Create a new media descriptor object from an input_item
 * (libvlc internal)
 * That's the generic constructor
 **************************************************************************/
lib_media_t * lib_media_new_from_input_item(input_item_t *p_input_item )
{
    lib_media_t * p_md;

    if (!p_input_item)
    {
        printf( "No input item given" );
        return NULL;
    }

    p_md = calloc( 1, sizeof(lib_media_t) );
    if( !p_md )
    {
        printf( "Not enough memory" );
        return NULL;
    }

   // p_md->p_libvlc_instance = p_instance;
    p_md->p_input_item      = p_input_item;
    p_md->i_refcount        = 1;

    oid_cond_init(&p_md->parsed_cond);
    oid_mutex_init(&p_md->parsed_lock);

    p_md->state = libvlc_NothingSpecial;

    /* A media descriptor can be a playlist. When you open a playlist
     * It can give a bunch of item to read. */
   // p_md->p_subitems        = NULL;

   // p_md->p_event_manager = libvlc_event_manager_new( p_md, p_instance );
   // if( unlikely(p_md->p_event_manager == NULL) )
   // {
   //     free(p_md);
    //    return NULL;
   // }

   // libvlc_event_manager_t *em = p_md->p_event_manager;
   // libvlc_event_manager_register_event_type(em, libvlc_MediaMetaChanged);
   // libvlc_event_manager_register_event_type(em, libvlc_MediaSubItemAdded);
   // libvlc_event_manager_register_event_type(em, libvlc_MediaFreed);
   // libvlc_event_manager_register_event_type(em, libvlc_MediaDurationChanged);
   // libvlc_event_manager_register_event_type(em, libvlc_MediaStateChanged);
   // libvlc_event_manager_register_event_type(em, libvlc_MediaParsedChanged);
   // libvlc_event_manager_register_event_type(em, libvlc_MediaSubItemTreeAdded);

   // vlc_gc_incref( p_md->p_input_item );

   // install_input_item_observer( p_md );

    return p_md;
}

/**************************************************************************
 * Create a new media descriptor object
 **************************************************************************/
lib_media_t *lib_media_new_location( const char * psz_mrl )
{
    input_item_t * p_input_item;
    lib_media_t * p_md;

    p_input_item = input_item_New( psz_mrl, NULL );

    if (!p_input_item)
    {
        printf( "Not enough memory" );
        return NULL;
    }

    p_md = lib_media_new_from_input_item( p_input_item );

    /* The p_input_item is retained in libvlc_media_new_from_input_item */
   // vlc_gc_decref( p_input_item );

    return p_md;
}


#if 0
lib_media_t *lib_media_new_path(const char *path )
{
    char *mrl = lib_path2uri( path, NULL );
    if( unlikely(mrl == NULL) )
    {
       // libvlc_printerr( "%s", vlc_strerror_c(errno) );
        return NULL;
    }

    lib_media_t *m = lib_media_new_location(mrl );
    free( mrl );
    return m;
}
#endif

lib_media_t *lib_media_new_fd(int fd )
{
    char mrl[16];
    snprintf( mrl, sizeof(mrl), "fd://%d", fd );

    return lib_media_new_location( mrl );
}

