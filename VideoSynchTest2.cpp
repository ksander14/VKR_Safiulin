// VidoeSynch.cpp: определяет точку входа для консольного приложения.
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


// Функции для работы с гистограммами
// построение характеризующих векторов
void build_sequences_hists(int frames_count1, int frames_count2, MatND* indices1, MatND* indices2, double* sequence_1, double* sequence_2)
{
	for (int i = 0; i < frames_count1 - 1; i++)
		sequence_1[i] = compareHist(indices1[i], indices1[i + 1], CV_COMP_HELLINGER);
	for (int j = 0; j < frames_count2 - 1; j++)
		sequence_2[j] = compareHist(indices2[j], indices2[j + 1], CV_COMP_HELLINGER);
}



// Функции для работы с матрицами переходов
// grey level cooccurence matrix count
void glcm(Mat img1, double** matrixes)
{
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

// grey level cooccurence matrix distance
double glcmDistance(double **matrix1, double **matrix2)
{
	double distance = 0;
	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 4; j++)
			distance += abs(matrix1[i][j] - matrix2[i][j]);
	return distance;
}

// построение характеризующих векторов
void build_sequences_glcms(int frames_count1, int frames_count2, double*** glcms_1, double*** glcms_2, double* sequence_1, double* sequence_2)
{
	for (int i = 0; i < frames_count1 - 1; i++)
		sequence_1[i] = glcmDistance(glcms_1[i], glcms_1[i + 1]);
	for (int j = 0; j < frames_count2 - 1; j++)
		sequence_2[j] = glcmDistance(glcms_2[j], glcms_2[j + 1]);
}



// Функции на случай сравнения хешей 32 на 32
// хеш картинки
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

// расстояние хэмминга
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

// построение векторов
void build_sequences_hash1(int frames_count1, int frames_count2, bool** hashes_1, bool** hashes_2, double* sequence_1, double* sequence_2)
{
	for (int i = 0; i < frames_count1 - 1; i++)
		sequence_1[i] = calcHammingDistance1(hashes_1[i], hashes_1[i + 1]);
	for (int j = 0; j < frames_count2 - 1; j++)
		sequence_2[j] = calcHammingDistance1(hashes_2[j], hashes_2[j + 1]);
}



// Основная функция синхронизации видеопотоков
void synchronization(int frames_count1, int frames_count2, double* sequence_1, double* sequence_2, int &index1, int &index2)
{
	// Инициализация переменных	
	int min_size = (int) (0.5 * (double)frames_count1);
	double min_comp = frames_count1 * 10;
	double current_comp;
	index1 = 0;
	index2 = 0;


	// Прогонка первого сегмента
	for (int i = min_size; i < frames_count1; i++)
	{
		current_comp = 0;
		for (int k = 0; k < i; k++)
		{
			// Метрика евклида
			current_comp += pow((sequence_1[k + frames_count1 - i] - sequence_2[k]), 2);
			// Метрика модуль
			//current_comp += abs(sequence_1[k + frames_count1 - i] - sequence_2[k]);
			// Метрика L0
			//current_comp += (sequence_1[k + frames_count1 - i] == sequence_2[k]) ? 0 : 1;
		}
		current_comp /= (double)i;
		if (i == min_size)
		{
			index1 = frames_count1 - i;
			min_comp = current_comp;
			index2 = 0;
		}
		if (current_comp < min_comp)
		{
			index1 = frames_count1 - i;
			min_comp = current_comp;
			index2 = 0;
		}
	}
	int finish = 0;

	// Прогонка второго сегмента
	for (int i = 0; i < frames_count2 - frames_count1 + 1; i++)
	{
		current_comp = 0;
		for (int k = 0; k < frames_count1; k++)
		{
			// Метрика евклида
			current_comp += pow((sequence_1[k] - sequence_2[i + k]), 2);
			// Метрика модуль
			//current_comp += abs(sequence_1[k] - sequence_2[i + k]);
			// Метрика L0
			//current_comp += (sequence_1[k] == sequence_2[i + k]) ? 0 : 1;
		}
		current_comp /= (double)frames_count1;
		if (current_comp < min_comp)
		{
			index1 = 0;
			min_comp = current_comp;
			index2 = i;
		}
	}
	finish = 0;

	// Прогонка третьего сегмента
	for (int i = frames_count1 - 1; i >= min_size; i--)
	{
		current_comp = 0;
		for (int k = 0; k < i; k++)
		{
			// Метрика евклида
			current_comp += pow((sequence_1[k] - sequence_2[frames_count2 - i + k]), 2);
			// Метрика модуль
			//current_comp += abs(sequence_1[k] - sequence_2[frames_count2 - i + k]);
			// Метрика L0
			//current_comp += (sequence_1[k] == sequence_2[frames_count2 - i + k]) ? 0 : 1;
		}
		current_comp /= (double)i;
		if (current_comp < min_comp)
		{
			index1 = 0;
			min_comp = current_comp;
			index2 = frames_count2 - i;
		}
	}
	finish = 0;
}



int main()
{
	// считывание видео и метаданных из файлов
	VideoCapture cap_right("test6_1.mp4");
	VideoCapture cap_left("test6_2.mp4");
	double frames_countD1 = cap_right.get(CV_CAP_PROP_FRAME_COUNT);
	double frames_countD2 = cap_left.get(CV_CAP_PROP_FRAME_COUNT);
	int frames_count1 = (int)frames_countD1;
	int frames_count2 = (int)frames_countD2;

	double fps_1 = cap_right.get(CV_CAP_PROP_FPS);
	double fps_2 = cap_left.get(CV_CAP_PROP_FPS);

	Mat buferFrame;
	Mat buferFrame_1;
	int start1 = 0, start2 = 0;

	
	// Объявление переменных на каждый из вариантов
	/*
	// Гистограммы
	int bins = 4;
	int histSize[] = { bins, bins, bins };
	float range[] = { 0, 256 };
	const float* ranges[] = { range, range, range };
	int channels[] = { 0, 1, 2 };
	MatND* indices1 = new MatND[frames_count1];
	MatND* indices2 = new MatND[frames_count2];
	double minCompare, currentCompare;
	int frameNumber = 0;
	Mat maxFrame;
	*/
	// Матрицы переходов
	double*** video_1_glcms = new double**[frames_count1];
	double*** video_2_glcms = new double**[frames_count1];
	for (int i = 0; i < frames_count1; i++)
	{
		video_1_glcms[i] = new double*[8];
		for (int j = 0; j < 8; j++)
		{
			video_1_glcms[i][j] = new double[4];
			for (int l = 0; l < 4; l++)
				video_1_glcms[i][j][l] = 0;
		}
	}
	for (int i = 0; i < frames_count2; i++)
	{
		video_2_glcms[i] = new double*[8];
		for (int j = 0; j < 8; j++)
		{
			video_2_glcms[i][j] = new double[4];
			for (int l = 0; l < 4; l++)
				video_2_glcms[i][j][l] = 0;
		}
	}
	/*
	// Хэши
	bool** hashes_1 = new bool*[frames_count1];
	for (int i = 0; i < frames_count1; i++)
		hashes_1[i] = new bool[1024];
	bool** hashes_2 = new bool*[frames_count2];
	for (int i = 0; i < frames_count2; i++)
		hashes_2[i] = new bool[1024];
	*/

	// Индексирование каждого видео в зависимости от метода индексации
	for (int i = 0; i < frames_count1; i++)
	{
		cap_right.read(buferFrame);
		if (buferFrame.rows == 0)
		{

			// Гистограммы
			//indices1[i] = indices1[i - 1];
			// Матрицы переходов
			video_1_glcms[i] = video_1_glcms[i - 1];
			// Хеши
			//hashes_1[i] = hashes_1[i - 1];
			continue;
		}
		// Гистограммы
		//calcHist(&buferFrame, 1, channels, Mat(),
		//	indices1[i], 3, histSize,
		//	ranges, true, false);
		//normalize(indices1[i], indices1[i], 1, 0, NORM_L1, -1, Mat());
		// Матрицы переходов
		cvtColor(buferFrame, buferFrame, CV_BGR2GRAY);
		int s = 124;
		glcm(buferFrame, video_1_glcms[i]);
		// Хеши
		//calcImageHash1(buferFrame, hashes_1[i]);
	}
	for (int i = 0; i < frames_count2; i++)
	{
		cap_left.read(buferFrame);
		if (buferFrame.rows == 0)
		{
			// Гистограммы
			//indices2[i] = indices2[i - 1];
			// Матрицы переходов
			video_2_glcms[i] = video_2_glcms[i - 1];
			// Хеши
			//hashes_2[i] = hashes_2[i - 1];
			continue;
		}
		// Гистограммы
		//calcHist(&buferFrame, 1, channels, Mat(),
		//	indices2[i], 3, histSize,
		//	ranges, true, false);
		//normalize(indices2[i], indices2[i], 1, 0, NORM_L1, -1, Mat());
		// Матрицы переходов
		cvtColor(buferFrame, buferFrame, CV_BGR2GRAY);
		glcm(buferFrame, video_2_glcms[i]);
		// Хеши
		//calcImageHash1(buferFrame, hashes_2[i]);
	}


	time_t start_time, finish_time;
	start_time = clock();
	// Построение характеризующих векторов для каждого видео
	double* sequence_1 = new double[frames_count1 - 1];
	double* sequence_2 = new double[frames_count2 - 1];
	// Гистограммы
	//build_sequences_hists(frames_count1, frames_count2, indices1, indices2, sequence_1, sequence_2);
	// Матрицы переходов
	build_sequences_glcms(frames_count1, frames_count2, video_1_glcms, video_2_glcms, sequence_1, sequence_2);
	// Хеши
	//build_sequences_hash1(frames_count1, frames_count2, hashes_1, hashes_2, sequence_1, sequence_2);
	
	/*
	for (int i = 0; i < frames_count1 - 1; i++)
		cout << sequence_1[i] << "       " << sequence_2[i] << endl;
	for (int i = frames_count1 - 1; i < frames_count2 - 1; i++)
		cout << "                    " << sequence_2[i] << endl;
	*/

	// синхрозируем видеопотоки
	// int window_size = fps_1 * approx_time;
	// Гистограммы
	// Матрицы переходов
	// Хеши
	synchronization(frames_count1 - 1, frames_count2 - 1, sequence_1, sequence_2, start1, start2);
	finish_time = clock();


	// Выводим результаты
	cout << "Synch time: " << finish_time - start_time << endl;
	cout << "1 video position: " << start1 << "\n2 video position: " << start2 << endl;
	cap_right.set(CV_CAP_PROP_POS_FRAMES, start1);
	cap_left.set(CV_CAP_PROP_POS_FRAMES, start2);
	Mat buferFrame1, buferFrame2;
	for (int i = start2; i < frames_count1; i++)
	{

		cap_right.read(buferFrame1);
		cap_left.read(buferFrame2);
		imshow("1", buferFrame1);
		imshow("2", buferFrame2);
		char c = cvWaitKey(20);
		if (c == 27)
			break;
	}
	system("pause");
	return 0;
}