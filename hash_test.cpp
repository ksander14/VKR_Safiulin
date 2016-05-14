// hash_testing.cpp: определяет точку входа для консольного приложения.
//


#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <ctime>
#include "opencv2\core\core.hpp"
#include "opencv2\features2d\features2d.hpp"
#include "opencv2\highgui\highgui.hpp"
#include "opencv2\nonfree\nonfree.hpp"
#include "opencv2\opencv.hpp"


using namespace std;
using namespace cv;

const int K = 64;
const int approx_time = 39;

// frame numbers for test1
const int frame1_1 = 10;
const int frame1_2 = 1100;

// frame numbers for test2
const int frame2_1 = 50;
const int frame2_2 = 320;
const int frame2_3 = 550;

// frame numbers for test3
const int frame3_1 = 100;
const int frame3_2 = 356;

// frame numbers fot test4

const int frame4_1 = 10;
const int frame4_2 = 180;
const int frame4_3 = 375;

void calcImageHash1(Mat src, bool* hash)
{
	Mat *res = new Mat(32, 32, CV_8UC3, Scalar(0, 0, 0));
	Mat *gray = new Mat(res->size(), CV_8U);
	Mat *binary = new Mat(gray->size(), gray->type());

	// уменьшаем картинку
	resize(src, *res, res->size());

	// переводим в градации серого
	cvtColor(*res, *gray, CV_BGR2GRAY);

	// вычисляем среднее
	Scalar average = mean(*gray);

	// получим бинарное изображение относительно среднего
	// для этого воспользуемся пороговым преобразованием
	threshold(*gray, *binary, average[0], 255, CV_THRESH_BINARY);

	int i = 0;
	// пробегаемся по всем пикселям изображения
	for (int y = 0; y < binary->rows; y++) {
		for (int x = 0; x < binary->cols; x++) {
			if ((int)binary->at<unsigned char>(y, x)) {
				hash[x + y * 32] = true;
			}
			i++;
		}
	}
	delete res, gray, binary;
}


int calcHammingDistance1(bool* x, bool* y)
{
	int result = 0;
	for (int i = 0; i < 1024; i++)
	{
		if (x[i] != y[i]) 
			result++;
	}
	return result;
}

// рассчитать хеш картинки
__int64 calcImageHash(Mat src)
{
	Mat *res = new Mat(8, 8, CV_8UC3, Scalar(0, 0, 0));
	Mat *gray = new Mat(res->size(), CV_8U);
	Mat *binary = new Mat(gray->size(), gray->type());

	// уменьшаем картинку
	resize(src, *res, res->size());

	// переводим в градации серого
	cvtColor(*res, *gray, CV_BGR2GRAY);

	// вычисляем среднее
	Scalar average = mean(*gray);

	// получим бинарное изображение относительно среднего
	// для этого воспользуемся пороговым преобразованием
	threshold(*gray, *binary, average[0], 255, CV_THRESH_BINARY);

	// построим хэш
	__int64 hash = 0;

	int i = 0;
	// пробегаемся по всем пикселям изображения
	for (int y = 0; y < binary->rows; y++) {
		for (int x = 0; x < binary->cols; x++) {
			if ((int)binary->at<unsigned char>(y, x)) {
				hash |= 1i64 << i;
			}
			i++;
		}
	}
	delete res, gray, binary;
	return hash;
}



// рассчёт расстояния Хэмминга между двумя хэшами
__int64 calcHammingDistance(__int64 x, __int64 y)
{
	__int64 dist = 0, val = x ^ y;

	// Count the number of set bits
	while (val)
	{
		++dist;
		val &= val - 1;
	}

	return dist;
}

int main()
{
	// read videos and count frames
	VideoCapture cap_left("test4_1.mp4");
	VideoCapture cap_right("test4_2.mp4");

	double framesCountD1 = cap_right.get(CV_CAP_PROP_FRAME_COUNT);
	double framesCountD2 = cap_left.get(CV_CAP_PROP_FRAME_COUNT);
	int frames_count1 = (int)framesCountD1 - 10;
	int frames_count2 = (int)framesCountD2;


	// choose random frame from capture2 video
	int randomFrameNumber = frame4_3;
	Mat random_frame, random_frame1;
	cap_left.set(CV_CAP_PROP_POS_FRAMES, randomFrameNumber);
	cap_left >> random_frame1;


	// matrix declaration
	__int64* video_indexes = new __int64[frames_count1];
	//bool** video_indexes = new bool*[frames_count1];
	//for (int i = 0; i < frames_count1; i++)
	//	video_indexes[i] = new bool[1024];

	// count hash for random frame
	__int64 frame_index = calcImageHash(random_frame1);
	//bool* frame_index = new bool[1024];
	//calcImageHash1(random_frame1, frame_index);

	// indexing video by histogram and searcing frame
	int startTime, endTime, allTime;
	Mat buferFrame;
	startTime = clock();
	for (int i = 0; i < frames_count1; i++)
	{
		cap_right.read(buferFrame);
		video_indexes[i] = calcImageHash(buferFrame);
		//calcImageHash1(buferFrame, video_indexes[i]);
	}
	endTime = clock();
	allTime = endTime - startTime;
	std::cout << "Indexing time: " << allTime << " " << "\n";



	double minCompare;
	int frameNumber;
	Mat maxFrame, current_frame;
	double current_distance;


	// count min
	startTime = clock();
	for (int i = 0; i < frames_count1; i++)
	{
		//current_distance = calcHammingDistance1(frame_index, video_indexes[i]);
		current_distance = calcHammingDistance(frame_index, video_indexes[i]);
		if (i == 0)
		{
			frameNumber = 0;
			minCompare = current_distance;
		}
		else
			if (current_distance < minCompare)
			{
				frameNumber = i;
				minCompare = current_distance;
			}
	}
	endTime = clock();
	cout << "Searcing time: " << endTime - startTime << endl;

	// show min frame
	cap_right.set(CV_CAP_PROP_POS_FRAMES, frameNumber);
	cap_right >> current_frame;
	cout << "Compare resut:  " << minCompare << "\nFrame number: " << frameNumber << "\n";
	imshow("Found", current_frame);
	imshow("Random", random_frame1);
	waitKey();
	system("pause");
	
	return 0;
}