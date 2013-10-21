#include "stdafx.h"
#include "MarkerDetector.h"

CLEyeCameraCapture::CLEyeCameraCapture(LPSTR windowName, GUID cameraGUID, CLEyeCameraColorMode mode, CLEyeCameraResolution resolution, float fps) :
_cameraGUID(cameraGUID), _cam(NULL), _mode(mode), _resolution(resolution), _fps(fps), _running(false)
{
	strcpy(_windowName, windowName);
	pCapBuffer = NULL;
	bool _isTracking = false;
	markerDetector = new MarkerDetector();
}
bool CLEyeCameraCapture::StartCapture()
{
	_running = true;
	namedWindow(_windowName, 0);
	// Start CLEye image capture thread
	_hThread = CreateThread(NULL, 0, &CLEyeCameraCapture::CaptureThread, this, 0, 0);
	if(_hThread == NULL)
	{
		//MessageBox(NULL,"Could not create capture thread","CLEyeMulticamTest", MB_ICONEXCLAMATION);
		return false;
	}
	return true;
}
void CLEyeCameraCapture::StopCapture()
{
	if(!_running)	return;
	_running = false;
	WaitForSingleObject(_hThread, 1000);
	destroyAllWindows();
}
void CLEyeCameraCapture::IncrementCameraParameter(int param)
{
	if(!_cam)	return;
	cout << "CLEyeGetCameraParameter " << CLEyeGetCameraParameter(_cam, (CLEyeCameraParameter)param) << endl;
	CLEyeSetCameraParameter(_cam, (CLEyeCameraParameter)param, CLEyeGetCameraParameter(_cam, (CLEyeCameraParameter)param)+10);
}
void CLEyeCameraCapture::DecrementCameraParameter(int param)
{
	if(!_cam)	return;
	cout << "CLEyeGetCameraParameter " << CLEyeGetCameraParameter(_cam, (CLEyeCameraParameter)param) << endl;
	CLEyeSetCameraParameter(_cam, (CLEyeCameraParameter)param, CLEyeGetCameraParameter(_cam, (CLEyeCameraParameter)param)-10);
}
void CLEyeCameraCapture::Run()
{	
	// Create camera instance
	_cam = CLEyeCreateCamera(_cameraGUID, _mode, _resolution, _fps);
	if(_cam == NULL)		return;
	// Get camera frame dimensions
	CLEyeCameraGetFrameDimensions(_cam, w, h);
	// Depending on color mode chosen, create the appropriate OpenCV image
	if(_mode == CLEYE_COLOR_PROCESSED || _mode == CLEYE_COLOR_RAW)
		pCapImage = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 4);
	else
		pCapImage = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 1);

	// Set some camera parameters
	//CLEyeSetCameraParameter(_cam, CLEYE_GAIN, 20);
	//CLEyeSetCameraParameter(_cam, CLEYE_EXPOSURE, 511);
	CLEyeSetCameraParameter(_cam, CLEYE_AUTO_GAIN, true);
	CLEyeSetCameraParameter(_cam, CLEYE_AUTO_EXPOSURE, true);
	//CLEyeSetCameraParameter( _cam, CLEYE_HFLIP, true);

	// Start capturing
	CLEyeCameraStart(_cam);
	cvGetImageRawData(pCapImage, &pCapBuffer);
	pCapture = pCapImage;
	long frames = 0;
	long count = GetTickCount();
	long prevCount = 0;
	double fps = 0;
	// image capturing loop
	Mat src_gray, subImage, subImage_gray;

	vector<Vec3f> circles;
	Point center;
	Point n_center;
	int radius = 0;
	int counter = 0;

	char* fpsText = new char[5];
	char* pos_text = new char[10];
	//initialize marker detector
	markerDetector->Init();

	while(_running)
	{
		CLEyeCameraGetFrame(_cam, pCapBuffer);
		//check fps every 100 frames
		frames++;

		if((frames % 100) == 0){
			prevCount = count;
			count = GetTickCount();
			fps = 100000.0/(count - prevCount);
			//std::cout << "fps: " << fps << endl;
			sprintf(fpsText, "fps: %f", fps);
		}

		//points of marker rectangle contour
		Point2f markerPoints[4];
		markerDetector->MainLoop(pCapture, markerPoints);
		//draw 4 edges of the rectangle of marker 
		for(int i = 0; i < 3; i++)
			line(pCapture, markerPoints[i], markerPoints[i+1], Scalar(255, 0, 0), 3);
		line(pCapture, markerPoints[0], markerPoints[3], Scalar(255, 0, 0), 3);

		if(frames > 100)
			putText(pCapture, fpsText, Point(5, 20), CV_FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0));
		else
			putText(pCapture, "calculating fps...", Point(5, 20), CV_FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0));

		imshow(_windowName, pCapture);
	}

	// Stop camera capture
	CLEyeCameraStop(_cam);
	// Destroy camera object
	CLEyeDestroyCamera(_cam);
	// Destroy the allocated OpenCV image
	cvReleaseImage(&pCapImage);
	_cam = NULL;
}

void CLEyeCameraCapture::CircleDetector(Mat& input, Mat& input_gray, vector<Vec3f>& circles, Point& center, int& radius)
{
	if(input.data != NULL){
		cvtColor(input, input_gray, CV_BGR2GRAY);
		GaussianBlur(input_gray, input_gray, Size(3, 3), 2, 2);
		HoughCircles(input_gray, circles, CV_HOUGH_GRADIENT, 2, input_gray.rows, 130, 50, 5, 20);
		for(size_t i = 0; i < circles.size(); i++)
		{
			center.x = cvRound(circles[i][0]);
			center.y = cvRound(circles[i][1]);
			radius = cvRound(circles[i][2]);
			circle(input, center, 3, Scalar(0, 255, 0), -1, 8, 0);
			circle(input, center, radius, Scalar(0, 0, 255), 3, 8, 0);
		}
	}
}

Point CLEyeCameraCapture::FixSubImageSize(Point center, int w, int h, int size)
{
	center.x = center.x < size ? size : center.x;
	center.x = center.x > w - size ? w - size : center.x;
	center.y = center.y < size ? size : center.y;
	center.y = center.y > h - size ? h - size : center.y;
	return center;
}

Mat CLEyeCameraCapture::GetCaptureImage()
{
	return pCapture;
}

// Main program entry point
//int _tmain(int argc, _TCHAR* argv[])
int main(int argc, char** argv)
{
	CLEyeCameraCapture *cam[2] = { NULL };
	srand(GetTickCount());
	// Query for number of connected cameras
	int numCams = CLEyeGetCameraCount();
	if(numCams == 0)
	{
		cout << "No PS3Eye cameras detected" << endl;
		return -1;
	}
	cout << "Found " << numCams << " cameras" << endl;
	for(int i = 0; i < numCams; i++)
	{
		char windowName[64];
		// Query unique camera uuid
		GUID guid = CLEyeGetCameraUUID(i);
		printf("Camera %d GUID: [%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x]\n", 
			i+1, guid.Data1, guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1], guid.Data4[2],
			guid.Data4[3], guid.Data4[4], guid.Data4[5],
			guid.Data4[6], guid.Data4[7]);
		sprintf(windowName, "Camera Window %d", i+1);
		// Create camera capture object
		cam[i] = new CLEyeCameraCapture(windowName, guid,  CLEYE_COLOR_PROCESSED, CLEYE_VGA, 60);
		cout << "Starting capture on camera " << i+1 << endl;
		cam[i]->StartCapture();
	}
	cout << "Use the following keys to change camera parameters:\n"
		"\t'1' - select camera 1\n"
		"\t'2' - select camera 2\n"
		"\t'g' - select gain parameter\n"
		"\t'e' - select exposure parameter\n"
		"\t'z' - select zoom parameter\n"
		"\t'r' - select rotation parameter\n"
		"\t'+' - increment selected parameter\n"
		"\t'-' - decrement selected parameter\n" << endl;
	// The <ESC> key will exit the program
	CLEyeCameraCapture *pCam = NULL;
	int param = -1, key;
	while((key = cvWaitKey(0)) != 0x1b)
	{
		switch(key)
		{
		case 'g':	case 'G':	cout << "Parameter Gain" << endl;		param = CLEYE_GAIN;		break;
		case 'e':	case 'E':	cout << "Parameter Exposure" << endl;	param = CLEYE_EXPOSURE;	break;
		case 'z':	case 'Z':	cout << "Parameter Zoom" << endl;		param = CLEYE_ZOOM;		break;
		case 'r':	case 'R':	cout << "Parameter Rotation" << endl;	param = CLEYE_ROTATION;	break;
		case '1':				cout << "Selected camera 1" << endl;	pCam = cam[0];			break;
		case '2':				cout << "Selected camera 2" << endl;	pCam = cam[1];			break;
		case '+':	if(numCams == 1){
			pCam = cam[0];
					}
					if(pCam)	pCam->IncrementCameraParameter(param);		break;
		case '-':	if(numCams == 1){
			pCam = cam[0];
					}
					if(pCam)	pCam->DecrementCameraParameter(param);		break;
		}
	}
	//delete cams
	for(int i = 0; i < numCams; i++)
	{
		cout << "Stopping capture on camera " << i+1 << endl;
		cam[i]->StopCapture();
		delete cam[i];
	}
	return 0;
}