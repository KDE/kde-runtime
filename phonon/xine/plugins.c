#include <xine.h>
#include <xine/compat.h>
#include <xine/input_plugin.h>
#include <xine/xine_internal.h>
#include <xine/xineutils.h>

extern void *init_kbytestream_plugin (xine_t *xine, void *data);
extern void *init_kvolumefader_plugin (xine_t *xine, void *data);
/*extern void *init_kmixer_plugin(xine_t *xine, void *data);*/

static const post_info_t kvolumefader_special_info = { XINE_POST_TYPE_AUDIO_FILTER };
/*static const post_info_t kmixer_special_info = { XINE_POST_TYPE_AUDIO_FILTER };*/

/*
 * exported plugin catalog entry
 */
const plugin_info_t phonon_xine_plugin_info[] = {
    /* type, API, "name", version, special_info, init_function */
    { PLUGIN_INPUT, 17, "KBYTESTREAM" , XINE_VERSION_CODE, NULL                      , init_kbytestream_plugin   },
    { PLUGIN_POST , 9 , "KVolumeFader", XINE_VERSION_CODE, &kvolumefader_special_info, &init_kvolumefader_plugin },
    /*{ PLUGIN_POST , 9 , "KMixer"      , XINE_VERSION_CODE, &kmixer_special_info      , &init_kmixer_plugin       },*/
    { PLUGIN_NONE , 0 , ""            , 0                , NULL                      , NULL                      }
};
