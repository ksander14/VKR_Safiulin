// perehod_matrix.cpp: определяет точку входа для консольного приложения.
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

void glcm(Mat img1, double** matrixes)
{
	Mat img2 = img1.clone();
	unsigned int **quanting = new unsigned int*[img1.rows];
	for (int i = 0; i < img1.rows; i++)
	{
		quanting[i] = new unsigned int[img1.cols];
		for (int j = 0; j < img1.cols; j++)
			quanting[i][j] = (int)img1.at<unsigned char>(i, j) % 4;
	}
	for (int i = 0; i < img1.rows; i++)
	{
		for (int j = 0; j < img1.cols - 1; j++)
		{
			matrixes[quanting[i][j]][quanting[i][j + 1]]++;
		}
	}
	for (int i = 0; i < img1.rows - 1; i++)
		for (int j = 0; j < img1.cols; j++)
		{
			matrixes[quanting[i][j] + 4][quanting[i + 1][j]]++;
		}
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			matrixes[i][j] /= (img1.rows * (img1.cols - 1));
	for (int i = 4; i < 8; i++)
		for (int j = 0; j < 4; j++)
			matrixes[i][j] /= (img1.cols * (img1.rows - 1));
	for (int i = 0; i < img1.rows; i++)
		delete[] quanting[i];
	delete[] quanting;
}

double glcmDistance(double **matrix1, double **matrix2)
{
	double distance = 0;
	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 4; j++)
			distance += abs(matrix1[i][j] - matrix2[i][j]);
	return distance;
}



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

	VideoCapture cap_left("test1_1.mp4");
	VideoCapture cap_right("test1_2.mp4");

	double framesCountD1 = cap_right.get(CV_CAP_PROP_FRAME_COUNT);
	double framesCountD2 = cap_left.get(CV_CAP_PROP_FRAME_COUNT);
	int frames_count1 = (int)framesCountD1;
	int frames_count2 = (int)framesCountD2;

	// choose random frame from capture2 video
	int randomFrameNumber = frame1_1;
	Mat random_frame, random_frame1;
	cap_left.set(CV_CAP_PROP_POS_FRAMES, randomFrameNumber);
	cap_left >> random_frame1;


	cvtColor(random_frame1, random_frame, CV_BGR2GRAY);
	

	// matrixes declaration
	double*** matrixes = new double**[frames_count1];
	for (int i = 0; i < frames_count1; i++)
	{
		matrixes[i] = new double*[8];
		for (int j = 0; j < 8; j++)
		{
			matrixes[i][j] = new double[4];
			for (int l = 0; l < 4; l++)
				matrixes[i][j][l] = 0;
		}
	}
	double** frameMatrix = new double*[8];
	for (int i = 0; i < 8; i++)
	{
		frameMatrix[i] = new double[4];
		for (int j = 0; j < 4; j++)
			frameMatrix[i][j] = 0;
	}


	// count matrix for random frame
	glcm(random_frame, frameMatrix);


	/*
	// count matrixes for video
	time_t start_time = clock();
	for (int i = 0; i < frames_count2; i++)
	{
		cap_right >> current_frame;
		cvtColor(current_frame, current_frame, CV_BGR2GRAY);
		glcm(current_frame, matrixes[i]);
	}
	time_t end_time = clock();
	cout << end_time - start_time << endl;*/
	

	// indexing random frame
	int* randomFrameIndex = new int[K];

	int bins = 4;
	int histSize[] = { bins, bins, bins };
	float range[] = { 0, 256 };
	const float* ranges[] = { range, range, range };
	int channels[] = { 0, 1, 2 };
	MatND histogram;
	calcHist(&random_frame1, 1, channels, Mat(),
		histogram, 3, histSize,
		ranges, true, false);
	normalize(histogram, histogram, 1, 0, NORM_L1, -1, Mat());
	clock_t startTime, endTime, allTime;


	// indexing video by histogram and searcing frame
	MatND* indices = new MatND[frames_count1];
	Mat buferFrame;
	startTime = clock();
	for (int i = 0; i < frames_count1; i++)
	{
		cap_right.read(buferFrame);
		/*calcHist(&buferFrame, 1, channels, Mat(),
			indices[i], 3, histSize,
			ranges, true, false);
		normalize(indices[i], indices[i], 1, 0, NORM_L1, -1, Mat());*/
		cvtColor(buferFrame, buferFrame, CV_BGR2GRAY);
		glcm(buferFrame, matrixes[i]);
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
		current_distance = glcmDistance(matrixes[i], frameMatrix);
		// currentCompare = compareHist(histogram, indices[i], CV_COMP_HELLINGER);
		if (i == 0)
		{
			frameNumber = 0;
			minCompare = /*currentCompare +*/ current_distance;
		}
		else
			if ( (current_distance/* + currentCompare*/) < minCompare)
			{
				frameNumber = i;
				minCompare = /*currentCompare +*/ current_distance;
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
