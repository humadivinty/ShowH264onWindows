#ifndef H264_H
#define H264_H

#ifdef H264_EXPORTS
#define H264API extern "C" __declspec(dllexport)
#else
#define H264API extern "C" __declspec(dllimport)
#endif

// ��rtsp��Ƶ��rtsp://xxx.xxx.xxx.xxx/h264ESVideoTest��
H264API HANDLE WINAPI H264_Play(HWND hWnd, LPCSTR szURL);

// �ر�rtsp��Ƶ
H264API VOID WINAPI H264_Destroy(HANDLE hPlay);

// �����˳��������ʱ�ɼ��ٵȴ�
H264API VOID WINAPI H264_SetExitStatus(HANDLE hPlay);

#endif // H264_H
