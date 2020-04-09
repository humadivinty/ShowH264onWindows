#include "MyImgList.h"
#include<stdio.h>
#include<string.h>


MyImgList::MyImgList() 
:m_iCurIndex(0),
m_iMaxSize(MAX_ARRAY_SIZE)
{
    InitArray();
}


MyImgList::~MyImgList()
{
    ClearArray();
}

void MyImgList::AddOneIMG(IMG_BMP* img)
{
	std::lock_guard<std::mutex> guard(m_mtx);

    IMG_BMP*pImg = m_ImgArray2[m_iCurIndex++];
    memcpy(pImg->pData, img->pData, img->ImgLength);
    pImg->ImgLength = img->ImgLength;
    pImg->width = img->width;
    pImg->height = img->height;
    pImg->type = img->type;
    
    m_iCurIndex = (m_iCurIndex >= m_iMaxSize) ? 0 : m_iCurIndex;


}

IMG_BMP* MyImgList::getOneIMG()
{
    IMG_BMP*pImg = NULL;
	std::lock_guard<std::mutex> guard(m_mtx);

    if (m_iCurIndex == 0 && (m_ImgArray2[m_iMaxSize-1]->ImgLength == 0) )
    {
        pImg = NULL;
    }
    else
    {
        int iIndex = (m_iCurIndex == 0) ? (m_iMaxSize - 1) : (m_iCurIndex - 1);
        IMG_BMP*pSrcImg = m_ImgArray2[iIndex];
        pImg = new IMG_BMP();
        if (pImg)
        {
            pImg->pData = new unsigned char[pSrcImg->ImgLength];
            memcpy(pImg->pData, pSrcImg->pData, pSrcImg->ImgLength);
            pImg->ImgLength = pSrcImg->ImgLength;
            pImg->width = pSrcImg->width;
            pImg->height = pSrcImg->height;
            pImg->type = pSrcImg->type;
        }
        pSrcImg = NULL;
    }

    return pImg;
}

void MyImgList::InitArray()
{
	std::lock_guard<std::mutex> guard(m_mtx);

    IMG_BMP* pTemp = NULL;
    for (int i = 0; i < MAX_ARRAY_SIZE;i++)
    {
        pTemp = new IMG_BMP();
        if (pTemp)
        {
            pTemp->pData = new unsigned char[MAX_IMG_BUFFER_SIZE];
			if (pTemp->pData)
			{
				memset(pTemp->pData, 0, MAX_IMG_BUFFER_SIZE);
				pTemp->iBufferLenth = MAX_IMG_BUFFER_SIZE;				
			}
			m_ImgArray2[i] = pTemp;
        }
        pTemp = NULL;
    }
}

void MyImgList::ClearArray()
{
	std::lock_guard<std::mutex> guard(m_mtx);

    IMG_BMP* pTemp = NULL;
    for (int i = 0; i < MAX_ARRAY_SIZE; i++)
    {
        pTemp = m_ImgArray2[i];
        
        if (pTemp)
        {
            delete pTemp;
            pTemp = NULL;
        }
        //m_ImgArray.at(i) = NULL;
        m_ImgArray2[i] = NULL;
    }
}
