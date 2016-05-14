// diplom1.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include "opencv2\core\core.hpp"
#include "opencv2\features2d\features2d.hpp"
#include "opencv2\highgui\highgui.hpp"
#include "opencv2\nonfree\nonfree.hpp"
#include "opencv2\opencv.hpp"
#include <vector>
#include <time.h>


using namespace cv;

const int K = 64;
const int TREE_SIZE = 4;



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


int countPosition(int k1, int k2, int k3)
{
	return 0;
}

int main()
{
	// read videos and count frames

	VideoCapture capRight("test4_2.mp4");
	VideoCapture capLeft("test4_1.mp4");

	double framesCountD1 = capRight.get(CV_CAP_PROP_FRAME_COUNT);
	double framesCountD2 = capLeft.get(CV_CAP_PROP_FRAME_COUNT);
	int framesCount1 = (int)framesCountD1 - 10;
	int framesCount2 = (int)framesCountD2;

	// choose random frame from capture2 video
	int randomFrameNumber = frame4_3;
	Mat randomFrame;
	capLeft.set(CV_CAP_PROP_POS_FRAMES, randomFrameNumber);
	capLeft >> randomFrame;


	// indexing random frame
	// parameters for indexing 
	int* randomFrameIndex = new int[K];
	int bins = 4;
	int histSize[] = { bins, bins, bins };
	float range[] = { 0, 256 };
	const float* ranges[] = { range, range, range };
	int channels[] = { 0, 1, 2 };
	MatND* histogram = new MatND[TREE_SIZE*TREE_SIZE];
	Mat bufRect;

	
	// indexing
	for (int i = 0; i < TREE_SIZE; i++)
	for (int j = 0; j < TREE_SIZE; j++)
	{
		Rect frameRect = Rect(i * (randomFrame.cols / TREE_SIZE), j*(randomFrame.rows / TREE_SIZE),
		randomFrame.cols / TREE_SIZE, randomFrame.rows / TREE_SIZE);
		randomFrame(frameRect).copyTo(bufRect);
		calcHist (&bufRect, 1, channels, Mat(),
				  histogram[i*TREE_SIZE + j], 3, histSize,
				  ranges, true, true);
		normalize(histogram[i*TREE_SIZE + j], histogram[i*TREE_SIZE + j], 1, 0, NORM_L1, -1, Mat());
	}
	
	// time variables
	clock_t startTime, endTime, allTime;
	

	// indexing video
	MatND** indices = new MatND*[framesCount1];
	Mat buferFrame;
	double minCompare, currentCompare;
	int frameNumber;
	Mat maxFrame;
	double buferCompare;
	startTime = clock();
	for (int i = 0; i < framesCount1; i++)
	{
		// create hist massiv
		indices[i] = new MatND[TREE_SIZE*TREE_SIZE];

		// read frame
		capRight >> buferFrame;

		// count index
		for (int j = 0; j < TREE_SIZE; j++)
			for (int l = 0; l < TREE_SIZE; l++)
			{
				Rect frameRect = Rect(j * (randomFrame.cols / TREE_SIZE), l*(randomFrame.rows / TREE_SIZE),
					randomFrame.cols / TREE_SIZE, randomFrame.rows / TREE_SIZE);
				buferFrame(frameRect).copyTo(bufRect);
				calcHist(&bufRect, 1, channels, Mat(),
					indices[i][j*TREE_SIZE + l], 3, histSize,
					ranges, true, true);
				normalize(indices[i][j*TREE_SIZE + l], indices[i][j*TREE_SIZE + l], 1, 0, NORM_L1, -1, Mat());
			}
	}
	endTime = clock();
	allTime = endTime - startTime;
	std::cout << allTime << "\n";


	// searcing frame
	startTime = clock();
	for (int i = 0; i < framesCount1; i++)
	{
		// nulling variables
		currentCompare = 0;
		for (int j = 0; j < TREE_SIZE; j++)
			for (int l = 0; l < TREE_SIZE; l++)
			{
				//buferCompare = compareHist(histogram[j*TREE_SIZE + l], indices[i][j*TREE_SIZE + l], CV_COMP_INTERSECT);
				//buferCompare = compareHist(histogram[j*TREE_SIZE + l], indices[i][j*TREE_SIZE + l], CV_COMP_CHISQR);
				buferCompare = compareHist(histogram[j*TREE_SIZE + l], indices[i][j*TREE_SIZE + l], CV_COMP_HELLINGER);
				
				// buferCompare *= buferCompare;
				currentCompare += buferCompare;
			}
		if (i == 0)
		{
			minCompare = currentCompare;
			frameNumber = i;
		}
		else
			if (minCompare > currentCompare)
			{
				minCompare = currentCompare;
				frameNumber = i;
			}
	}
	endTime = clock();
	allTime = endTime - startTime;
	
	// writing
	std::cout << "Searcing Time: " << allTime << "\nValue of min compare: " << minCompare;
	std::cout << "\nFrame Number: " << frameNumber << "\n";

	capRight.set(CV_CAP_PROP_POS_FRAMES, frameNumber);
	//capLeft.read(randomFrame);
	imshow("Example", randomFrame);
	capRight.read(maxFrame);
	imshow("Found", maxFrame);
	waitKey(0);

	system("pause");

	return 0;
}
