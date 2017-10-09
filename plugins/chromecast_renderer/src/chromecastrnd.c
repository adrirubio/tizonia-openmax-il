/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * along with Tizonia.  If not, see <chromecast://www.gnu.org/licenses/>.
 */

/**
 * @file   chromecastrnd.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Chromecast renderer component
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_TizoniaExt.h>
#include <OMX_Types.h>

#include <tizplatform.h>

#include <tizport.h>
#include <tizscheduler.h>

#include "chromecastrndprc.h"
#include "chromecastrndport.h"
#include "chromecastrnd.h"
#include "cc_gmusicprc.h"
#include "cc_gmusiccfgport.h"
#include "cc_scloudprc.h"
#include "cc_scloudcfgport.h"
#include "cc_dirbleprc.h"
#include "cc_dirblecfgport.h"
#include "cc_youtubeprc.h"
#include "cc_youtubecfgport.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer"
#endif

/**
 *@defgroup libtizchromecastrnd 'libtizchromecastrnd' : OpenMAX IL Chromecast audio client
 *
 * - Component name : "OMX.Aratelia.audio_renderer.chromecast"
 * - Implements role: "audio_renderer.chromecast"
 * - Implements role: "audio_renderer.chromecast.gmusic"
 * - Implements role: "audio_renderer.chromecast.scloud"
 * - Implements role: "audio_renderer.chromecast.dirble"
 * - Implements role: "audio_renderer.chromecast.youtube"
 *
 *@ingroup plugins
 */

static OMX_VERSIONTYPE chromecast_renderer_version = {{1, 0, 0, 0}};

static OMX_PTR
instantiate_input_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_CODINGTYPE encodings[]
    = {OMX_AUDIO_CodingUnused, OMX_AUDIO_CodingMax};
  tiz_port_options_t port_opts = {
    OMX_PortDomainAudio,
    OMX_DirInput,
    ARATELIA_CHROMECAST_RENDERER_PORT_MIN_BUF_COUNT,
    ARATELIA_CHROMECAST_RENDERER_PORT_MIN_BUF_SIZE,
    ARATELIA_CHROMECAST_RENDERER_PORT_NONCONTIGUOUS,
    ARATELIA_CHROMECAST_RENDERER_PORT_ALIGNMENT,
    ARATELIA_CHROMECAST_RENDERER_PORT_SUPPLIERPREF,
    {ARATELIA_CHROMECAST_RENDERER_PORT_INDEX, NULL, NULL, NULL},
    -1 /* use -1 for now */
  };

  return factory_new (tiz_get_type (ap_hdl, "chromecastrndport"), &port_opts,
                      &encodings);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizuricfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME,
                      chromecast_renderer_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "chromecastrndprc"));
}

static OMX_PTR
instantiate_gmusic_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_gmusiccfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME,
                      chromecast_renderer_version);
}

static OMX_PTR
instantiate_gmusic_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_gmusicprc"));
}

static OMX_PTR
instantiate_scloud_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_scloudcfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME,
                      chromecast_renderer_version);
}

static OMX_PTR
instantiate_scloud_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_scloudprc"));
}

static OMX_PTR
instantiate_dirble_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_dirblecfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME,
                      chromecast_renderer_version);
}

static OMX_PTR
instantiate_dirble_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_dirbleprc"));
}

static OMX_PTR
instantiate_youtube_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_youtubecfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME,
                      chromecast_renderer_version);
}

static OMX_PTR
instantiate_youtube_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_youtubeprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t chromecast_client_role;
  tiz_role_factory_t gmusic_client_role;
  tiz_role_factory_t scloud_client_role;
  tiz_role_factory_t dirble_client_role;
  tiz_role_factory_t youtube_client_role;
  const tiz_role_factory_t * rf_list[]
    = {&chromecast_client_role, &gmusic_client_role, &scloud_client_role,
       &dirble_client_role, &youtube_client_role};
  tiz_type_factory_t chromecastrndprc_type;
  tiz_type_factory_t chromecastrndport_type;
  tiz_type_factory_t cc_gmusicprc_type;
  tiz_type_factory_t cc_gmusiccfgport_type;
  tiz_type_factory_t cc_scloudprc_type;
  tiz_type_factory_t cc_scloudcfgport_type;
  tiz_type_factory_t cc_dirbleprc_type;
  tiz_type_factory_t cc_dirblecfgport_type;
  tiz_type_factory_t cc_youtubeprc_type;
  tiz_type_factory_t cc_youtubecfgport_type;
  const tiz_type_factory_t * tf_list[]
    = {&chromecastrndprc_type, &chromecastrndport_type, &cc_gmusicprc_type,
       &cc_gmusiccfgport_type, &cc_scloudprc_type,      &cc_scloudcfgport_type,
       &cc_dirbleprc_type,     &cc_dirblecfgport_type,  &cc_youtubeprc_type,
       &cc_youtubecfgport_type};

  strcpy ((OMX_STRING) chromecast_client_role.role,
          ARATELIA_CHROMECAST_RENDERER_DEFAULT_ROLE);
  chromecast_client_role.pf_cport = instantiate_config_port;
  chromecast_client_role.pf_port[0] = instantiate_input_port;
  chromecast_client_role.nports = 1;
  chromecast_client_role.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING) gmusic_client_role.role,
          ARATELIA_GMUSIC_SOURCE_DEFAULT_ROLE);
  gmusic_client_role.pf_cport = instantiate_gmusic_config_port;
  gmusic_client_role.pf_port[0] = instantiate_input_port;
  gmusic_client_role.nports = 1;
  gmusic_client_role.pf_proc = instantiate_gmusic_processor;

  strcpy ((OMX_STRING) scloud_client_role.role,
          ARATELIA_SCLOUD_SOURCE_DEFAULT_ROLE);
  scloud_client_role.pf_cport = instantiate_scloud_config_port;
  scloud_client_role.pf_port[0] = instantiate_input_port;
  scloud_client_role.nports = 1;
  scloud_client_role.pf_proc = instantiate_scloud_processor;

  strcpy ((OMX_STRING) dirble_client_role.role,
          ARATELIA_DIRBLE_SOURCE_DEFAULT_ROLE);
  dirble_client_role.pf_cport = instantiate_dirble_config_port;
  dirble_client_role.pf_port[0] = instantiate_input_port;
  dirble_client_role.nports = 1;
  dirble_client_role.pf_proc = instantiate_dirble_processor;

  strcpy ((OMX_STRING) youtube_client_role.role,
          ARATELIA_YOUTUBE_SOURCE_DEFAULT_ROLE);
  youtube_client_role.pf_cport = instantiate_youtube_config_port;
  youtube_client_role.pf_port[0] = instantiate_input_port;
  youtube_client_role.nports = 1;
  youtube_client_role.pf_proc = instantiate_youtube_processor;

  strcpy ((OMX_STRING) chromecastrndprc_type.class_name,
          "chromecastrndprc_class");
  chromecastrndprc_type.pf_class_init = chromecastrnd_prc_class_init;
  strcpy ((OMX_STRING) chromecastrndprc_type.object_name, "chromecastrndprc");
  chromecastrndprc_type.pf_object_init = chromecastrnd_prc_init;

  strcpy ((OMX_STRING) chromecastrndport_type.class_name,
          "chromecastrndport_class");
  chromecastrndport_type.pf_class_init = chromecastrnd_port_class_init;
  strcpy ((OMX_STRING) chromecastrndport_type.object_name, "chromecastrndport");
  chromecastrndport_type.pf_object_init = chromecastrnd_port_init;

  strcpy ((OMX_STRING) cc_gmusicprc_type.class_name, "cc_gmusicprc_class");
  cc_gmusicprc_type.pf_class_init = cc_gmusic_prc_class_init;
  strcpy ((OMX_STRING) cc_gmusicprc_type.object_name, "cc_gmusicprc");
  cc_gmusicprc_type.pf_object_init = cc_gmusic_prc_init;

  strcpy ((OMX_STRING) cc_gmusiccfgport_type.class_name,
          "cc_gmusiccfgport_class");
  cc_gmusiccfgport_type.pf_class_init = cc_gmusic_cfgport_class_init;
  strcpy ((OMX_STRING) cc_gmusiccfgport_type.object_name, "cc_gmusiccfgport");
  cc_gmusiccfgport_type.pf_object_init = cc_gmusic_cfgport_init;

  strcpy ((OMX_STRING) cc_scloudprc_type.class_name, "cc_scloudprc_class");
  cc_scloudprc_type.pf_class_init = cc_scloud_prc_class_init;
  strcpy ((OMX_STRING) cc_scloudprc_type.object_name, "cc_scloudprc");
  cc_scloudprc_type.pf_object_init = cc_scloud_prc_init;

  strcpy ((OMX_STRING) cc_scloudcfgport_type.class_name,
          "cc_scloudcfgport_class");
  cc_scloudcfgport_type.pf_class_init = cc_scloud_cfgport_class_init;
  strcpy ((OMX_STRING) cc_scloudcfgport_type.object_name, "cc_scloudcfgport");
  cc_scloudcfgport_type.pf_object_init = cc_scloud_cfgport_init;

  strcpy ((OMX_STRING) cc_dirbleprc_type.class_name, "cc_dirbleprc_class");
  cc_dirbleprc_type.pf_class_init = cc_dirble_prc_class_init;
  strcpy ((OMX_STRING) cc_dirbleprc_type.object_name, "cc_dirbleprc");
  cc_dirbleprc_type.pf_object_init = cc_dirble_prc_init;

  strcpy ((OMX_STRING) cc_dirblecfgport_type.class_name,
          "cc_dirblecfgport_class");
  cc_dirblecfgport_type.pf_class_init = cc_dirble_cfgport_class_init;
  strcpy ((OMX_STRING) cc_dirblecfgport_type.object_name, "cc_dirblecfgport");
  cc_dirblecfgport_type.pf_object_init = cc_dirble_cfgport_init;

  strcpy ((OMX_STRING) cc_youtubeprc_type.class_name, "cc_youtubeprc_class");
  cc_youtubeprc_type.pf_class_init = cc_youtube_prc_class_init;
  strcpy ((OMX_STRING) cc_youtubeprc_type.object_name, "cc_youtubeprc");
  cc_youtubeprc_type.pf_object_init = cc_youtube_prc_init;

  strcpy ((OMX_STRING) cc_youtubecfgport_type.class_name,
          "cc_youtubecfgport_class");
  cc_youtubecfgport_type.pf_class_init = cc_youtube_cfgport_class_init;
  strcpy ((OMX_STRING) cc_youtubecfgport_type.object_name, "cc_youtubecfgport");
  cc_youtubecfgport_type.pf_object_init = cc_youtube_cfgport_init;

  /* Initialize the component infrastructure */
  tiz_check_omx (
    tiz_comp_init (ap_hdl, ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME));

  /* Register the various classes */
  tiz_check_omx (tiz_comp_register_types (ap_hdl, tf_list, 10));

  /* Register the component roles */
  tiz_check_omx (tiz_comp_register_roles (ap_hdl, rf_list, 5));

  return OMX_ErrorNone;
}
