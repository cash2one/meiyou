/***********************************************************
 Copyright (C), MeetYou
 FileName: CalculatePicHash.cpp
 Author: Ronghong Huang
 Date:   16/2/17
 ***********************************************************/

#include <stdio.h>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include<unistd.h>

using namespace std;
using namespace cv;

#define NWIDTH 8
#define NHEIGHT 8

bool calculatePicHash(string strPicPath, int arrHash[NWIDTH * NHEIGHT])
{
    if (NULL == arrHash)
    {
        return false;
    }
    
    Mat matPic, matCompressed;
    
    matPic = imread(strPicPath);
    CvCapture *capture = NULL;
    if (NULL == matPic.data)
    {
        capture = cvCreateFileCapture(strPicPath.c_str());
        if (NULL == capture)
        {
            return false;
        }
        
        matPic = cvQueryFrame(capture);
        if (NULL == matPic.data)
        {
            return false;
        }
    }
    
    cv::resize(matPic, matCompressed, cv::Size(NWIDTH, NHEIGHT), 0, 0, cv::INTER_LINEAR);
    cv::cvtColor(matCompressed, matCompressed, CV_RGBA2GRAY);
    
    float fAvrSrc = mean(matCompressed)[0];
    matCompressed = (matCompressed >= fAvrSrc) / 255;
    
    for (int i = 0; i < NHEIGHT; i++)
    {
        for (int j = 0; j < NWIDTH; j++)
        {
            arrHash[i*NWIDTH + j] = matCompressed.at<uchar>(i, j);
        }
    }
    
    return true;
}

string getPicHashString(string strPicPath)
{
    string strPicHash;
    char szPicHash[NWIDTH * NHEIGHT / 4] = {0};
    int arrSrcHash[NWIDTH * NHEIGHT] = {0};
    
    if (!calculatePicHash(strPicPath, arrSrcHash))
    {
        return strPicHash;
    }
    
    assert((NWIDTH * NHEIGHT) % 4 == 0);
    int nTemp = 0;
    char cTemp = 0;
    for (int i = 0; i < (NWIDTH * NHEIGHT) / 4; i++)
    {
        nTemp = arrSrcHash[i*4 + 3]
        + arrSrcHash[i*4 + 2] * 2
        + arrSrcHash[i*4 + 1] * 4
        + arrSrcHash[i*4] * 8;
        sprintf(szPicHash + i, "%0x", nTemp);
    }
    
    strPicHash = szPicHash;
    
    return strPicHash;
}

int main(int argc, char* argv[])
{
    
    string strSrcPath = "";
    int param;
    
    while( (param = getopt(argc, argv, "Hs:")) != -1 )
    {
        if ( param == 's' ){
            strSrcPath = optarg;
        }else if ( param == 'H')
        {
            cout << "Usage: exclip [options] [-s] <source_file> [--] [args...]" << endl;
            cout << "-s<path>	the path of source file" << endl;
            return 0;
        }
    }
    
    if ( strSrcPath == "" )
    {
        cerr << "you should specify the path of source file.[use -H for help]" << endl;
        return 1;
    }
    
    string strPicHash = getPicHashString(strSrcPath);
    if (strPicHash.length() == 0)
    {
        cout << "can't get hash code. please check if the picture exists" << endl;
    }
    else
    {
        cout << strPicHash << endl;
    }
    
    return 0;
    
}
