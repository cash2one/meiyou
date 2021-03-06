
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include <string>
#include <string.h>
#include <vector>

#include "FreeImage.h"
#include "stdtostring.h"

#include <time.h>

// Calc the gap of reduce frame
int calcGap(int numFrames)
{
	int gap = 1;

	if (numFrames <= 20)
	{
		gap = 1;
	}
	else if (numFrames > 20 && numFrames <= 80)
	{
		gap = 2;
	}
	else if (numFrames > 80)
	{
		gap = 3;
	}

	return gap;
}

bool CompressGifGlobal(FIMULTIBITMAP *gif, int nFrames, int gap, const std::string &filePath, const std::string& gifsiclePath)
{
	//Step1: Calc delay of each frame
	LONG totalDelay = 0;
	int count = 0;
	std::vector<LONG> delayVec;
	delayVec.reserve(nFrames);

	FIBITMAP *pFrame = NULL;
	for (int i = 0; i < nFrames; i++)
	{
		pFrame = FreeImage_LockPage(gif, i);
        //std::cout << "i = " << i << std::endl;
		//"FrameTime" : 以毫秒计的每帧播放时间，所以在操作的时候是用ms作为单位
		//而在最终保存文件的时候则是保存成百分之一秒，比如40ms保存成4cs，48ms也
		//保存成4cs。而ImageMagick不管操作还是保存，都是用百分之一秒即cs来操作！
		FITAG *tagDelay = NULL;
		FreeImage_GetMetadata(FIMD_ANIMATION, pFrame, "FrameTime", &tagDelay);

		if (tagDelay != NULL)
		{
			LONG *pcurDelay = (LONG *)FreeImage_GetTagValue(tagDelay);
			int curDelay = *pcurDelay;

			if (curDelay != 0)
			{
			    //std::cout << "curDelay = " << curDelay << std::endl;
				//FreeImage use ms, Gifsicle use cs, need to convert
				int csDelay = (int)(curDelay / 10);
				delayVec.push_back(csDelay);

                //如果当前帧delay大于500ms，那么给totalDelay加上固定的80ms，而不是当前帧的delay
				if (csDelay >= 50)
				{
					totalDelay += 8;
					count++;
				}
				else
                {
                    totalDelay += csDelay;
                }

			}
			else
			{
				delayVec.push_back(8);
				totalDelay += 8;
			}
		}

		FreeImage_UnlockPage(gif, pFrame, FALSE);
	}

	//Step2: Reduce frame
	if (count < (int)(nFrames * 0.5))
	{
		int newNumFrames = (int)(nFrames / gap);

		//normalize to 200ms if bigger than 200ms
		if ((int)(totalDelay / newNumFrames) > 20)
		{
			double scale = (newNumFrames * 1.0 * 20) / totalDelay;
			for (int i = 0; i < (int)delayVec.size(); i++)
			{
			    //大于500ms的帧delay得保留，不能scale
			    if (delayVec[i] < 50)
                {
                    delayVec[i] = (int)(delayVec[i] * scale);
                }
			}
		}

		std::vector<std::string> paramVec;
		paramVec.push_back(std::string("gifsicle"));
		paramVec.push_back(std::string("-b"));
        paramVec.push_back(std::string("--unoptimize"));
        paramVec.push_back(filePath);

		//reduce frame
		for (int i = 0; i < nFrames; i++)
		{
		    //如果当前帧的delay大于500ms，这一帧直接保留
		    if (delayVec[i] >= 50)
            {
                //std::cout << "The delay of this frame is greater than 500ms, reserve this frame!" << std::endl;
                paramVec.push_back(std::string("-d" + std::to_string(delayVec[i])));
				paramVec.push_back(std::string("#" + std::to_string(i)));
            }
            else
            {
                if (i % gap == 0)
                {
                    int tempDelay = 0;
                    for (int k = 0; k < gap; k++)
                    {
                        int curIdx = i + k;
                        if (curIdx >= nFrames)
                            break;
                        //如果当前第i帧后面的某一帧delay大于500ms，那么不给当前帧加那大于500ms的时间
                        //而是直接加上一个80ms，因为大于500ms的那一帧在后面的处理中肯定是会保留的
                        if (delayVec[curIdx] >= 50)
                        {
                            tempDelay += 8;
                        }
                        else
                        {
                            tempDelay += delayVec[curIdx];
                        }

                    }

                    paramVec.push_back(std::string("-d" + std::to_string(tempDelay)));
                    paramVec.push_back(std::string("#" + std::to_string(i)));
                }
            }

		}

		paramVec.push_back(std::string("--optimize=2"));

		//convert string vector to char** array
		int numParams = (int)paramVec.size();
        char **paramsArr = new char*[numParams + 1];
        for (int i = 0; i < numParams; i++)
        {
            int curSize = paramVec[i].length();
            paramsArr[i] = new char[curSize + 1];
            strcpy(paramsArr[i], paramVec[i].c_str());
        }
        paramsArr[numParams] = NULL;

        //const char *shellPath = "./gifsicle";

        //创建子进程来调用execv函数，并等待执行完成
        int status;
        pid_t fpid = fork();
        if (-1 == fpid) //创建子进程出错
        {
            std::cerr << "Failed to create a child process-result!" << std::endl;
            return false;
        }
        else if (0 == fpid) //执行子进程
        {
            if (execv(gifsiclePath.c_str(), paramsArr) < 0) //execv只有出错才有返回值
            {
                std::cerr << "Failed to execute gifsicle-result!" << std::endl;
				std::cerr << "ERROR: value = " << errno << ", it means : " << strerror(errno) << std::endl;
                //如果这个函数调用失败，我们最终目的应该是退出整个函数，返回false，代表未压缩成功
                //因此这里如果执行失败，应该以一个错误状态退出，反馈给父进程，然后父进程捕捉到之后
                //返回一个false，告诉整个函数结束了，没有压缩成功！

                _exit(5);
            }

        }
        else //父进程应先阻塞，等待子进程执行完毕，再执行父进程
        {
            wait(&status);

            //清理分配的动态内存
            for (int i = 0; i < numParams + 1; i++)
            {
                delete []paramsArr[i];
            }
            delete []paramsArr;
            paramsArr = NULL;

            //判断子进程的退出状态
            if (0 != WIFEXITED(status)) //子进程正常退出
            {
                //如果execv调用失败，子进程以5这个值返回，所以判断返回值是否为5
                if (5 == WEXITSTATUS(status))
                {

                    return false;
                }

            }
            else //WIFEXITED(status)返回0，子进程异常退出
            {
                std::cerr << "Child process-result exit abnormally!" << std::endl;
                return false;
            }

        }

	}
	else
	{
		return false;
	}

	return true;
}

bool compressGif(const std::string& filePath, const std::string& gifsiclePath)
{
	try
	{
		//Step1: read gif
		FIMULTIBITMAP *gif = FreeImage_OpenMultiBitmap(FIF_GIF, filePath.c_str(), FALSE, FALSE, FALSE, GIF_DEFAULT);
		int nFrames = FreeImage_GetPageCount(gif);

		if (0 == nFrames)
		{
			std::cout << "Empty gif file!" << std::endl;
			return false;
		}

		//Step2: calc gap
		// If gap > 1, go on
		int gap = calcGap(nFrames);

		if (gap > 1)
		{

			//Step3: check if gif have local palette
			FIBITMAP *palFrame1 = FreeImage_LockPage(gif, 1);
			FITAG *tagPalette1 = NULL;
			FreeImage_GetMetadata(FIMD_ANIMATION, palFrame1, "NoLocalPalette", &tagPalette1);

			int palette1 = 1; //default = no local palette
			if (tagPalette1 != NULL)
			{
				BYTE *pPalette1 = (BYTE *)FreeImage_GetTagValue(tagPalette1);
				palette1 = *pPalette1;
			}
			else
			{
				FreeImage_CloseMultiBitmap(gif, 0);
				return false;
			}
			FreeImage_UnlockPage(gif, palFrame1, FALSE);

			//Step4: If the gif use local palette，delete it
			if (palette1 == 0)
			{
				FIBITMAP *palFrame0 = FreeImage_LockPage(gif, 0);
				int nColors = FreeImage_GetColorsUsed(palFrame0);
                FreeImage_UnlockPage(gif, palFrame0, FALSE);
                //第一次调用gifsicle删除local palette之前先关闭打开的gif文件
                FreeImage_CloseMultiBitmap(gif, 0);

                //2017.05.19修改记录：有些gif的local色盘如果都是256，不知道为什么colors设置成
                //256之后会造成第一帧的delay变得非常大。而且256和128的显示效果差不多，所以设置成128
                //原因：进行第一次调用gifsicle之前没有关闭打开的gif！

                //remove local palette
                std::vector<std::string> paramVec;
                paramVec.push_back(std::string("gifsicle"));
                paramVec.push_back(std::string("-b"));
                paramVec.push_back(std::string("--colors=") + std::to_string(nColors));
                paramVec.push_back(filePath);

                //convert string vector to char** array
                int numParams = (int)paramVec.size();
                char **paramsArr = new char*[numParams + 1];
                for (int i = 0; i < numParams; i++)
                {
                    int curSize = paramVec[i].length();
                    paramsArr[i] = new char[curSize + 1];
                    strcpy(paramsArr[i], paramVec[i].c_str());
                }
                paramsArr[numParams] = NULL;

                //const char *shellPath = "./gifsicle";

                 //创建子进程来调用execv函数，并等待执行完成
                int status;
                pid_t fpid = fork();
                if (-1 == fpid) //创建子进程出错
                {
                    std::cerr << "Failed to create a child process-color!" << std::endl;
                    return false;
                }
                else if (0 == fpid) //执行子进程
                {
                    if (execv(gifsiclePath.c_str(), paramsArr) < 0) //execv只有出错才有返回值
                    {
                        std::cerr << "Failed to execute gifsicle-color!" << std::endl;
						std::cerr << "ERROR: value = " << errno << ", it means : " << strerror(errno) << std::endl;
                        //如果这个函数调用失败，我们最终目的应该是退出整个函数，返回false，代表未压缩成功
                        //因此这里如果执行失败，应该以一个错误状态退出，反馈给父进程，然后父进程捕捉到之后
                        //返回一个false，告诉整个函数结束了，没有压缩成功！

                        _exit(5);
                    }

                }
                else //父进程应阻塞，等待子进程执行完毕，再执行父进程
                {
                    wait(&status);

                    //清理分配的动态内存
                    for (int i = 0; i < numParams + 1; i++)
                    {
                        delete []paramsArr[i];
                    }
                    delete []paramsArr;
                    paramsArr = NULL;

                    //判断子进程的退出状态
                    if (0 != WIFEXITED(status)) //子进程正常退出
                    {
                        //如果execv调用失败，子进程以5这个值返回，所以判断返回值是否为5
                        if (5 == WEXITSTATUS(status))
                        {
                            return false;
                        }
                    }
                    else //WIFEXITED(status)返回0，子进程异常退出
                    {
                        std::cerr << "Child process-color exit abnormally!" << std::endl;
                        return false;
                    }

                    //子进程正常退出，且返回值status不是5，则执行下面的代码
                    //重新打开gif进行抽帧
                    FIMULTIBITMAP *gif2 = FreeImage_OpenMultiBitmap(FIF_GIF, filePath.c_str(), FALSE, FALSE, FALSE, GIF_DEFAULT);
                    int nFrames2 = FreeImage_GetPageCount(gif2);

                    if (0 == nFrames2)
                    {
                        std::cout << "Empty gif2 file!" << std::endl;
                        return false;
                    }

                    bool res = CompressGifGlobal(gif2, nFrames2, gap, filePath, gifsiclePath);

                    FreeImage_CloseMultiBitmap(gif2, 0);

                    if (res == false)
                    {
                        return false;
                    }


                }

			} //if (palette1 == 0)
			else if (palette1 == 1)
			{
				bool res = CompressGifGlobal(gif, nFrames, gap, filePath, gifsiclePath);

				FreeImage_CloseMultiBitmap(gif, 0);

				if (res == false)
				{
					return false;
				}
			}
		}
		else
		{
			FreeImage_CloseMultiBitmap(gif, 0);
			return false;
		}

		return true;
	}
	catch (const std::exception &error_)
	{
		std::cout << "Caught exception: " << error_.what() << std::endl;
		return false;
	}
}

int main(int argc, char *argv[])
{
	//Step1: Check params
	std::string strSrcPath = "";
	std::string gifsiclePath = "";

	int nParam;
	while ((nParam = getopt(argc, argv, "Hs:g:")) != -1)
	{
		if (nParam == 's')
		{
			strSrcPath = optarg;
		}
		else if (nParam == 'g')
        {
            gifsiclePath = optarg;
        }
		else if (nParam == 'H')
		{
			std::cout << "Usage: exclip [options] [-s] <source_file> [-g] <gifsiclePath> [--] [args...]" << std::endl;
			std::cout << "-s<path>  the path of source gif file" << std::endl;
			std::cout << "-g<path>  the path of gifsicle program" << std::endl;
			return 0;
		}
	}

	if (strSrcPath == "")
	{
		std::cerr << "you should specify the path of source file.[use -H for help]" << std::endl;
		return 1;
	}

	if (gifsiclePath == "")
    {
        std::cerr << "you should specify the path of gifsicle program.[use -H for help]" << std::endl;
		return 1;
    }

	//Step2: Compress gif
	if (compressGif(strSrcPath, gifsiclePath))
	{
		std::cout << "Compressed!" << std::endl;
	}
	else
	{
		std::cout << "NOT Compressed!" << std::endl;
	}

	return 0;
}

