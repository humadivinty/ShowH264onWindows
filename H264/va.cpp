/*****************************************************************************
 * va.h: Video Acceleration API for avcodec
 *****************************************************************************
 * Copyright (C) 2012 tuyuandong
 *
 * Authors: Yuandong Tu <tuyuandong@gmail.com>
 *
 * This file is part of FFmpeg. 
 *****************************************************************************/
extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
#include <libavcodec/avcodec.h>
}
#include <va.h>

enum PixelFormat DxGetFormat( AVCodecContext *avctx,
                                          const enum PixelFormat *pi_fmt )
{
	unsigned int i;
	dxva_t *p_va = (dxva_t*)avctx->opaque;

    if( p_va != NULL )
        dxva_Delete( p_va );

    p_va = dxva_New(avctx->codec_id);
    if(p_va != NULL )
    {
        /* Try too look for a supported hw acceleration */
        for(i = 0; pi_fmt[i] != PIX_FMT_NONE; i++ )
        {	/*
            const char *name = av_get_pix_fmt_name(pi_fmt[i]);
            av_log(NULL,AV_LOG_INFO, "Available decoder output format %d (%s)",
                     pi_fmt[i], name ? name : "unknown" );
			*/
            if( p_va->pix_fmt != pi_fmt[i] )
                continue;

            /* We try to call dxva_Setup when possible to detect errors when
             * possible (later is too late) */
            if( avctx->width > 0 && avctx->height > 0
             && dxva_Setup(p_va, &avctx->hwaccel_context,avctx) )
            {
                av_log(NULL,AV_LOG_ERROR, "acceleration setup failure" );
                break;
            }

            //if( p_va->description )
            //    av_log(NULL,AV_LOG_INFO, "Using %s for hardware decoding.",
            //              p_va->description );

            /* FIXME this will disable direct rendering
             * even if a new pixel format is renegotiated
             */
            //p_sys->b_direct_rendering = false;
            //p_sys->p_va = p_va;
            avctx->opaque = p_va;
            avctx->draw_horiz_band = NULL;	
            return pi_fmt[i];
        }

        av_log(NULL,AV_LOG_ERROR, "acceleration not available" );
        dxva_Delete( p_va );
    }	
    avctx->opaque = NULL;
    /* Fallback to default behaviour */
    return avcodec_default_get_format(avctx, pi_fmt );
}


/*****************************************************************************
 * DxGetFrameBuf: callback used by ffmpeg to get a frame buffer.
 *****************************************************************************
 * It is used for direct rendering as well as to get the right PTS for each
 * decoded picture (even in indirect rendering mode).
 *****************************************************************************/
int DxGetFrameBuf( struct AVCodecContext *avctx,
                               AVFrame *pic )
{
    dxva_t *p_va = (dxva_t *)avctx->opaque;
    //picture_t *p_pic;

    /* */
    pic->reordered_opaque = avctx->reordered_opaque;
    pic->opaque = NULL;

    if(p_va)
    {
        /* hwaccel_context is not present in old ffmpeg version */
        if( dxva_Setup(p_va,&avctx->hwaccel_context, avctx) )
        {
            av_log(NULL,AV_LOG_ERROR, "vlc_va_Setup failed" );
            return -1;
        }

        /* */
        pic->type = FF_BUFFER_TYPE_USER;

#if LIBAVCODEC_VERSION_MAJOR < 54
        pic->age = 256*256*256*64;
#endif

        if(dxva_Get(p_va,pic ) )
        {
            av_log(NULL,AV_LOG_ERROR, "VaGrabSurface failed" );
            return -1;
        }
        return 0;
    }

    return avcodec_default_get_buffer(avctx,pic );
}
int  DxReGetFrameBuf( struct AVCodecContext *avctx, AVFrame *pic )
{
    pic->reordered_opaque = avctx->reordered_opaque;

    /* We always use default reget function, it works perfectly fine */
    return avcodec_default_reget_buffer(avctx, pic );
}

void DxReleaseFrameBuf( struct AVCodecContext *avctx,
                                    AVFrame *pic )
{
    dxva_t *p_va = (dxva_t *)avctx->opaque;
	int i;
	
    if(p_va )
    {
        dxva_Release(p_va, pic );
    }
    else if( !pic->opaque )
    {
        /* We can end up here without the AVFrame being allocated by
         * avcodec_default_get_buffer() if VA is used and the frame is
         * released when the decoder is closed
         */
        if( pic->type == FF_BUFFER_TYPE_INTERNAL )
            avcodec_default_release_buffer( avctx,pic );
    }
    else
    {
        //picture_t *p_pic = (picture_t*)p_ff_pic->opaque;
        //decoder_UnlinkPicture( p_dec, p_pic );
        av_log(NULL,AV_LOG_ERROR,"%d %s a error is rasied\e\n");
    }
    for(i = 0; i < 4; i++ )
        pic->data[i] = NULL;
}

int DxPictureCopy(struct AVCodecContext *avctx,AVFrame *src, AVFrame* dst, int width, int height)
{
	dxva_t *p_va = (dxva_t *)avctx->opaque;
	return NULL != p_va ? dxva_Extract(p_va,src,dst) : 0;
}