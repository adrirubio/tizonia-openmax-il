/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Tizonia is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   tizdemuxerport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - demuxerport class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizdemuxerport.h"
#include "tizdemuxerport_decls.h"

#include "tizpcmport.h"
#include "tizvideoport.h"

#include "tizosal.h"
#include "tizutils.h"

#include <assert.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.demuxerport"
#endif

/*
 * tizdemuxerport class
 */

static void *
demuxerport_ctor (void *ap_obj, va_list * app)
{
  tiz_demuxerport_t *p_obj = NULL;
  tiz_port_options_t *p_opts = NULL;
  va_list app_copy;

  /* Make a copy of the incoming va_list before it gets parsed by the parent
     class:
     The expected arguments are:
     ap_hdl,
     port_opts

     */
  va_copy (app_copy, *app);

  /* Now give the original to the base class */
  if (NULL !=  (p_obj = super_ctor (tizdemuxerport, ap_obj, app)))
    {
      /* Register the demuxer-specific indexes  */
      tiz_check_omx_err_ret_null
        (tiz_port_register_index (p_obj, OMX_IndexParamNumAvailableStreams));
      tiz_check_omx_err_ret_null
        (tiz_port_register_index (p_obj, OMX_IndexParamActiveStream));

      /* Do not forget to pop the component handle (its needed in a base class) */
      (void) va_arg (app_copy, OMX_HANDLETYPE);

      /* Grab the port options structure (mandatory argument) */
      p_opts      = va_arg (app_copy, tiz_port_options_t *);
      assert (NULL != p_opts);

      TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_obj), "min_buf_size [%d]",
                p_opts->min_buf_size);

      switch (p_opts->domain)
        {
        case OMX_PortDomainAudio:
          {
            /* Let's instantiate a PCM port */
            OMX_AUDIO_CODINGTYPE *p_encodings = NULL;
            OMX_AUDIO_PARAM_PCMMODETYPE *p_pcmmode = NULL;
            OMX_AUDIO_CONFIG_VOLUMETYPE *p_volume = NULL;
            OMX_AUDIO_CONFIG_MUTETYPE *p_mute = NULL;
            OMX_AUDIO_CODINGTYPE encodings[] = {
              OMX_AUDIO_CodingUnused,
              OMX_AUDIO_CodingMax
            };

            /* Register the PCM port indexes, so this port receives the get/set
               requests */
            tiz_check_omx_err_ret_null
              (tiz_port_register_index (p_obj, OMX_IndexParamAudioPcm));
            tiz_check_omx_err_ret_null
              (tiz_port_register_index (p_obj, OMX_IndexConfigAudioVolume));
            tiz_check_omx_err_ret_null
              (tiz_port_register_index (p_obj, OMX_IndexConfigAudioMute));

            /* Get the array of OMX_AUDIO_CODINGTYPE values  (mandatory argument) */
            p_encodings = va_arg (app_copy, OMX_AUDIO_CODINGTYPE *);
            assert (NULL != p_encodings);

            /* Get the OMX_AUDIO_PARAM_PCMMODETYPE structure (mandatory argument) */
            p_pcmmode = va_arg (app_copy, OMX_AUDIO_PARAM_PCMMODETYPE *);
            assert (NULL != p_pcmmode);

            /* Get the OMX_AUDIO_CONFIG_VOLUMETYPE structure (mandatory argument) */
            p_volume = va_arg (app_copy, OMX_AUDIO_CONFIG_VOLUMETYPE *);
            assert (NULL != p_volume);

            TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_obj), "p_volume->sVolume.nValue [%d]",
                      p_volume->sVolume.nValue);

            /* Get the OMX_AUDIO_CONFIG_MUTETYPE structure (mandatory argument) */
            p_mute = va_arg (app_copy, OMX_AUDIO_CONFIG_MUTETYPE *);
            assert (NULL != p_mute);

            tiz_check_omx_err_ret_null (tiz_pcmport_init ());
            p_obj->p_port_
              = factory_new (tizpcmport, tiz_api_get_hdl (ap_obj),
                             p_opts, &encodings, p_pcmmode, p_volume, p_mute);
            if (NULL == p_obj->p_port_)
              {
                return NULL;
              }
          }
          break;

        case OMX_PortDomainVideo:
          {
            OMX_VIDEO_PORTDEFINITIONTYPE *p_portdef = NULL;
            OMX_VIDEO_CODINGTYPE *p_encodings = NULL;
            OMX_COLOR_FORMATTYPE *p_formats = NULL;

            /* Register the raw video port indexes, so this port receives the
               get/set requests */
            tiz_check_omx_err_ret_null
              (tiz_port_register_index (p_obj, OMX_IndexParamVideoPortFormat));

            /* Get the OMX_VIDEO_PORTDEFINITIONTYPE structure (mandatory argument) */
            p_portdef = va_arg (app_copy, OMX_VIDEO_PORTDEFINITIONTYPE *);
            assert (NULL != p_portdef);

            /* Get the array of OMX_VIDEO_CODINGTYPE values (mandatory argument) */
            p_encodings = va_arg (app_copy, OMX_VIDEO_CODINGTYPE *);
            assert (NULL != p_encodings);

            /* Get the array of OMX_COLOR_FORMATTYPE values (mandatory argument) */
            p_formats = va_arg (app_copy, OMX_COLOR_FORMATTYPE *);
            assert (NULL != p_formats);

            tiz_check_omx_err_ret_null (tiz_videoport_init ());
            if (NULL == (p_obj->p_port_
                         = factory_new (tizvideoport, tiz_api_get_hdl (ap_obj),
                                        p_opts, p_portdef, p_encodings, p_formats)))
              {
                return NULL;
              }
          }
          break;

        default:
          assert (0);
        };
    }
  va_end (app_copy);

  return p_obj;
}

static void *
demuxerport_dtor (void *ap_obj)
{
  tiz_demuxerport_t *p_obj = ap_obj;
  assert (NULL != p_obj);
  factory_delete (p_obj->p_port_);
  return super_dtor (tizdemuxerport, ap_obj);
}

/*
 * from tiz_api
 */
static OMX_ERRORTYPE
demuxerport_GetParameter (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_demuxerport_t *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_obj),
            "GetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamNumAvailableStreams:
    case OMX_IndexParamActiveStream:
      {
        /* Only the processor knows about available or active streams. So lets
           get the processor to fill this info for us. */
        void *p_prc = tiz_get_prc (ap_hdl);
        assert (NULL != p_prc);
        if (OMX_ErrorNone != (rc = tiz_api_GetParameter (p_prc, ap_hdl,
                                                         a_index, ap_struct)))
          {
            TIZ_LOGN (TIZ_ERROR, ap_hdl, "[%s] : Error retrieving [%s] "
                      "from the processor", tiz_err_to_str (rc),
                      tiz_idx_to_str (a_index));
            return rc;
          }
      }
      break;

    case OMX_IndexParamAudioPortFormat:
    case OMX_IndexParamVideoPortFormat:
    case OMX_IndexParamAudioPcm:
      {
        /* Delegate to the domain-specific port */
        if (OMX_ErrorUnsupportedIndex
            != (rc = tiz_api_GetParameter (p_obj->p_port_,
                                          ap_hdl, a_index, ap_struct)))
          {
            return rc;
          }
      }
      /* NOTE: Fall through if GetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
      /*@fallthrough@*/
    default:
      {
        /* Delegate to the base port */
        return super_GetParameter (tizdemuxerport,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
demuxerport_SetParameter (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_demuxerport_t *p_obj = (tiz_demuxerport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_obj),
            "SetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamNumAvailableStreams:
      {
        /* This is a read-only index. Simply ignore it. */
        TIZ_LOGN (TIZ_NOTICE, ap_hdl, "Ignoring read-only index [%s] ",
                  tiz_idx_to_str (a_index));
      }
      break;

    case OMX_IndexParamActiveStream:
      {
        /* Only the processor knows about active streams. So lets
           get the processor update this info for us. */
        void *p_prc = tiz_get_prc (ap_hdl);
        assert (NULL != p_prc);
        if (OMX_ErrorUnsupportedIndex
            != (rc = tiz_api_SetParameter (p_prc, ap_hdl,
                                           a_index, ap_struct)))
          {
            TIZ_LOGN (TIZ_ERROR, ap_hdl, "[%s] : Error setting [%s] "
                      "on the processor", tiz_err_to_str (rc),
                      tiz_idx_to_str (a_index));
            return rc;
          }
      }
      break;

    case OMX_IndexParamAudioPortFormat:
    case OMX_IndexParamVideoPortFormat:
      {
        /* Delegate to the domain-specific port */
        assert (NULL != p_obj->p_port_);
        if (OMX_ErrorUnsupportedIndex
            != (rc = tiz_api_SetParameter (p_obj->p_port_,
                                           ap_hdl, a_index, ap_struct)))
          {
            return rc;
          }
      }

      /* NOTE: Fall through if SetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
      /*@fallthrough@*/
    default:
      {
        /* Delegate to the base port */
        return super_SetParameter (tizdemuxerport,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
demuxerport_GetConfig (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                       OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_demuxerport_t *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_obj),
            "GetConfig [%s]...", tiz_idx_to_str (a_index));

  assert (NULL != ap_obj);

  switch (a_index)
    {
    case OMX_IndexConfigAudioVolume:
    case OMX_IndexConfigAudioMute:
      {
        /* Delegate to the domain-specific port */
        if (OMX_ErrorUnsupportedIndex
            != (rc = tiz_api_GetConfig (p_obj->p_port_,
                                           ap_hdl, a_index, ap_struct)))
          {
            return rc;
          }
      }

      /* NOTE: Fall through if GetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
      /*@fallthrough@*/
    default:
      {
        /* Try the parent's indexes */
        rc = super_GetConfig (tizdemuxerport,
                              ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return rc;

}

static OMX_ERRORTYPE
demuxerport_SetConfig (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_demuxerport_t *p_obj = (tiz_demuxerport_t *) ap_obj;
        OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_obj),
            "SetConfig [%s]...", tiz_idx_to_str (a_index));

  assert (NULL != ap_obj);

  switch (a_index)
    {
    case OMX_IndexConfigAudioVolume:
    case OMX_IndexConfigAudioMute:
      {
        /* TODO: Delegate this to the processor */
        if (OMX_ErrorUnsupportedIndex
            != (rc = tiz_api_SetConfig (p_obj->p_port_,
                                        ap_hdl, a_index, ap_struct)))
          {
            return rc;
          }
      }

      /* NOTE: Fall through if GetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
      /*@fallthrough@*/
    default:
      {
        /* Try the parent's indexes */
        rc = super_SetConfig (tizdemuxerport,
                              ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return rc;

}

static bool
demuxerport_check_tunnel_compat
(const void *ap_obj,
 OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
 OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  tiz_port_t *p_obj = (tiz_port_t *) ap_obj;
  assert (NULL != ap_this_def);
  assert (NULL != ap_other_def);

  if (ap_other_def->eDomain != ap_this_def->eDomain)
    {
      TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_obj),
                "port [%d] check_tunnel_compat : Different domain found [%d]",
                p_obj->pid_, ap_other_def->eDomain);
      return false;
    }

  return true;
}

/*
 * tizdemuxerport_class
 */

static void *
demuxerport_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (tizdemuxerport_class, ap_obj, app);
}

/*
 * initialization
 */

const void *tizdemuxerport, *tizdemuxerport_class;

OMX_ERRORTYPE
tiz_demuxerport_init (void)
{
  if (!tizdemuxerport_class)
    {
      tiz_check_omx_err_ret_oom (tiz_port_init ());
      tiz_check_null_ret_oom
        (tizdemuxerport_class = factory_new (tizport_class,
                                            "tizdemuxerport_class",
                                            tizport_class,
                                            sizeof (tiz_demuxerport_class_t),
                                            ctor, demuxerport_class_ctor, 0));
    }

  if (!tizdemuxerport)
    {
      tiz_check_omx_err_ret_oom (tiz_port_init ());
      tiz_check_null_ret_oom
        (tizdemuxerport =
         factory_new
         (tizdemuxerport_class,
          "tizdemuxerport",
          tizport,
          sizeof (tiz_demuxerport_t),
          ctor, demuxerport_ctor,
          dtor, demuxerport_dtor,
          tiz_api_GetParameter, demuxerport_GetParameter,
          tiz_api_SetParameter, demuxerport_SetParameter,
          tiz_api_GetConfig, demuxerport_GetConfig,
          tiz_api_SetConfig, demuxerport_SetConfig,
          tiz_port_check_tunnel_compat, demuxerport_check_tunnel_compat, 0));
    }
  return OMX_ErrorNone;
}
