#pragma once

//#include <../DSVL/src/GL/>
//#include <../DSVL/src/GL/glut.h>
#include <stdio.h>

//#include <GL/gl.h>
//#include <GL/glut.h>
#include <windows.h>
#include <iostream>
#include <stdlib.h>
#include <AR\ar.h>
#include <AR\gsub.h>
#include <AR\video.h>
#include <AR\param.h>
#include <vector>
#include <opencv2\opencv.hpp>

using namespace std;
using namespace cv;

class MarkerDetector
{
	char * vconf;
	int width, height;
	int thresh;
	int count;

	char *cparam_name;
	ARParam cparam;

	char *patt_name;
	int patt_id;
	double patt_width;
	double patt_center[2];
	double patt_trans[3][4];
	double cam_trans[3][4];

	ARUint8 *dataPtr;
	


	ARParam wparam;

public:
	MarkerDetector();
	void Init(int width, int height);
	void Cleanup();
	//void keyEvent(unsigned char key, int x, int y);
	bool MainLoop(const cv::Mat& img, Point2f markerPoints[4]);
	void Draw();
	void Run();
	ARUint8* Mat2ARUint8(const cv::Mat& src);
};