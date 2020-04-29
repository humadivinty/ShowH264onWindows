#include "stdafx.h"
#include "GFFX.h"
#include "va.h"
#include <vfw.h>

//#define USE_DXVA 
extern "C"
{
    void* gvp_create_new_queue();
    void gvp_delete_queue(void* pq);
    int gvp_enqueue(void* pq, void* data, int length);
    int gvp_dequeue(void* pq, void** data, int* length, int block);
    int gvp_queue_size(void* pq);
}

// 初始化标记
BOOL CGFFX::m_fInitialize = FALSE;

#define  FLG_BMP 1
#define  MAX_IMG_SIZE 10*1024*1024

int simplest_bgr24_to_bmp(unsigned char*pSrcBuffer, int iBufferLen, int width, int height, unsigned char * pDestBuffer, int* OutPutLength)
{
	typedef struct
	{
		long imageSize;
		long blank;
		long startPosition;        
	}BmpHead;

	typedef struct
	{
		long  Length;
		long  width;
		long  height;
		unsigned short  colorPlane;
		unsigned short  bitColor;
		long  zipFormat;
		long  realSize;
		long  xPels;
		long  yPels;
		long  colorUse;
		long  colorImportant;
	}InfoHead;

	//int i = 0, j = 0;
	BmpHead m_BMPHeader = { 0, 0, 0 };

	InfoHead  m_BMPInfoHeader = { 0 , 0 , 0, 0, 0, 0, 0, 0, 0, 0, 0};
	char bfType[2] = { 'B', 'M' };
	int header_size = sizeof(bfType)+sizeof(BmpHead)+sizeof(InfoHead);
	unsigned char *rgb24_buffer = pSrcBuffer;
	unsigned char *pDest = pDestBuffer;

	m_BMPHeader.imageSize = 3 * width*height + header_size;
	m_BMPHeader.startPosition = header_size;

	m_BMPInfoHeader.Length = sizeof(InfoHead);
	m_BMPInfoHeader.width = width;
	//BMP storage pixel data in opposite direction of Y-axis (from bottom to top).  
	m_BMPInfoHeader.height = -height;
	m_BMPInfoHeader.colorPlane = 1;
	m_BMPInfoHeader.bitColor = 24;
	m_BMPInfoHeader.realSize = 3 * width*height;

	memcpy(pDest, bfType, sizeof(bfType));
	pDest += sizeof(bfType);
	memcpy(pDest, &m_BMPHeader, sizeof(m_BMPHeader));
	pDest += sizeof(m_BMPHeader);
	memcpy(pDest, &m_BMPInfoHeader, sizeof(m_BMPInfoHeader));
	pDest += sizeof(m_BMPInfoHeader);

	memcpy(pDest, rgb24_buffer, iBufferLen);

	*OutPutLength = 3 * width*height + sizeof(bfType)+sizeof(m_BMPHeader)+sizeof(m_BMPInfoHeader);

	//BMP save R1|G1|B1,R2|G2|B2 as B1|G1|R1,B2|G2|R2  
	//It saves pixel data in Little Endian  
	//So we change 'R' and 'B'  
	//for (j = 0; j<height; j++){
	//    for (i = 0; i<width; i++){
	//        char temp = rgb24_buffer[(j*width + i) * 3 + 2];
	//        rgb24_buffer[(j*width + i) * 3 + 2] = rgb24_buffer[(j*width + i) * 3 + 0];
	//        rgb24_buffer[(j*width + i) * 3 + 0] = temp;
	//    }
	//}

	return 0;
}


CGFFX::CGFFX()
{
    m_hPlayThread = NULL;
    m_hH264Thread = NULL;
    m_iVideoStream = -1;
    m_fRunning = FALSE;
    m_pFormatCtx = NULL;
    m_pContext = NULL;
    m_hWnd = NULL;

    m_lFps = 0;
    m_lUserData = 0;
    m_lRecordSecond = 0;
    m_iStartRecordTime = 0;
    strcpy_s(m_szURL, "");

    m_pReceivePacket = NULL;
    m_pDecodePacket = NULL;
    m_pFrame = NULL;
    m_pSws = NULL;
    m_pBmp = NULL;

#ifdef USE_CAPTURE
	m_pSrcImg = NULL;
	m_pDestImg = NULL;
#endif

    m_hQueue = gvp_create_new_queue();
}

CGFFX::~CGFFX(void)
{
    Stop();
    gvp_delete_queue(m_hQueue);
	m_hQueue = NULL;

#ifdef USE_CAPTURE
	SAFE_DELETE_ARRAY(m_pSrcImg);
	SAFE_DELETE_ARRAY(m_pDestImg);
#endif
}

// 视频库初始化
HRESULT CGFFX::Initialize(void)
{
    if (!m_fInitialize)
    {
        m_fInitialize = TRUE;
        avcodec_register_all();
        av_register_all();
        avformat_network_init();
    }
    return S_OK;
}

// 设置退出信号
VOID CGFFX::SetExitStatus()
{
    m_fRunning = FALSE;
    m_hWnd = NULL;
}

// 播放
HRESULT CGFFX::Play(HWND hWnd, LPCSTR szURL)
{	
    Initialize();
    Stop();

    // 配置窗口信息
    m_hWnd = hWnd;
    //if (NULL != hWnd && ::IsWindow(hWnd))
    //{
    //	RECT rc;
    //	GetClientRect(hWnd, &rc);
    //	m_hWnd = capCreateCaptureWindow("视频播放器", WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|WS_DISABLED, 0, 0, rc.right - rc.left, rc.bottom - rc.top, hWnd, 0);
    //}
    //else
    //{
    //	// 独立窗口
    //	m_hWnd = capCreateCaptureWindow("视频播放器", WS_VISIBLE|WS_OVERLAPPEDWINDOW, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0);
    //}

    // URL
    strcpy_s(m_szURL, szURL);
    m_fRunning = TRUE;
    m_pFormatCtx = NULL;	
    m_fDecoding = FALSE;
    DWORD id;

    // 创建RTSP接收线程
    m_hPlayThread = CreateThread(NULL, 0, OnRTSPReceiveThread, this, 0, &id);
    // 创建解压H264线程
    m_hH264Thread = CreateThread(NULL, 0, OnH264DecodeThread, this, 0, &id);

    return NULL != m_hPlayThread && NULL != m_hH264Thread ? S_OK : E_FAIL;
}

// 停止
VOID CGFFX::Stop(VOID)
{
    MSG msg;
    m_fRunning = FALSE;
    m_hWnd = NULL;

	// 停止播放线程
	DWORD dwRet = -1;
	while (NULL != m_hPlayThread && WAIT_OBJECT_0 != dwRet) // INFINITE
	{
		dwRet = MsgWaitForMultipleObjects(1, &m_hPlayThread, FALSE, 100, QS_ALLINPUT);
		if (dwRet == WAIT_OBJECT_0 + 1)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	// 停止解压线程
	dwRet = -1;
	while (NULL != m_hH264Thread && WAIT_OBJECT_0 != dwRet) // INFINITE
	{
		dwRet = MsgWaitForMultipleObjects(1, &m_hH264Thread, FALSE, 100, QS_ALLINPUT);
		if (dwRet == WAIT_OBJECT_0 + 1)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

    if (m_hPlayThread != NULL)
    {
        //TerminateThread(m_hPlayThread, 0);
        CloseHandle(m_hPlayThread);
        m_hPlayThread = NULL;
    }

    if (m_hH264Thread != NULL)
    {
        //TerminateThread(m_hH264Thread, 0);
        CloseHandle(m_hH264Thread);
        m_hH264Thread = NULL;
    }

    // 清空队列
    AVPacket* pk = NULL;
    int pk_size = 0;
    while (0 < gvp_queue_size(m_hQueue))
    {
        gvp_dequeue(m_hQueue, (void **)&pk, &pk_size, 1);
        if (NULL != pk)
        {
            av_free_packet(pk);
            av_free(pk);
            pk = NULL;
        }
    }

    // 释放解码过程数据
    if (m_pReceivePacket != NULL)
    {
        //av_free_packet(m_pReceivePacket);
        av_free(m_pReceivePacket);
        m_pReceivePacket = NULL;
    }
    if (m_pDecodePacket != NULL)
    {
        av_free_packet(m_pDecodePacket);
        av_free(m_pDecodePacket);
        m_pDecodePacket = NULL;
    }
    if (m_pFrame != NULL) 
    {
        av_free(m_pFrame);
        m_pFrame = NULL;
    }
    if (NULL != m_pSws)
    {
        sws_freeContext(m_pSws);
        m_pSws = NULL;
    }
    if (NULL != m_pBmp)
    {
        av_free(m_pBmp);
        m_pBmp = NULL;
    }

    // 关闭解码
    if (NULL != m_pContext)
    {
		avcodec_flush_buffers(m_pContext);
        avcodec_close(m_pContext);
        av_free(m_pContext);
        m_pContext = NULL;
    }

    // 关闭解码
    if (NULL != m_pFormatCtx)
    {
        //avcodec_close(m_pFormatCtx->streams[m_iVideoStream]->codec);
        avformat_close_input(&m_pFormatCtx);
        m_pFormatCtx = NULL;
    }

    //if (::IsWindow(m_hWnd))
    //{
    //	DestroyWindow(m_hWnd);
    //}

    if (m_hWnd != NULL)
    {
        //CloseHandle(m_hWnd);
        m_hWnd = NULL;
    }

    m_lFps = 0;
    m_lUserData = 0;
    m_lRecordSecond = 0;
    m_iStartRecordTime = 0;
}

// 视频流接收线程
DWORD WINAPI CGFFX::OnRTSPReceiveThread(VOID* pContext)
{
    CGFFX* pThis = (CGFFX *)pContext;
    BOOL fConnect = FALSE;
    DWORD dwFrames = 0;
    DWORD dwTime = 0;

    pThis->m_fConnect = fConnect;
    while (pThis->m_fRunning)
    {
        // 连接RTSP
        if (!fConnect && !(pThis->m_fConnect = fConnect = SUCCEEDED(pThis->Connect())))
        {
            Sleep(25);
            continue;
        }

        // 接收每一帧视频
        pThis->m_pReceivePacket = (AVPacket *)av_malloc(sizeof(AVPacket));
        int iAVFrameRet = av_read_frame(pThis->m_pFormatCtx, pThis->m_pReceivePacket);
        if (iAVFrameRet != 0)
        {
            char chLog[128] = {0};
            sprintf_s(chLog, "<H264>Read Frame Error:%d\n", iAVFrameRet);
            OutputDebugString(chLog);
        }
        if (AVERROR(ETIMEDOUT) == iAVFrameRet || iAVFrameRet < 0)
        {    
            dwFrames = 0;
            fConnect = FALSE;
            pThis->m_fConnect = fConnect;

            // 等待队列清空
            while (0 < gvp_queue_size(pThis->m_hQueue))
            {
                if (!pThis->m_fRunning)
                {
                    if (NULL != pThis->m_pReceivePacket)
                    {
						av_free_packet(pThis->m_pReceivePacket);
						av_free(pThis->m_pReceivePacket);
						pThis->m_pReceivePacket = NULL;
                    }
                    break; // 接收到退出信号直接退出不等了
                }
                //char chLog[128] = {0};
                //sprintf_s(chLog, "<H264>Frame 清空队列:%d\n", gvp_queue_size(pThis->m_hQueue));
                //OutputDebugString(chLog);
                Sleep(20);
            }

            /*
            // Error Log
            FILE* pFile = NULL;
            pFile = fopen("Frame.txt", "a+");
            if (pFile)
            {
				fprintf(pFile, "Error Frame: %I64d--%I64d--%d--%d--%I64d--%I64d--%d--%d--%d\n", pk->convergence_duration, pk->dts, pk->flags, pk->duration, pk->pos, pk->pts, pk->side_data_elems,
				pk->size, pk->stream_index);
				fclose(pFile);
				pFile = NULL;
            }
            */

            // 等待最后一帧处理完成再进行释放
            while (TRUE == pThis->m_fDecoding) {
                OutputDebugString("<H264>Decode is running!\n");
                Sleep(20);
            }

			if (NULL != pThis->m_pReceivePacket)
			{
				// 用av_malloc申请的内存，结构体内部指针并非为NULL，接收到有误的包不能用这个函数把数据置0
				av_free_packet(pThis->m_pReceivePacket);
				av_free(pThis->m_pReceivePacket);
				pThis->m_pReceivePacket = NULL;
			}

            // 删除连接
            if (NULL != pThis->m_pContext)
            {
                avcodec_flush_buffers(pThis->m_pContext);
                avcodec_close(pThis->m_pContext);
                av_free(pThis->m_pContext);
                pThis->m_pContext = NULL;
            }
            if (NULL != pThis->m_pFormatCtx)
            {
                avformat_close_input(&pThis->m_pFormatCtx);
                pThis->m_pFormatCtx = NULL;
            }
        }
        else if (NULL != pThis->m_pReceivePacket->data)
        {
			if (pThis->m_pReceivePacket->stream_index == pThis->m_iVideoStream)
			{
				// 统计帧率
				if (!dwFrames++)
				{
					dwTime = GetTickCount();
				}
				else if (GetTickCount() - dwTime > 10000)
				{
					pThis->m_lFps = 1000 * dwFrames / (GetTickCount() - dwTime);
					dwFrames = 0;
					//char chLog[128] = {0};
					//sprintf_s(chLog, "<H264>Frame 帧率:%d\n", pThis->m_lFps);
					//OutputDebugString(chLog);
				}
			}

            // 压入队列
            if (100 > gvp_queue_size(pThis->m_hQueue)
				&& pThis->m_pReceivePacket->stream_index == pThis->m_iVideoStream)
            {
                gvp_enqueue(pThis->m_hQueue, pThis->m_pReceivePacket, sizeof(pThis->m_pReceivePacket));
                pThis->m_pReceivePacket = NULL;
            }
        }

        // 释放一个rtsp包
        if (NULL != pThis->m_pReceivePacket)
        {
			av_free_packet(pThis->m_pReceivePacket);
			av_free(pThis->m_pReceivePacket);
			pThis->m_pReceivePacket = NULL;
        }
    }
	OutputDebugString("Exit OnRTSPReceiveThread.\n");
    return 0;
}

// 视频流解码线程
DWORD WINAPI CGFFX::OnH264DecodeThread(VOID* pContext)
{
    CGFFX* pThis = (CGFFX *)pContext;
    AVFrame* pFrameYUV = NULL;
    pThis->m_pFrame = av_frame_alloc();

#ifdef USE_DXVA
    AVFrame* pFrameDxva = avcodec_alloc_frame();
    CHAR* pDxvaBuf =NULL;
    INT width = 0, height = 0;
#endif

    INT got_picture = 0, w = 0, h = 0;
    RECT rc1, rc2;
    memset(&rc1, 0, sizeof(rc1));
    memset(&rc2, 0, sizeof(rc2));

    int iLastWidth = 0;
    int iLastHeight = 0;

    // 使用GDI来绘制
    BITMAPINFOHEADER hdr = 
    {
        sizeof(BITMAPINFOHEADER), 0, 0, 1, 24, BI_RGB, 0, 0, 0, 0, 0
    };

    int pk_size = 0;
    char chLog[256] = {0};
	int iTryTime = 0;
    // 循环解压数据
    while (pThis->m_fRunning)
    {
        // 等待数据
        gvp_dequeue(pThis->m_hQueue, (void **)&pThis->m_pDecodePacket, &pk_size, 1);
        if (NULL == pThis->m_pDecodePacket) {      
            continue;
        }
        pThis->m_fDecoding = TRUE;

        // 解码显示
        if (pThis->m_fConnect
            && NULL != pThis->m_hWnd
            && IsWindow(pThis->m_hWnd)
            && IsWindowVisible(pThis->m_hWnd)
            && NULL != pThis->m_pContext
            && 0 < avcodec_decode_video2(pThis->m_pContext, pThis->m_pFrame, &got_picture, pThis->m_pDecodePacket) 
            && got_picture)
        {
            pFrameYUV = NULL;

#ifdef USE_DXVA
            if (NULL == pDxvaBuf || width  != pThis->m_pFormatCtx->streams[0]->codec->width || height != pThis->m_pFormatCtx->streams[0]->codec->height)
            {	
                if (NULL != pDxvaBuf)
                {
                    av_free(pDxvaBuf);
                    pDxvaBuf = NULL;
                }
                width  = pThis->m_pFormatCtx->streams[0]->codec->width;
                height = pThis->m_pFormatCtx->streams[0]->codec->height;
                pDxvaBuf=(CHAR *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, width, height));				
                avpicture_fill((AVPicture *)pFrameDxva, (uint8_t*)pDxvaBuf, PIX_FMT_YUV420P, width, height);
            }
            if (NULL != pThis->m_pFormatCtx->streams[0]->codec->opaque)
            {
                DxPictureCopy(pThis->m_pFormatCtx->streams[0]->codec, pFrame, pFrameDxva, width, height);
                pFrameYUV = pFrameDxva;
            }
#endif

            if (NULL == pFrameYUV)
            {
                pFrameYUV = pThis->m_pFrame;
            }

            // 预先取值
            int iFrameWidth = pThis->m_pContext->width;
            int iFrameHeight= pThis->m_pContext->height;
            AVPixelFormat frameFormat = pThis->m_pContext->pix_fmt;

#ifdef USE_CAPTURE
			if (pThis->m_pDestImg == NULL)
			{
				pThis->m_pDestImg = new BYTE[MAX_IMG_SIZE];
			}
			if (pThis->m_pSrcImg == NULL)
			{
				pThis->m_pSrcImg = new BYTE[MAX_IMG_SIZE];
			}
			if (iTryTime++ > 50)
			{
				SwsContext* pTempSws= NULL;
				// 缩放	
				pTempSws = sws_getContext(iFrameWidth, iFrameHeight, 
					frameFormat, iFrameWidth, iFrameHeight, PIX_FMT_BGR24, SWS_BILINEAR, NULL, NULL, NULL);	

				if (pThis->m_pSrcImg )
				{
					memset(pThis->m_pSrcImg, 0, MAX_IMG_SIZE);
				}
				if (pTempSws)
				{
					BYTE* data[4] = {pThis->m_pSrcImg, 0, 0, 0};
					INT  pitch[4] = {3 * iFrameWidth, 0, 0, 0};
					sws_scale(pTempSws, pFrameYUV->data, pFrameYUV->linesize, 0, iFrameHeight, data, pitch);

					if (pThis->m_pDestImg)
					{
						memset(pThis->m_pDestImg, 0, MAX_IMG_SIZE);
					}
					int iBufferLength = MAX_IMG_SIZE;
					simplest_bgr24_to_bmp(pThis->m_pSrcImg, iFrameWidth*iFrameHeight*3, iFrameWidth, iFrameHeight, pThis->m_pDestImg, &iBufferLength);
					if (iBufferLength > 0)
					{
						IMG_BMP tempBmpStruct;
						tempBmpStruct.width = iFrameWidth;
						tempBmpStruct.height = iFrameHeight;
						tempBmpStruct.ImgLength = iBufferLength;
						tempBmpStruct.pData = pThis->m_pDestImg;

						pThis->m_lBmplist.AddOneIMG(&tempBmpStruct);
						tempBmpStruct.pData = NULL;

						//char chFileName[256] = {0};
						//sprintf(chFileName, "./%ld.bmp", GetTickCount());
						//FILE* pFile = fopen(chFileName, "w");
						//if (pFile)
						//{
						//	fwrite(pThis->m_pDestImg, iBufferLength, 1, pFile);
						//	fclose(pFile);
						//	pFile = NULL;
						//}
					}
				}
				iTryTime = 0;

				if (NULL != pTempSws) 
				{
					sws_freeContext(pTempSws);
					pTempSws = NULL;
				}
			}
#endif
            // 生成缩放因子
			if (NULL != pThis->m_hWnd  
				&&IsWindow(pThis->m_hWnd)
				&&IsWindowVisible(pThis->m_hWnd)
				)
			{
				GetClientRect(pThis->m_hWnd, &rc2);
				if (NULL == pThis->m_pSws 
					|| (rc1.right - rc1.left) / 4 * 4 != (rc2.right - rc2.left) / 4 * 4 
					|| (rc1.bottom - rc1.top) != (rc2.bottom - rc2.top)
					|| iLastWidth != iFrameWidth
					|| iLastHeight != iFrameHeight)
				{
					rc1 = rc2;
					w = (rc1.right - rc1.left) / 4 * 4 ;
					h = rc1.bottom - rc1.top;
					hdr.biWidth = w;
					hdr.biHeight = -h;
					hdr.biSizeImage  = 3 * w * h;
					iLastWidth = iFrameWidth;
					iLastHeight = iFrameHeight;

					sprintf_s(chLog, "<H264>Frame Info:%d %d %d %d width:%d height:%d FrameWidth:%d FrameHeight:%d \n", 
						rc2.left, rc2.right, rc2.top, rc2.bottom, w, h, iFrameWidth, iFrameHeight);
					OutputDebugString(chLog);

					// 创建位图
					if (NULL != pThis->m_pBmp)
					{
						av_free(pThis->m_pBmp);
						pThis->m_pBmp = NULL;
					}
					pThis->m_pBmp = (BYTE *)av_malloc(hdr.biSizeImage);

					// 缩放
					if (NULL != pThis->m_pSws) {
						sws_freeContext(pThis->m_pSws);
						pThis->m_pSws = NULL;
					}
					pThis->m_pSws = sws_getContext(iFrameWidth, iFrameHeight, 
						frameFormat, w, h, PIX_FMT_BGR24, SWS_BILINEAR, NULL, NULL, NULL);
				}
				// 刷新显示
				if (NULL != pThis->m_pSws)
				{	
					// 缩放
					BYTE* data[4] = {pThis->m_pBmp, 0, 0, 0};
					INT  pitch[4] = {3 * w, 0, 0, 0};
					sws_scale(pThis->m_pSws, pFrameYUV->data, pFrameYUV->linesize, 0, iFrameHeight, data, pitch);

					// 显示
					if (IsWindow(pThis->m_hWnd))
					{
						HDC hDC = ::GetDC(pThis->m_hWnd);
						SetDIBitsToDevice(hDC, 0, 0, w, h, 0, 0, 0, h, pThis->m_pBmp, (BITMAPINFO *)&hdr, DIB_RGB_COLORS);
						::ReleaseDC(pThis->m_hWnd, hDC);
					}
				}
			}

        }

        pThis->m_fDecoding = FALSE;
        // 释放资源
        if (pThis->m_pDecodePacket != NULL)
        {
            av_free_packet(pThis->m_pDecodePacket);
            av_free(pThis->m_pDecodePacket);
            pThis->m_pDecodePacket = NULL;
        }
    }

    if (NULL != pThis->m_pFrame)
    {
        av_free(pThis->m_pFrame);
        pThis->m_pFrame = NULL;
    }

#ifdef USE_DXVA
    if (NULL != pDxvaBuf)
    {
        av_free(pDxvaBuf);
        pDxvaBuf = NULL;
    }
    if (NULL != pFrameYUV)
    {
        av_free(pFrameYUV);
        pFrameYUV = NULL;
    }
#endif

    if (NULL != pThis->m_pSws)
    {
        sws_freeContext(pThis->m_pSws);
        pThis->m_pSws = NULL;
    }
    if (NULL != pThis->m_pBmp)
    {
        av_free(pThis->m_pBmp);
        pThis->m_pBmp = NULL;
    }

    return 0;
}

// 连接RTSP视频
HRESULT CGFFX::Connect(VOID)
{
    if (!m_fRunning || !_stricmp(m_szURL, "rtsp://0.0.0.0"))
    {
        return E_FAIL;
    }

    AVDictionary* opts = NULL;
    // TCP模式接收
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    // 设置缓冲区大小
    av_dict_set(&opts, "bufsize", "655350", 0);
    // 设置超时时间间隔x秒[单位为微秒]
	av_dict_set(&opts, "stimeout", "500000", 0);

	////设置缓存大小，1080p可将值调大
	//av_dict_set(&opts, "buffer_size", "1024000", 0);
	////以udp方式打开，如果以tcp方式打开将udp替换为tcp
	//av_dict_set(&opts, "rtsp_transport", "tcp", 0);
	//// 设置超时时间间隔x秒[单位为u秒] 如果没有设置stimeout，那么把ipc网线拔掉，av_read_frame会阻塞（时间单位是u妙) 设置超时3秒
	//av_dict_set(&opts, "stimeout", "3000000", 0);
	//// 设置最大时延  缓冲长，应该是在计算收到视频的帧率。你发udp包，没有帧率信息，ffplay可能会缓很多帧
	//av_dict_set(&opts, "max_delay", "5000", 0);

	//av_dict_set(&opts, "preset", "superfast", 0);
	//av_dict_set(&opts, "tune", "zerolatency", 0);

    //av_log_set_level(AV_LOG_DEBUG);

    // 打开文件
    INT iRet = avformat_open_input(&m_pFormatCtx, m_szURL, NULL, &opts);
    if (iRet || !m_fRunning)
    {	
        avformat_close_input(&m_pFormatCtx);
        av_dict_free(&opts);
        m_pFormatCtx = NULL;
        return E_FAIL;
    }

    // 缩短提取流信息的时间
    m_pFormatCtx->probesize = 100 * 1024;
    m_pFormatCtx->max_analyze_duration = 5 * AV_TIME_BASE;

    // 从文件提取流信息
    if (!m_fRunning || 0 > avformat_find_stream_info(m_pFormatCtx, &opts) || !m_pFormatCtx->nb_streams)
    {
        avformat_close_input(&m_pFormatCtx);
        av_dict_free(&opts);
        m_pFormatCtx = NULL;
        return E_FAIL;
    }
    av_dict_free(&opts);

    // 查找视频原始流的下标
    m_iVideoStream = -1;
    for (unsigned int i = 0; i < m_pFormatCtx->nb_streams; i++)
    {
        if (AVMEDIA_TYPE_VIDEO == m_pFormatCtx->streams[i]->codec->codec_type)
        {
            m_iVideoStream = i;
            break;
        }
    }
    if (!m_fRunning || -1 == m_iVideoStream)
    {
        avformat_close_input(&m_pFormatCtx);
        m_pFormatCtx = NULL;
        return E_FAIL;
    }
    // 取对应视频流编解码器信息
    AVCodecContext* enc = m_pFormatCtx->streams[m_iVideoStream]->codec;	

#ifdef USE_DXVA
    enc->get_buffer = DxGetFrameBuf;
    enc->reget_buffer = DxReGetFrameBuf;
    enc->release_buffer = DxReleaseFrameBuf;
    enc->opaque = NULL;
    // 是否为需要硬解
    if (enc->codec_id == CODEC_ID_MPEG1VIDEO 
        || enc->codec_id == CODEC_ID_MPEG2VIDEO
        || enc->codec_id == CODEC_ID_H264 
        || enc->codec_id == CODEC_ID_VC1 
        || enc->codec_id == CODEC_ID_WMV3)
    {
        enc->get_format = DxGetFormat;
    }
#endif

    // 查找视频流的对应解码器
    AVCodec* pCodec = avcodec_find_decoder(enc->codec_id);
    // 不直接使用从AVFormatContext得到的CodecContext，要复制一个
    m_pContext = avcodec_alloc_context3(pCodec);
    if (!m_fRunning || avcodec_copy_context(m_pContext, m_pFormatCtx->streams[m_iVideoStream]->codec) != 0)
    {
        avformat_close_input(&m_pFormatCtx);
        m_pFormatCtx = NULL;
        avcodec_close(m_pContext);
        av_free(m_pContext);
        m_pContext = NULL;
        return E_FAIL;
    }

    // 打开解码器
    if (!m_fRunning || !pCodec || 0 > avcodec_open2(m_pContext, pCodec, NULL))
    {
        avformat_close_input(&m_pFormatCtx);
        m_pFormatCtx = NULL;
        avcodec_close(m_pContext);
        av_free(m_pContext);
        m_pContext = NULL;
        return E_FAIL;
    }
    return S_OK;
}


bool CGFFX::GetOneBmpImg(PBYTE DestImgData, int& iLength, int& iWidth, int& iHeight)
{
	if (!DestImgData)
	{
		return false;
	}
#ifdef USE_CAPTURE
	IMG_BMP* pTempImg = NULL;
	pTempImg = m_lBmplist.getOneIMG();
	if (!pTempImg)
	{
		return false;
	}
	if (iLength < pTempImg->ImgLength)
	{
		iLength = pTempImg->ImgLength;
		return false;
	}

	iLength = pTempImg->ImgLength;
	iWidth = pTempImg->width;
	iHeight = pTempImg->height;
	memcpy(DestImgData, pTempImg->pData, pTempImg->ImgLength);

	SAFE_DELETE_OBJ(pTempImg);
	return true;
#else
	return false;
#endif
}
