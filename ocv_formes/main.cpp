#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\highgui\highgui.hpp>

#include <iostream>
#include <time.h>

#define VK_ESCAPE 27

using namespace std;
using namespace cv;

// Prototype
void afficherManuel();
void init();
void input();
void update();
void render();

Mat imgFrame;
VideoCapture capture;

char *filename = "";
char *winName = "Principale";

int c = 0;
int thresh_value = 127;
int increment = 5;
int dirtyFlag = 1;

int64 int_refresh;

int64 currentTime = getTickCount();
int64 previousTime = currentTime;

int64 deltaTime;
int64 acc_refresh = 0;

void main(int argc, char *argv[])
{
	init();

	while (c != VK_ESCAPE)
	{
		/// Gestion du temps

		currentTime = getTickCount();
		deltaTime = currentTime - previousTime;
		previousTime = currentTime;

		acc_refresh += deltaTime;

		input();
		update();

		if (acc_refresh > int_refresh) {
			render();
			acc_refresh = 0;
		}
	}

	return;
}

// Affiche le manuel d'instruction
void afficherManuel()
{
	cout << "Usage : ocv_final nomDuFichierVideo\r\n";
	cout << "\tnomDuFichierVideo\t: Nom complet vers le fichier video\r\n";
}

void init()
{
	int_refresh = getTickFrequency() / 30;

	capture.open(0);

	if (!capture.isOpened()) {
		printf("--(!)Error opening video capture\n");
		exit(-1);
	}
	namedWindow(winName);
}


void input()
{
	c = waitKey(1);

	switch (c)
	{
	case '=':
		thresh_value += increment;
		dirtyFlag = 1;
		break;
	case '-':
		thresh_value -= increment;
		dirtyFlag = 1;
		break;
	default:
		break;
	}
}

void update()
{
	if (dirtyFlag) {
		if (thresh_value > 255) {
			thresh_value = 255;
		}

		if (thresh_value < 0) {
			thresh_value = 0;
		}

		cout << "Threshold = " << thresh_value << endl;
		dirtyFlag = 0;
	}

	capture.read(imgFrame);


}

void render()
{
	imshow(winName, imgFrame);

}
