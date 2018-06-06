#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "inttypes.h"
#include "stdint.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
}
#endif

#include "MyImgList.h"

class CGFFX
{
public:
	CGFFX();
	virtual ~CGFFX(void);
	static HRESULT Initialize(void);
	HRESULT Play(HWND hWnd, LPCSTR szURL);
	VOID Stop(VOID);
    VOID SetExitStatus();
	bool GetOneBmpImg(PBYTE DestImgData, int& iLength, int& iWidth, int& iHeight);

	CRITICAL_SECTION m_csDecode;
protected:
	HRESULT Connect(VOID);

protected:
	static DWORD WINAPI OnRTSPReceiveThread(VOID* pContext);
	static DWORD WINAPI OnH264DecodeThread(VOID* pContext);

private:
	static BOOL m_fInitialize;
	HANDLE m_hPlayThread;
	HANDLE m_hH264Thread;
	INT m_iVideoStream;
	BOOL m_fRunning;
	BOOL m_fConnect;
    BOOL m_fDecoding;
	CHAR m_szURL[MAX_PATH];
	AVFormatContext* m_pFormatCtx;
	AVCodecContext* m_pContext;
    
    // 解码过程数据
    AVPacket* m_pReceivePacket;
    AVPacket* m_pDecodePacket;
    AVFrame* m_pFrame; 
    SwsContext* m_pSws;
    BYTE* m_pBmp;

	void* m_hQueue;
	HWND m_hWnd;

	long m_lFps;
	long m_lUserData;
	long m_lRecordSecond;
	long m_iStartRecordTime;


	PBYTE m_pSrcImg;
	PBYTE m_pDestImg;
	MyImgList m_lBmplist;
};

