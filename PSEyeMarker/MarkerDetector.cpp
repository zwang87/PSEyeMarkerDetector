#include "MarkerDetector.h";
#include <GL/gl.h>
#include <GL/glut.h>

MarkerDetector::MarkerDetector()
{
	vconf = "Data/WDM_camera.flipV.xml";
	cparam_name = "../Data/camera_para.dat";
	patt_name = "../Data/patt.hiro";
	thresh = 100;
	count = 0;
	patt_width = 80.0;
	memset(patt_center, 0.0, sizeof(patt_center));
}

void MarkerDetector::Init(int width, int height)
{
	
	cout << "Init() run!!!!!!!!!!!!!!!!!!!!" << endl;
	if((patt_id = arLoadPatt(patt_name)) < 0)
	{
		cout << patt_name << " pattern load error" << endl;
		exit(-1);
	}
	else
		cout << "pattern loaded!!!!" << endl;
	if (arParamLoad(cparam_name, 1, &cparam) < 0) {
		cout << "Camera calibration parameters file load error" << endl;
		exit(-1);
	}
	arParamChangeSize(&cparam, width, height, &wparam);

	arInitCparam(&wparam);
}



bool MarkerDetector::MainLoop(const cv::Mat& img, Point2f markerPoints[4])
{
	int j, k;
	count ++;
	//arParamChangeSize(&wparam, img.rows, img.cols, &cparam);
	ARMarkerInfo *markerInfo = NULL;
	int marker_num = -1;
	if(arDetectMarker((ARUint8*)img.data, 50, &markerInfo, &marker_num)<0)
	{
		return false;
	}
	k = -1;
	for( j = 0; j < marker_num; j++ ) {
		if( patt_id == markerInfo[j].id ) {
			if( k == -1 ) k = j;
			else if( markerInfo[k].cf < markerInfo[j].cf ) k = j;
		}
	}
	if( k == -1 ) {
		return false;
	}

	/* get the transformation between the marker and the real camera */
	arGetTransMat(&markerInfo[k], patt_center, patt_width, patt_trans);
	arUtilMatInv(patt_trans, cam_trans);
	
	markerPoints[0] = Point2f(markerInfo[k].vertex[0][0], markerInfo[k].vertex[0][1]);
	markerPoints[1] = Point2f(markerInfo[k].vertex[1][0], markerInfo[k].vertex[1][1]);
	markerPoints[2] = Point2f(markerInfo[k].vertex[2][0], markerInfo[k].vertex[2][1]);
	markerPoints[3] = Point2f(markerInfo[k].vertex[3][0], markerInfo[k].vertex[3][1]);

	if(count % 10 == 0 && k != -1)
	{
		cout << "camera pos: " << cam_trans[0][3] << ", " << cam_trans[1][3] << ", " << cam_trans[2][3] << endl;

	}
	return true;
}

void MarkerDetector::Draw(  )
{
	double    gl_para[16];
	GLfloat   mat_ambient[]     = {0.0, 0.0, 1.0, 1.0};
	GLfloat   mat_flash[]       = {0.0, 0.0, 1.0, 1.0};
	GLfloat   mat_flash_shiny[] = {50.0};
	GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
	GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
	GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};

	argDrawMode3D();
	argDraw3dCamera( 0, 0 );
	glClearDepth( 1.0 );
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	/* load the camera transformation matrix */
	argConvGlpara(patt_trans, gl_para);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd( gl_para );

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);	
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMatrixMode(GL_MODELVIEW);
	glTranslatef( 0.0, 0.0, 25.0 );
	glutSolidCube(50.0);
	glDisable( GL_LIGHTING );
	cout << "gl" << endl;
	glDisable( GL_DEPTH_TEST );
}

void MarkerDetector::Run()
{
	//argMainLoop(NULL, NULL, MainLoop);
}


/*
ARUint8* MarkerDetector::Mat2ARUint8(const cv::Mat &src)
{
	
	int width = src.cols;
	int height = src.rows;
	int channels = src.channels();
	int cvStep = src.step;
	int arStep = width * 4;

	ARUint8* dst = new ARUint8[width*height*channels];

	for(int i = 0; i < height; i++)
		for(int j = 0; j < width; j++)
			for(int c = 0; c < channels; c++)
				dst[arStep*i + j*4 + c] = src.data[cvStep*i + j*channels + c];

	//argDrawMode2D();
	//argDispImage(dst, 0, 0);
	//cout << "this works tooooooooooooo.............." << endl;
	
	//ARUint8* dst = (ARUint8*)src.data;
	//argDrawMode2D();
	//argDispImage(dst, 0, 0);
	return dst;
}
*/