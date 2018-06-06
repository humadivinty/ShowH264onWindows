/*****************************************************************************
 * dxva2_api.c: Video Acceleration helpers
 *****************************************************************************
 * Copyright (C) 2012 tuyuandong
 *
 * Authors: tuyuandong <tuyaundong@gmail.com>
 *
 * This file is part of FFmpeg. 
 *****************************************************************************/

 
#if _WIN32_WINNT < 0x600
 /* dxva2 needs Vista\win7\win8 support */
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x600
#endif
#define DXVA2API_USE_BITFIELDS
#define COBJMACROS
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/dxva2.h>
}
#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <d3d9.h>
#include <dxva2api.h>
#include "va.h"

#include <initguid.h> /* must be last included to not redefine existing GUIDs */
#include <assert.h>
/* dxva2api.h GUIDs: http://msdn.microsoft.com/en-us/library/windows/desktop/ms697067(v=vs100).aspx
 * assume that they are declared in dxva2api.h */
#define MS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)

#ifdef __MINGW32__
//#include <_mingw.h>

#if !defined(__MINGW64_VERSION_MAJOR)
#undef MS_GUID
#define MS_GUID DEFINE_GUID /* dxva2api.h fails to declare those, redefine as static */
#define DXVA2_E_NEW_VIDEO_DEVICE MAKE_HRESULT(1, 4, 4097)
#else
#include <dxva.h>
#endif

#endif /* __MINGW32__ */


MS_GUID(IID_IDirectXVideoDecoderService, 0xfc51a551, 0xd5e7, 0x11d9, 0xaf,0x55,0x00,0x05,0x4e,0x43,0xff,0x02);
MS_GUID(IID_IDirectXVideoAccelerationService, 0xfc51a550, 0xd5e7, 0x11d9, 0xaf,0x55,0x00,0x05,0x4e,0x43,0xff,0x02);

MS_GUID    (DXVA_NoEncrypt,                         0x1b81bed0, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

/* Codec capabilities GUID, sorted by codec */
MS_GUID    (DXVA2_ModeMPEG2_MoComp,                 0xe6a9f44b, 0x61b0, 0x4563, 0x9e, 0xa4, 0x63, 0xd2, 0xa3, 0xc6, 0xfe, 0x66);
MS_GUID    (DXVA2_ModeMPEG2_IDCT,                   0xbf22ad00, 0x03ea, 0x4690, 0x80, 0x77, 0x47, 0x33, 0x46, 0x20, 0x9b, 0x7e);
MS_GUID    (DXVA2_ModeMPEG2_VLD,                    0xee27417f, 0x5e28, 0x4e65, 0xbe, 0xea, 0x1d, 0x26, 0xb5, 0x08, 0xad, 0xc9);
DEFINE_GUID(DXVA2_ModeMPEG2and1_VLD,                0x86695f12, 0x340e, 0x4f04, 0x9f, 0xd3, 0x92, 0x53, 0xdd, 0x32, 0x74, 0x60);
DEFINE_GUID(DXVA2_ModeMPEG1_VLD,                    0x6f3ec719, 0x3735, 0x42cc, 0x80, 0x63, 0x65, 0xcc, 0x3c, 0xb3, 0x66, 0x16);

MS_GUID    (DXVA2_ModeH264_A,                       0x1b81be64, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeH264_B,                       0x1b81be65, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeH264_C,                       0x1b81be66, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeH264_D,                       0x1b81be67, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeH264_E,                       0x1b81be68, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeH264_F,                       0x1b81be69, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH264_VLD_Multiview,            0x9901CCD3, 0xca12, 0x4b7e, 0x86, 0x7a, 0xe2, 0x22, 0x3d, 0x92, 0x55, 0xc3); // MVC
DEFINE_GUID(DXVA_ModeH264_VLD_WithFMOASO_NoFGT,     0xd5f04ff9, 0x3418, 0x45d8, 0x95, 0x61, 0x32, 0xa7, 0x6a, 0xae, 0x2d, 0xdd);
DEFINE_GUID(DXVADDI_Intel_ModeH264_A,               0x604F8E64, 0x4951, 0x4c54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_C,               0x604F8E66, 0x4951, 0x4c54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_E,               0x604F8E68, 0x4951, 0x4c54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6); // DXVA_Intel_H264_NoFGT_ClearVideo
DEFINE_GUID(DXVA_ModeH264_VLD_NoFGT_Flash,          0x4245F676, 0x2BBC, 0x4166, 0xa0, 0xBB, 0x54, 0xE7, 0xB8, 0x49, 0xC3, 0x80);

MS_GUID    (DXVA2_ModeWMV8_A,                       0x1b81be80, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeWMV8_B,                       0x1b81be81, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

MS_GUID    (DXVA2_ModeWMV9_A,                       0x1b81be90, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeWMV9_B,                       0x1b81be91, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeWMV9_C,                       0x1b81be94, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

MS_GUID    (DXVA2_ModeVC1_A,                        0x1b81beA0, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeVC1_B,                        0x1b81beA1, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeVC1_C,                        0x1b81beA2, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeVC1_D,                        0x1b81beA3, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA2_ModeVC1_D2010,                    0x1b81beA4, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5); // August 2010 update
DEFINE_GUID(DXVA_Intel_VC1_ClearVideo,              0xBCC5DB6D, 0xA2B6, 0x4AF0, 0xAC, 0xE4, 0xAD, 0xB1, 0xF7, 0x87, 0xBC, 0x89);
DEFINE_GUID(DXVA_Intel_VC1_ClearVideo_2,            0xE07EC519, 0xE651, 0x4CD6, 0xAC, 0x84, 0x13, 0x70, 0xCC, 0xEE, 0xC8, 0x51);

DEFINE_GUID(DXVA_nVidia_MPEG4_ASP,                  0x9947EC6F, 0x689B, 0x11DC, 0xA3, 0x20, 0x00, 0x19, 0xDB, 0xBC, 0x41, 0x84);
DEFINE_GUID(DXVA_ModeMPEG4pt2_VLD_Simple,           0xefd64d74, 0xc9e8, 0x41d7, 0xa5, 0xe9, 0xe9, 0xb0, 0xe3, 0x9f, 0xa3, 0x19);
DEFINE_GUID(DXVA_ModeMPEG4pt2_VLD_AdvSimple_NoGMC,  0xed418a9f, 0x010d, 0x4eda, 0x9a, 0xe3, 0x9a, 0x65, 0x35, 0x8d, 0x8d, 0x2e);
DEFINE_GUID(DXVA_ModeMPEG4pt2_VLD_AdvSimple_GMC,    0xab998b5b, 0x4258, 0x44a9, 0x9f, 0xeb, 0x94, 0xe5, 0x97, 0xa6, 0xba, 0xae);
DEFINE_GUID(DXVA_ModeMPEG4pt2_VLD_AdvSimple_Avivo,  0x7C74ADC6, 0xe2ba, 0x4ade, 0x86, 0xde, 0x30, 0xbe, 0xab, 0xb4, 0x0c, 0xc1);


/* */
typedef struct tagdxva2_mode{
    const char   *name;
    const GUID   *guid;
    int          codec;
} dxva2_mode_t;
/* XXX Prefered modes must come first */
static const dxva2_mode_t dxva2_modes[] = {
    /* MPEG-1/2 */
    { "MPEG-2 variable-length decoder",                                               &DXVA2_ModeMPEG2_VLD,                   CODEC_ID_MPEG2VIDEO },
    { "MPEG-2 & MPEG-1 variable-length decoder",                                      &DXVA2_ModeMPEG2and1_VLD,               CODEC_ID_MPEG2VIDEO },
    { "MPEG-2 motion compensation",                                                   &DXVA2_ModeMPEG2_MoComp,                0 },
    { "MPEG-2 inverse discrete cosine transform",                                     &DXVA2_ModeMPEG2_IDCT,                  0 },

    { "MPEG-1 variable-length decoder",                                               &DXVA2_ModeMPEG1_VLD,                   0 },

    /* H.264 */
    { "H.264 variable-length decoder, film grain technology",                         &DXVA2_ModeH264_F,                      CODEC_ID_H264 },
    { "H.264 variable-length decoder, no film grain technology",                      &DXVA2_ModeH264_E,                      CODEC_ID_H264 },
    { "H.264 variable-length decoder, no film grain technology (Intel ClearVideo)",   &DXVADDI_Intel_ModeH264_E,              CODEC_ID_H264 },
    { "H.264 variable-length decoder, no film grain technology, FMO/ASO",             &DXVA_ModeH264_VLD_WithFMOASO_NoFGT,    CODEC_ID_H264 },
    { "H.264 variable-length decoder, no film grain technology, Flash",               &DXVA_ModeH264_VLD_NoFGT_Flash,         CODEC_ID_H264 },

    { "H.264 inverse discrete cosine transform, film grain technology",               &DXVA2_ModeH264_D,                      0 },
    { "H.264 inverse discrete cosine transform, no film grain technology",            &DXVA2_ModeH264_C,                      0 },
    { "H.264 inverse discrete cosine transform, no film grain technology (Intel)",    &DXVADDI_Intel_ModeH264_C,              0 },

    { "H.264 motion compensation, film grain technology",                             &DXVA2_ModeH264_B,                      0 },
    { "H.264 motion compensation, no film grain technology",                          &DXVA2_ModeH264_A,                      0 },
    { "H.264 motion compensation, no film grain technology (Intel)",                  &DXVADDI_Intel_ModeH264_A,              0 },

    /* WMV */
    { "Windows Media Video 8 motion compensation",                                    &DXVA2_ModeWMV8_B,                      0 },
    { "Windows Media Video 8 post processing",                                        &DXVA2_ModeWMV8_A,                      0 },

    { "Windows Media Video 9 IDCT",                                                   &DXVA2_ModeWMV9_C,                      0 },
    { "Windows Media Video 9 motion compensation",                                    &DXVA2_ModeWMV9_B,                      0 },
    { "Windows Media Video 9 post processing",                                        &DXVA2_ModeWMV9_A,                      0 },

    /* VC-1 */
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D,                       CODEC_ID_VC1 },
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D,                       CODEC_ID_WMV3 },
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D2010,                   CODEC_ID_VC1 },
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D2010,                   CODEC_ID_WMV3 },
    { "VC-1 variable-length decoder 2 (Intel)",                                       &DXVA_Intel_VC1_ClearVideo_2,           0 },
    { "VC-1 variable-length decoder (Intel)",                                         &DXVA_Intel_VC1_ClearVideo,             0 },

    { "VC-1 inverse discrete cosine transform",                                       &DXVA2_ModeVC1_C,                       0 },
    { "VC-1 motion compensation",                                                     &DXVA2_ModeVC1_B,                       0 },
    { "VC-1 post processing",                                                         &DXVA2_ModeVC1_A,                       0 },

    /* Xvid/Divx: TODO */
    { "MPEG-4 Part 2 nVidia bitstream decoder",                                       &DXVA_nVidia_MPEG4_ASP,                 0 },
    { "MPEG-4 Part 2 variable-length decoder, Simple Profile",                        &DXVA_ModeMPEG4pt2_VLD_Simple,          0 },
    { "MPEG-4 Part 2 variable-length decoder, Simple&Advanced Profile, no GMC",       &DXVA_ModeMPEG4pt2_VLD_AdvSimple_NoGMC, 0 },
    { "MPEG-4 Part 2 variable-length decoder, Simple&Advanced Profile, GMC",          &DXVA_ModeMPEG4pt2_VLD_AdvSimple_GMC,   0 },
    { "MPEG-4 Part 2 variable-length decoder, Simple&Advanced Profile, Avivo",        &DXVA_ModeMPEG4pt2_VLD_AdvSimple_Avivo, 0 },

    { NULL, NULL, 0 }
};


static const dxva2_mode_t *Dxva2FindMode(const GUID *guid)
{
	unsigned i;
    for (i = 0; dxva2_modes[i].name; i++) {
        if (!memcmp(dxva2_modes[i].guid, guid, sizeof(GUID)))
            return &dxva2_modes[i];
    }
    return NULL;
}

/* */
typedef struct tag_d3d_format{
    const char   *name;
    D3DFORMAT    format;
    uint32_t	 codec;
} d3d_format_t;
/* XXX Prefered format must come first */
static const d3d_format_t d3d_formats[] = {
    { "YV12",   (D3DFORMAT)MAKEFOURCC('Y','V','1','2'),    PIX_FMT_YUV420P},
    { "NV12",   (D3DFORMAT)MAKEFOURCC('N','V','1','2'),    PIX_FMT_NV12 },
    { "IMC3",   (D3DFORMAT)MAKEFOURCC('I','M','C','3'),    PIX_FMT_YUV420P },

    { NULL, (D3DFORMAT)0, 0 }
};

//typedef struct
//{
//    const char   *name;
//    D3DFORMAT    format;    /* D3D format */
//    uint32_t	 fourcc;    /* VLC fourcc */
//    uint32_t     rmask;
//    uint32_t     gmask;
//    uint32_t     bmask;
//} d3d_format_t;

//static const d3d_format_t d3d_formats[] = {
//    /* YV12 is always used for planar 420, the planes are then swapped in Lock() */
//    { "YV12",       MAKEFOURCC('Y','V','1','2'),    PIX_FMT_NV12,  	 0,0,0 },
//    { "YV12",       MAKEFOURCC('Y','V','1','2'),    PIX_FMT_YUVA420P,  0,0,0 },
//    { "YV12",       MAKEFOURCC('Y','V','1','2'),    PIX_FMT_YUVJ420P,  0,0,0 },
//    { "UYVY",       D3DFMT_UYVY,    PIX_FMT_UYVY422,  0,0,0 },
//    { "YUY2",       D3DFMT_YUY2,    PIX_FMT_YUYV422,  0,0,0 },
//    { "X8R8G8B8",   D3DFMT_X8R8G8B8,PIX_FMT_RGB32, 0xff0000, 0x00ff00, 0x0000ff },
//    { "A8R8G8B8",   D3DFMT_A8R8G8B8,PIX_FMT_RGB32, 0xff0000, 0x00ff00, 0x0000ff },
//    { "8G8B8",      D3DFMT_R8G8B8,  PIX_FMT_RGB24, 0xff0000, 0x00ff00, 0x0000ff },
//    { "R5G6B5",     D3DFMT_R5G6B5,  PIX_FMT_RGB565, 0x1f<<11, 0x3f<<5,  0x1f<<0 },
//    { "X1R5G5B5",   D3DFMT_X1R5G5B5,PIX_FMT_RGB555, 0x1f<<10, 0x1f<<5,  0x1f<<0 },

//    { NULL, 0, 0, 0,0,0}
//};

static const d3d_format_t *D3dFindFormat(D3DFORMAT format)
{
	unsigned i;
    for (i = 0; d3d_formats[i].name; i++) {
        if (d3d_formats[i].format == format)
            return &d3d_formats[i];
    }
    return NULL;
}



/* */
typedef struct tag_va_surface{
    LPDIRECT3DSURFACE9 d3d;
    int                refcount;
    unsigned int       order;
} va_surface_t;

#define VA_DXVA2_MAX_SURFACE_COUNT (16)
struct va_sys_t
{
    //vlc_object_t *log;
    int          codec_id;
    int          width;
    int          height;

    /* DLL */
    HINSTANCE             hd3d9_dll;
    HINSTANCE             hdxva2_dll;

    /* Direct3D */
    D3DPRESENT_PARAMETERS  d3dpp;
    LPDIRECT3D9            d3dobj;
    D3DADAPTER_IDENTIFIER9 d3dai;
    LPDIRECT3DDEVICE9      d3ddev;

    /* Device manager */
    UINT                     token;
    IDirect3DDeviceManager9  *devmng;
    HANDLE                   device;

    /* Video service */
    IDirectXVideoDecoderService  *vs;
    GUID                         input;
    D3DFORMAT                    render;

    /* Video decoder */
    DXVA2_ConfigPictureDecode    cfg;
    IDirectXVideoDecoder         *decoder;

    /* Option conversion */
    D3DFORMAT                    output;
    //copy_cache_t                 surface_cache;

    /* */
    struct dxva_context hw;

    /* */
    unsigned     surface_count;
    unsigned     surface_order;
    int          surface_width;
    int          surface_height;
    uint32_t	 surface_chroma;

    va_surface_t surface[VA_DXVA2_MAX_SURFACE_COUNT];
    LPDIRECT3DSURFACE9 hw_surface[VA_DXVA2_MAX_SURFACE_COUNT];
};
//typedef struct va_sys_t va_dxva2_t;


/* */
static int D3dCreateDevice(va_dxva2_t *);
static void D3dDestroyDevice(va_dxva2_t *);
static char *DxDescribe(va_dxva2_t *);

static int D3dCreateDeviceManager(va_dxva2_t *);
static void D3dDestroyDeviceManager(va_dxva2_t *);

static int DxCreateVideoService(va_dxva2_t *);
static void DxDestroyVideoService(va_dxva2_t *);
static int DxFindVideoServiceConversion(va_dxva2_t *, GUID *input, D3DFORMAT *output);

static int DxCreateVideoDecoder(va_dxva2_t *,
                                int codec_id, const AVCodecContext *avctx);
static void DxDestroyVideoDecoder(va_dxva2_t *);
static int DxResetVideoDecoder(va_dxva2_t *);

static void DxCreateVideoConversion(va_dxva2_t *);
static void DxDestroyVideoConversion(va_dxva2_t *);

static int Setup(va_dxva2_t *va, void **hw, const AVCodecContext *avctx)
{
    //va_dxva2_t *va = vlc_va_dxva2_Get(external);
	unsigned i;
	
    if (va->width == avctx->width&& va->height == avctx->height && va->decoder)
        goto ok;

    /* */
    DxDestroyVideoConversion(va);
    DxDestroyVideoDecoder(va);

    *hw = NULL;
    if (avctx->width <= 0 || avctx->height <= 0)
        return -1;

    if (DxCreateVideoDecoder(va, va->codec_id, avctx))
        return -1;
    /* */
    va->hw.decoder = va->decoder;
    va->hw.cfg = &va->cfg;
    va->hw.surface_count = va->surface_count;
    va->hw.surface = va->hw_surface;
    for (i = 0; i < va->surface_count; i++)
        va->hw.surface[i] = va->surface[i].d3d;

    /* */
    DxCreateVideoConversion(va);

    /* */
ok:
    *hw = &va->hw;
    const d3d_format_t *output = D3dFindFormat(va->output);
    //*chroma = output->codec;
    return 0;
}

/* FIXME it is nearly common with VAAPI */
static int Get(va_dxva2_t *va, AVFrame *ff)
{
	unsigned i, old;
    /* Check the device */
    HRESULT hr = IDirect3DDeviceManager9_TestDevice(va->devmng, va->device);
    if (hr == DXVA2_E_NEW_VIDEO_DEVICE) {
        if (DxResetVideoDecoder(va))
            return -1;
    } else if (FAILED(hr)) {
        av_log(NULL,AV_LOG_ERROR, "IDirect3DDeviceManager9_TestDevice %u\n", (unsigned)hr);
        return -1;
    }	
    /* Grab an unused surface, in case none are, try the oldest
     * XXX using the oldest is a workaround in case a problem happens with libavcodec */
    for (i = 0, old = 0; i < va->surface_count; i++) {
        va_surface_t *surface = &va->surface[i];

        if (!surface->refcount)
            break;

        if (surface->order < va->surface[old].order)
            old = i;
    }
    if (i >= va->surface_count)
        i = old;

    va_surface_t *surface = &va->surface[i];

    surface->refcount = 1;
    surface->order = va->surface_order++;

    /* */
    for (i = 0; i < 4; i++) {
        ff->data[i] = NULL;
        ff->linesize[i] = 0;

        if (i == 0 || i == 3)
            ff->data[i] = (uint8_t*)surface->d3d;/* Yummie */
    }
    return 0;
}

static void Release(va_dxva2_t *va, AVFrame *ff)
{
	unsigned i;
    LPDIRECT3DSURFACE9 d3d = (LPDIRECT3DSURFACE9)(uintptr_t)ff->data[3];

    for (i = 0; i < va->surface_count; i++) {
        va_surface_t *surface = &va->surface[i];

        if (surface->d3d == d3d)
        {
			surface->refcount--;
		}
    }
}

static void Close(va_dxva2_t* va)
{
    DxDestroyVideoConversion(va);
    DxDestroyVideoDecoder(va);
    DxDestroyVideoService(va);
    D3dDestroyDeviceManager(va);
    D3dDestroyDevice(va);

    if (va->hdxva2_dll)
        FreeLibrary(va->hdxva2_dll);
    if (va->hd3d9_dll)
        FreeLibrary(va->hd3d9_dll);
	
    free(va);
}

static int Open(va_dxva2_t** pva, int codec_id)
{
    va_dxva2_t *va = (va_dxva2_t *)calloc(1, sizeof(*va));
    if (!va)
        return NULL;

    *pva = va;
	
    /* */
    va->codec_id = codec_id;

    /* Load dll*/
    va->hd3d9_dll = LoadLibrary(TEXT("D3D9.DLL"));
    if (!va->hd3d9_dll) {
        av_log(NULL,AV_LOG_WARNING, "cannot load d3d9.dll\n");
        goto error;
    }
    va->hdxva2_dll = LoadLibrary(TEXT("DXVA2.DLL"));
    if (!va->hdxva2_dll) {
        av_log(NULL,AV_LOG_WARNING, "cannot load dxva2.dll\n");
        goto error;
    }
    av_log(NULL,AV_LOG_INFO, "DLLs loaded\n");
    /* */
    if (D3dCreateDevice(va)) {
        av_log(NULL,AV_LOG_ERROR, "Failed to create Direct3D device\n");
        goto error;
    }
    av_log(NULL,AV_LOG_INFO, "D3dCreateDevice succeed\n");

    if (D3dCreateDeviceManager(va)) {
        av_log(NULL,AV_LOG_ERROR, "D3dCreateDeviceManager failed\n");
        goto error;
    }

    if (DxCreateVideoService(va)) {
        av_log(NULL,AV_LOG_ERROR, "DxCreateVideoService failed\n");
        goto error;
    }

    /* */
    if (DxFindVideoServiceConversion(va, &va->input, &va->render)) {
        av_log(NULL,AV_LOG_ERROR, "DxFindVideoServiceConversion failed\n");
        goto error;
    }
    /* TODO print the hardware name/vendor for debugging purposes */
    //external->description = DxDescribe(va);
    //external->pix_fmt = PIX_FMT_DXVA2_VLD;
    //external->setup   = Setup;
    //external->get     = Get;
    //external->release = Release;
    //external->extract = Extract;
    return 0;

error:
    Close(va);
    return -1;
}
/* */

static void CopyPlane(uint8_t *dst, size_t dst_pitch,
                      const uint8_t *src, size_t src_pitch,
                      unsigned width, unsigned height)
{
    unsigned y;
    for (y = 0; y < height; y++) {
        memcpy(dst, src, width);
        src += src_pitch;
        dst += dst_pitch;
    }
}

static void SplitPlanes(uint8_t *dstu, size_t dstu_pitch,
                        uint8_t *dstv, size_t dstv_pitch,
                        const uint8_t *src, size_t src_pitch,
                        unsigned width, unsigned height)
{
    unsigned x,y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            dstu[x] = src[2*x+0];
            dstv[x] = src[2*x+1];
        }
        src  += src_pitch;
        dstu += dstu_pitch;
        dstv += dstv_pitch;
    }
}

static void CopyFromNv12(AVFrame *dst, uint8_t *src[2], size_t src_pitch[2],
                  unsigned width, unsigned height)
{
    CopyPlane(dst->data[0], dst->linesize[0],
              src[0], src_pitch[0],
              width, height);
    SplitPlanes(dst->data[2], dst->linesize[2],
                dst->data[1], dst->linesize[1],
                src[1], src_pitch[1],
                width/2, height/2);
}

static void CopyFromYv12(AVFrame *dst, uint8_t *src[3], size_t src_pitch[3],
                  unsigned width, unsigned height)
{
     CopyPlane(dst->data[0], dst->linesize[0],
               src[0], src_pitch[0], width, height);
     CopyPlane(dst->data[1], dst->linesize[1],
               src[1], src_pitch[1], width / 2, height / 2);
     CopyPlane(dst->data[2], dst->linesize[2],
               src[1], src_pitch[2], width / 2, height / 2);
}

static int Extract(va_dxva2_t *va, AVFrame *src, AVFrame *dst)
{
    LPDIRECT3DSURFACE9 d3d = (LPDIRECT3DSURFACE9)(uintptr_t)src->data[3];
    if (!dst->data[0])
        return -1;

    /* */
    if(va->output != MAKEFOURCC('Y','V','1','2'))
		return -1;

    /* */
    D3DLOCKED_RECT lock;
    if (FAILED(IDirect3DSurface9_LockRect(d3d, &lock, NULL, D3DLOCK_READONLY))) {
        av_log(NULL,AV_LOG_ERROR, "Failed to lock surface\n");
        return -1;
    }

    if (va->render == MAKEFOURCC('Y','V','1','2') ||
        va->render == MAKEFOURCC('I','M','C','3')) {
        int  imc3 = va->render == MAKEFOURCC('I','M','C','3');
        size_t chroma_pitch = imc3 ? lock.Pitch : (lock.Pitch / 2);

        size_t pitch[3] = {
            lock.Pitch,
            chroma_pitch,
            chroma_pitch,
        };

        uint8_t *plane[3] = {
            (uint8_t*)lock.pBits,
            (uint8_t*)lock.pBits + pitch[0] * va->surface_height,
            (uint8_t*)lock.pBits + pitch[0] * va->surface_height
                                 + pitch[1] * va->surface_height / 2,
        };

        if (imc3) {
            uint8_t *V = plane[1];
            plane[1] = plane[2];
            plane[2] = V;
        }
        CopyFromYv12(dst, plane, pitch,va->width, va->height);
    } else {
        //assert(va->render == MAKEFOURCC('N','V','1','2'));
        uint8_t *plane[2] = {
            (uint8_t*)lock.pBits,
            (uint8_t*)lock.pBits + lock.Pitch * va->surface_height
        };
        size_t  pitch[2] = {
            lock.Pitch,
            lock.Pitch,
        };
        CopyFromNv12(dst, plane, pitch,va->width, va->height);
    }

    /* */
    IDirect3DSurface9_UnlockRect(d3d);
    return 0;
}

dxva_t* dxva_New(int codec_id)
{
    dxva_t *p_va = (dxva_t*)av_mallocz(sizeof(dxva_t));
	p_va->setup = Setup;
	p_va->get = Get;
	p_va->release = Release;
	p_va->extract = Extract;
	p_va->pix_fmt = PIX_FMT_DXVA2_VLD;
	if(Open(&p_va->sys,codec_id) <0)
	{
		av_free(p_va);
		p_va =  NULL;		
	}
    return p_va;
}

void dxva_Delete(dxva_t *va)
{
	Close(va->sys);
	av_free(va);
}

/**
 * It creates a Direct3D device usable for DXVA 2
 */
static int D3dCreateDevice(va_dxva2_t *va)
{
    /* */
    typedef LPDIRECT3D9 (WINAPI *Create9Func)(UINT SDKVersion);
    Create9Func Create9 = (Create9Func)GetProcAddress(va->hd3d9_dll,
                                     TEXT("Direct3DCreate9"));
    if (!Create9) {
		av_log(NULL,AV_LOG_ERROR,"Cannot locate reference to Direct3DCreate9 ABI in DLL\n");
        return -1;
    }

    /* */
    LPDIRECT3D9 d3dobj;
    d3dobj = Create9(D3D_SDK_VERSION);
    if (!d3dobj) {
        av_log(NULL,AV_LOG_ERROR,"Direct3DCreate9 failed\n");
        return -1;
    }
    va->d3dobj = d3dobj;

    /* */
    D3DADAPTER_IDENTIFIER9 *d3dai = &va->d3dai;
    if (FAILED(IDirect3D9_GetAdapterIdentifier(va->d3dobj,D3DADAPTER_DEFAULT, 0, d3dai))) {
        av_log(NULL,AV_LOG_WARNING,"IDirect3D9_GetAdapterIdentifier failed\n");
        ZeroMemory(d3dai, sizeof(*d3dai));
    }

    /* */
    D3DPRESENT_PARAMETERS *d3dpp = &va->d3dpp;
    ZeroMemory(d3dpp, sizeof(*d3dpp));
    d3dpp->Flags                  = D3DPRESENTFLAG_VIDEO;
    d3dpp->Windowed               = TRUE;
    d3dpp->hDeviceWindow          = NULL;
    d3dpp->SwapEffect             = D3DSWAPEFFECT_DISCARD;
    d3dpp->MultiSampleType        = D3DMULTISAMPLE_NONE;
    d3dpp->PresentationInterval   = D3DPRESENT_INTERVAL_DEFAULT;
    d3dpp->BackBufferCount        = 0;                  /* FIXME what to put here */
    d3dpp->BackBufferFormat       = D3DFMT_X8R8G8B8;    /* FIXME what to put here */
    d3dpp->BackBufferWidth        = 0;
    d3dpp->BackBufferHeight       = 0;
    d3dpp->EnableAutoDepthStencil = FALSE;

    /* Direct3D needs a HWND to create a device, even without using ::Present
    this HWND is used to alert Direct3D when there's a change of focus window.
    For now, use GetDesktopWindow, as it looks harmless */
    LPDIRECT3DDEVICE9 d3ddev;
    if (FAILED(IDirect3D9_CreateDevice(d3dobj, D3DADAPTER_DEFAULT,
                                       D3DDEVTYPE_HAL, GetDesktopWindow(),
                                       D3DCREATE_SOFTWARE_VERTEXPROCESSING |
                                       D3DCREATE_MULTITHREADED,
                                       d3dpp, &d3ddev))) {
        av_log(NULL,AV_LOG_ERROR,"IDirect3D9_CreateDevice failed\n");
        return -1;
    }
    va->d3ddev = d3ddev;

    return 0;
}

/**
 * It releases a Direct3D device and its resources.
 */
static void D3dDestroyDevice(va_dxva2_t *va)
{
    if (va->d3ddev)
        IDirect3DDevice9_Release(va->d3ddev);
    if (va->d3dobj)
        IDirect3D9_Release(va->d3dobj);
}

/**
 * It describes our Direct3D object
 */
static char *DxDescribe(va_dxva2_t *va)
{
    static const struct {
        unsigned id;
        char     name[32];
    } vendors [] = {
        { 0x1002, "ATI" },
        { 0x10DE, "NVIDIA" },
        { 0x1106, "VIA" },
        { 0x8086, "Intel" },
        { 0x5333, "S3 Graphics" },
        { 0, "" }
    };
    D3DADAPTER_IDENTIFIER9 *id = &va->d3dai;
    const char *vendor = "Unknown";
	int i;
	
	static char description[255];
	
    for (i = 0; vendors[i].id != 0; i++) {
        if (vendors[i].id == id->VendorId) {
            vendor = vendors[i].name;
            break;
        }
    }

    if (sprintf(description, "DXVA2 (vendor %lu(%s), device %lu, revision %lu)",
                 id->VendorId, vendor, id->DeviceId, id->Revision) < 0)
        return NULL;
    return description;
}

/**
 * It creates a Direct3D device manager
 */
static int D3dCreateDeviceManager(va_dxva2_t *va)
{
    typedef HRESULT (WINAPI *CreateDeviceManager9Func)(UINT *pResetToken,
                                           IDirect3DDeviceManager9 **);
    CreateDeviceManager9Func CreateDeviceManager9 =
      (CreateDeviceManager9Func)GetProcAddress(va->hdxva2_dll,
                             TEXT("DXVA2CreateDirect3DDeviceManager9"));

    if (!CreateDeviceManager9) {
        av_log(NULL,AV_LOG_ERROR,"cannot load function\n");
        return -1;
    }
    av_log(NULL,AV_LOG_INFO,"OurDirect3DCreateDeviceManager9 Success!\n");

    UINT token;
    IDirect3DDeviceManager9 *devmng;
    if (FAILED(CreateDeviceManager9(&token, &devmng))) {
        av_log(NULL,AV_LOG_ERROR, " OurDirect3DCreateDeviceManager9 failed\n");
        return -1;
    }
    va->token  = token;
    va->devmng = devmng;
    av_log(NULL,AV_LOG_INFO,"obtained IDirect3DDeviceManager9\n");

    HRESULT hr = IDirect3DDeviceManager9_ResetDevice(devmng, va->d3ddev, token);
    if (FAILED(hr)) {
        av_log(NULL,AV_LOG_ERROR,"IDirect3DDeviceManager9_ResetDevice failed: %08x\n", (unsigned)hr);
        return -1;
    }
    return 0;
}
/**
 * It destroys a Direct3D device manager
 */
static void D3dDestroyDeviceManager(va_dxva2_t *va)
{
    if (va->devmng)
        IDirect3DDeviceManager9_Release(va->devmng);
}

/**
 * It creates a DirectX video service
 */
static int DxCreateVideoService(va_dxva2_t *va)
{
    typedef HRESULT (WINAPI *CreateVideoServiceFunc)(IDirect3DDevice9 *,
                                         REFIID riid,
                                         void **ppService);
    CreateVideoServiceFunc CreateVideoService =
      (CreateVideoServiceFunc)GetProcAddress(va->hdxva2_dll,
                             TEXT("DXVA2CreateVideoService"));

    if (!CreateVideoService) {
        av_log(NULL,AV_LOG_ERROR, "cannot load function\n");
        return -1;
    }
    av_log(NULL,AV_LOG_INFO, "DXVA2CreateVideoService Success!\n");

    HRESULT hr;

    HANDLE device;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(va->devmng, &device);
    if (FAILED(hr)) {
        av_log(NULL,AV_LOG_ERROR, "OpenDeviceHandle failed\n");
        return -1;
    }
    va->device = device;

    IDirectXVideoDecoderService *vs;
    hr = IDirect3DDeviceManager9_GetVideoService(va->devmng, device,
                                                 IID_IDirectXVideoDecoderService,
                                                 (void**)&vs);
    if (FAILED(hr)) {
        av_log(NULL,AV_LOG_ERROR, "GetVideoService failed\n");
        return -1;
    }
    va->vs = vs;

    return 0;
}

/**
 * It destroys a DirectX video service
 */
static void DxDestroyVideoService(va_dxva2_t *va)
{
    if (va->device)
        IDirect3DDeviceManager9_CloseDeviceHandle(va->devmng, va->device);
    if (va->vs)
        IDirectXVideoDecoderService_Release(va->vs);
}

/**
 * Find the best suited decoder mode GUID and render format.
 */
static int DxFindVideoServiceConversion(va_dxva2_t *va, GUID *input, D3DFORMAT *output)
{
    /* Retreive supported modes from the decoder service */
    UINT input_count = 0;
    GUID *input_list = NULL;
	unsigned i,j;
    if (FAILED(IDirectXVideoDecoderService_GetDecoderDeviceGuids(va->vs,
                                                                 &input_count,
                                                                 &input_list))) {
        av_log(NULL,AV_LOG_ERROR, "IDirectXVideoDecoderService_GetDecoderDeviceGuids failed\n");
        return -1;
    }
#if 0	//打印系统解压器的guid
	for (i = 0; i < input_count; i++) {
		av_log(NULL, AV_LOG_INFO, "DecoderDeviceGuids:%d -->{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n", i, input_list[i].Data1, input_list[i].Data2, input_list[i].Data3, input_list[i].Data4[0], input_list[i].Data4[1], input_list[i].Data4[2], input_list[i].Data4[3], input_list[i].Data4[4], input_list[i].Data4[5], input_list[i].Data4[6], input_list[i].Data4[7]);
	}
	for (i = 0; dxva2_modes[i].name; i++) {
		av_log(NULL,AV_LOG_INFO, "dxva mode:%d-->{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n", i, dxva2_modes[i].guid->Data1, dxva2_modes[i].guid->Data2, dxva2_modes[i].guid->Data3, dxva2_modes[i].guid->Data4[0], dxva2_modes[i].guid->Data4[1], dxva2_modes[i].guid->Data4[2], dxva2_modes[i].guid->Data4[3], dxva2_modes[i].guid->Data4[4], dxva2_modes[i].guid->Data4[5], dxva2_modes[i].guid->Data4[6], dxva2_modes[i].guid->Data4[7]);
	}
#endif
    for (i = 0; i < input_count; i++) {
        const GUID *g = &input_list[i];
        const dxva2_mode_t *mode = Dxva2FindMode(g);
        if (mode) {
            av_log(NULL,AV_LOG_INFO, "- '%s' is supported by hardware\n", mode->name);
        } else {
            av_log(NULL,AV_LOG_WARNING, "- Unknown GUID = %08X-%04x-%04x-%02X%02X%02X%02X%02X%02X\n",
                     (unsigned)g->Data1, g->Data2, g->Data3, g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3], g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);
        }
    }
    /* Try all supported mode by our priority */
    for (i = 0; dxva2_modes[i].name; i++) {
        const dxva2_mode_t *mode = &dxva2_modes[i];
		/* */
        int is_suported = FALSE;
		const GUID* g;
		unsigned j;
		
        if (!mode->codec || mode->codec != va->codec_id)
            continue;

        for (g = &input_list[0]; !is_suported && g < &input_list[input_count]; g++) {
            is_suported = !memcmp(mode->guid, g, sizeof(GUID));
        }
        if (!is_suported)
            continue;

        /* */
        av_log(NULL,AV_LOG_INFO, "Trying to use '%s' as input\n", mode->name);
		
        UINT      output_count = 0;
        D3DFORMAT *output_list = NULL;
        if (FAILED(IDirectXVideoDecoderService_GetDecoderRenderTargets(va->vs, *mode->guid,
                                                                       &output_count,
                                                                       &output_list))) {
            av_log(NULL,AV_LOG_ERROR, "IDirectXVideoDecoderService_GetDecoderRenderTargets failed\n");
            continue;
        }
        for (j = 0; j < output_count; j++) {
            const D3DFORMAT f = output_list[j];
            const d3d_format_t *format = D3dFindFormat(f);
            if (format) {
                av_log(NULL,AV_LOG_INFO, "%s is supported for output\n", format->name);
            } else {
                av_log(NULL,AV_LOG_INFO, "%d is supported for output (%4.4s)\n", f, (const char*)&f);
            }
        }

        /* */
        for (j = 0; d3d_formats[j].name; j++) {
            const d3d_format_t *format = &d3d_formats[j];

            /* */
            int is_suported = FALSE;
			unsigned k;
            for (k = 0; !is_suported && k < output_count; k++) {
                is_suported = format->format == output_list[k];
            }
            if (!is_suported)
                continue;

            /* We have our solution */
            av_log(NULL,AV_LOG_INFO, "Using '%s' to decode to '%s'\n", mode->name, format->name);
            *input  = *mode->guid;
            *output = format->format;
            CoTaskMemFree(output_list);
            CoTaskMemFree(input_list);
            return 0;
        }
        CoTaskMemFree(output_list);
    }
    CoTaskMemFree(input_list);
	
    return 0;
}


/**
 * It creates a DXVA2 decoder using the given video format
 */
static int DxCreateVideoDecoder(va_dxva2_t *va,
                                int codec_id, const AVCodecContext *avctx)
{
	unsigned i;
	/* */
    av_log(NULL,AV_LOG_INFO, "DxCreateVideoDecoder id %d %dx%d\n",
            codec_id, avctx->width, avctx->height);

    va->width  = avctx->width;
    va->height = avctx->height;

    /* Allocates all surfaces needed for the decoder */
    va->surface_width  = (avctx->width  + 15) & ~15;
    va->surface_height = (avctx->height + 15) & ~15;
    switch (codec_id) {
    case CODEC_ID_H264:
        va->surface_count = VA_DXVA2_MAX_SURFACE_COUNT;//16 + 1;
        break;
    default:
        va->surface_count = 2 + 1;
        break;
    }
    LPDIRECT3DSURFACE9 surface_list[VA_DXVA2_MAX_SURFACE_COUNT];
    if (FAILED(IDirectXVideoDecoderService_CreateSurface(va->vs,
                                                         va->surface_width,
                                                         va->surface_height,
                                                         va->surface_count - 1,
                                                         va->render,
                                                         D3DPOOL_DEFAULT,
                                                         0,
                                                         DXVA2_VideoDecoderRenderTarget,
                                                         surface_list,
                                                         NULL))) {
        av_log(NULL,AV_LOG_ERROR, "IDirectXVideoAccelerationService_CreateSurface failed\n");
        va->surface_count = 0;
        return -1;
    }
    for (i = 0; i < va->surface_count; i++) {
        va_surface_t *surface = &va->surface[i];
        surface->d3d = surface_list[i];
        surface->refcount = 0;
        surface->order = 0;
    }
    av_log(NULL,AV_LOG_INFO, "IDirectXVideoAccelerationService_CreateSurface succeed with %d surfaces (%dx%d)\n",
            va->surface_count, avctx->width, avctx->height);

    /* */
    DXVA2_VideoDesc dsc;
    ZeroMemory(&dsc, sizeof(dsc));
    dsc.SampleWidth     = avctx->width;
    dsc.SampleHeight    = avctx->height;
    dsc.Format          = va->render;
    //if (fmt->i_frame_rate > 0 && fmt->i_frame_rate_base > 0) {
    if (avctx->time_base.num > 0 && avctx->time_base.den > 0) {
        //dsc.InputSampleFreq.Numerator   = fmt->i_frame_rate;
        //dsc.InputSampleFreq.Denominator = fmt->i_frame_rate_base;
        dsc.InputSampleFreq.Numerator   = avctx->time_base.num;
        dsc.InputSampleFreq.Denominator = avctx->time_base.den;
    } else {
        dsc.InputSampleFreq.Numerator   = 0;
        dsc.InputSampleFreq.Denominator = 0;
    }
    dsc.OutputFrameFreq = dsc.InputSampleFreq;
    dsc.UABProtectionLevel = FALSE;
    dsc.Reserved = 0;

    /* FIXME I am unsure we can let unknown everywhere */
    DXVA2_ExtendedFormat *ext = &dsc.SampleFormat;
    ext->SampleFormat = 0;//DXVA2_SampleUnknown;
    ext->VideoChromaSubsampling = 0;//DXVA2_VideoChromaSubsampling_Unknown;
    ext->NominalRange = 0;//DXVA2_NominalRange_Unknown;
    ext->VideoTransferMatrix = 0;//DXVA2_VideoTransferMatrix_Unknown;
    ext->VideoLighting = 0;//DXVA2_VideoLighting_Unknown;
    ext->VideoPrimaries = 0;//DXVA2_VideoPrimaries_Unknown;
    ext->VideoTransferFunction = 0;//DXVA2_VideoTransFunc_Unknown;

    /* List all configurations available for the decoder */
    UINT                      cfg_count = 0;
    DXVA2_ConfigPictureDecode *cfg_list = NULL;
    if (FAILED(IDirectXVideoDecoderService_GetDecoderConfigurations(va->vs,
                                                                    va->input,
                                                                    &dsc,
                                                                    NULL,
                                                                    &cfg_count,
                                                                    &cfg_list))) {
        av_log(NULL,AV_LOG_ERROR, "IDirectXVideoDecoderService_GetDecoderConfigurations failed\n");
        return -1;
    }
    av_log(NULL,AV_LOG_INFO, "we got %d decoder configurations\n", cfg_count);

    /* Select the best decoder configuration */
    int cfg_score = 0;
    for (i = 0; i < cfg_count; i++) {
        const DXVA2_ConfigPictureDecode *cfg = &cfg_list[i];

        /* */
        av_log(NULL,AV_LOG_INFO, "configuration[%d] ConfigBitstreamRaw %d\n",
                i, cfg->ConfigBitstreamRaw);

        /* */
        int score;
        if (cfg->ConfigBitstreamRaw == 1)
            score = 1;
        else if (codec_id == CODEC_ID_H264 && cfg->ConfigBitstreamRaw == 2)
            score = 2;
        else
            continue;
        if (!memcmp(&cfg->guidConfigBitstreamEncryption, &DXVA_NoEncrypt, sizeof(GUID)))
            score += 16;

        if (cfg_score < score) {
            va->cfg = *cfg;
            cfg_score = score;
        }
    }
    CoTaskMemFree(cfg_list);
    if (cfg_score <= 0) {
        av_log(NULL,AV_LOG_ERROR, "Failed to find a supported decoder configuration\n");
        return -1;
    }

    /* Create the decoder */
    IDirectXVideoDecoder *decoder;
    if (FAILED(IDirectXVideoDecoderService_CreateVideoDecoder(va->vs,
                                                              va->input,
                                                              &dsc,
                                                              &va->cfg,
                                                              surface_list,
                                                              va->surface_count,
                                                              &decoder))) {
        av_log(NULL,AV_LOG_ERROR, "IDirectXVideoDecoderService_CreateVideoDecoder failed\n");
        return -1;
    }
    va->decoder = decoder;
    av_log(NULL,AV_LOG_INFO, "IDirectXVideoDecoderService_CreateVideoDecoder succeed\n");
	
    return 0;
}
static void DxDestroyVideoDecoder(va_dxva2_t *va)
{
	unsigned i;
    if (va->decoder)
        IDirectXVideoDecoder_Release(va->decoder);
    va->decoder = NULL;

    for ( i = 0; i < va->surface_count; i++)
        IDirect3DSurface9_Release(va->surface[i].d3d);
    va->surface_count = 0;
}

static int DxResetVideoDecoder(va_dxva2_t *va)
{
    av_log(NULL,AV_LOG_INFO, "DxResetVideoDecoder unimplemented\n");
    return -1;
}

static void DxCreateVideoConversion(va_dxva2_t *va)
{
    switch (va->render) {
    case MAKEFOURCC('N','V','1','2'):
    case MAKEFOURCC('I','M','C','3'):
        va->output = (D3DFORMAT)MAKEFOURCC('Y','V','1','2');
        break;
    default:
        va->output = va->render;
        break;
    }
   // CopyInitCache(&va->surface_cache, va->surface_width);
}

static void DxDestroyVideoConversion(va_dxva2_t *va)
{
    //CopyCleanCache(&va->surface_cache);
}

