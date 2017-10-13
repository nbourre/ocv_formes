#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\highgui\highgui.hpp>

#include <iostream>
#include <time.h>

#define VK_ESCAPE 27

using namespace std;
using namespace cv;

enum usageMode { CAMERA, IMAGE, VIDEO };

// Prototype
void afficherManuel();
void init();
void input();
void update();
void render();
int readArgs(int, char*[]);
void textInContours(Mat binary, Mat writeTo, string text, Scalar color, int minContourArea);
void imShowValidated(String windowsName, InputArray mat);

Mat imgFrame;
VideoCapture capture;

char *filename = "";


int c = 0;
int thresh_value = 170;
int thresh_blue = 200;
int thresh_red = 45;


int increment = 5;
int dirtyFlag = 1;

int64 int_refresh;

int64 currentTime = getTickCount();
int64 previousTime = currentTime;

int64 deltaTime;
int64 acc_refresh = 0;

usageMode runningMode = CAMERA;

bool isReal = true;

int useCam = 1;
int camID = 0;
int done = 0;

void main(int argc, char *argv[])
{
	if (readArgs(argc, argv) == EXIT_FAILURE) {
		exit(EXIT_FAILURE);
	}

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

int readArgs(int argc, char * argv[]) {
	// TODO : Faire le reste des arguements


	string arg1 = argv[1];

	// -i image, -v video
	if (argc == 3) {
		if (arg1 == "-i") {
			runningMode = IMAGE;
			filename = argv[2];
			return EXIT_SUCCESS;
		}
	}

	

	return EXIT_FAILURE;
}

/**
@brief Procédure permettant d'inscrire des étiquettes dans les contours détectés

@param binary : Matrice binaire source
@param writeTo  : Matrice destination pour inscrire le texte
@param text : Texte à inscrire dans les étiquettes
@param color : Couleur du texte
@param minContourArea : Surface minimum pour inscrire une étiquette dans un contour
*/
void textInContours(Mat binary, Mat writeTo, string text, Scalar color = Scalar(0, 0, 0), int minContourArea = 0)
{
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	findContours(binary, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

	Size textSize = getTextSize(text, FONT_HERSHEY_PLAIN, 2.0, 1, 0);

	for (int i = 0; i < contours.size(); i++) {
		double area = contourArea(contours[i]);

		if (area > minContourArea) {
			//drawContours(imgContours, contours, i, Scalar(255, 255, 255));
			Rect r = boundingRect(contours[i]);
			//rectangle(imgContours, r, Scalar(255, 255, 255));
			putText(writeTo, text, Point(r.x + r.width / 2 - textSize.width / 2, r.y + r.height / 2), FONT_HERSHEY_PLAIN, 2.0, color);
		}
	}

}


void init()
{
	// Calcul du taux de rafraîchissement désiré
	int_refresh = (int)(getTickFrequency() / 30);

	if (runningMode == CAMERA || runningMode == VIDEO) {

		switch (runningMode) {
		case CAMERA:
			capture.open(0);
			break;
		case VIDEO:
			capture.open(filename);
			break;
		}

		if (!capture.isOpened()) {
			printf("--(!)Error opening video or camera capture\n");
			exit(EXIT_FAILURE);
		}
	}
	else if (runningMode == IMAGE) {
		imgFrame = imread(filename);
	}

	startWindowThread();

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

	case '1':
		thresh_blue -= increment;
		dirtyFlag = 1;
		break;
	case '2':
		thresh_blue += increment;
		dirtyFlag = 1;
		break;
	case '3':
		thresh_red -= increment;
		dirtyFlag = 1;
		break;
	case '4':
		thresh_red += increment;
		dirtyFlag = 1;
		break;
	case 't':
		// Toggle between real image or synthetic
		isReal = !isReal;
		dirtyFlag = 1;
		break;
	default:
		break;
	}
}


string imageType = "Synthetic";

void showValues() {
	if (isReal) {
		imageType = "Real";
	}
	else {
		imageType = "Synthetic";
	}

	cout << "Image type = " << imageType << endl;
	cout << "Threshold mask = " << thresh_value << endl;
	cout << "Threshold blue = " << thresh_blue << endl;
	cout << "Threshold red = " << thresh_red << endl;
}

Mat imgHSV, imgConverted, imgThresh, imgBlur;
Mat imgGray;
Mat imgBlue, imgRed, imgGreen;
Mat imgH;
vector<Mat> channels;
Mat imgContours;
Mat imgTest;
Mat imgResult;
Mat imgMask;


Mat element = getStructuringElement(MORPH_RECT,
	Size(2 * 1 + 1, 2 * 1 + 1),
	Point(1, 1));



int minContourArea = 100;


void update()
{
	if (dirtyFlag) {
		if (thresh_value > 255) {
			thresh_value = 255;
		}

		if (thresh_value < 0) {
			thresh_value = 0;
		}

		showValues();

		done = 0;
		dirtyFlag = 0;
	}


	if (runningMode == CAMERA || runningMode == VIDEO) {
		// Retour au frame 0 si on dépasse
		if (capture.get(CAP_PROP_POS_FRAMES) >= capture.get(CAP_PROP_FRAME_COUNT)) {
			capture.set(CAP_PROP_POS_FRAMES, 0.0);
		}

		capture.read(imgFrame);
		done = 0;
	}

	if (done == 0) {
		if (imgResult.empty()) {
			imgResult = imgFrame.clone();
		}

		imgFrame.copyTo(imgResult);

		// Conversion
		cvtColor(imgFrame, imgGray, CV_BGR2GRAY);

		// Création du masque des formes
		threshold(imgGray, imgMask, thresh_value, 255, CV_THRESH_BINARY_INV);
		erode(imgMask, imgMask, element); // Rapetissage du masque


		if (!isReal) {
			// TODO : Faire le code pour l'image synthétisée

		}
		else {
			// Conversion
			cvtColor(imgFrame, imgConverted, CV_BGR2HSV);
			split(imgConverted, channels);

			// Retrait du fond
			bitwise_and(imgMask, channels[0], imgH);

			/*BLEU*/
			threshold(imgH, imgBlue, 100, 255, CV_THRESH_TOZERO); // Passe haut
			threshold(imgBlue, imgBlue, 170, 255, CV_THRESH_TOZERO_INV); // Passe bas
			threshold(imgBlue, imgBlue, 100, 255, CV_THRESH_BINARY); // Passe bas
			textInContours(imgBlue, imgResult, "Bleu", Scalar(255, 255, 0), minContourArea);

			//*ROUGE*/
			threshold(imgH, imgRed, thresh_red, 255, CV_THRESH_BINARY_INV);
			bitwise_and(imgMask, imgRed, imgRed);
			erode(imgRed, imgRed, element, Point(-1, -1), 1); // Retrait des artéfacts
			dilate(imgRed, imgRed, element, Point(-1, -1), 4); // Retrait des trous
			textInContours(imgRed, imgResult, "Rouge", Scalar(0, 255, 255), minContourArea);

			///*Vert*/
			//* TODO : Compléter cette partie*/

			//textInContours(imgGreen, imgResult, "Vert", Scalar(255, 0, 255), minContourArea);
		}

		done = 1;
	}
}

char *winMain = "Main";
char *winHSV = "HSV";
char *winH = "Hue";
char *winBlur = "Blurred";
char *winGray = "Gray";
char *winThresh = "Thresh";
char *winMask = "Masque";
char *winOR = "Bitwise OR";
char *winContours = "Contours";
char *winBlue = "blue";
char *winRed = "red";

void render()
{
	//imShowValidated(winGray, imgConverted);
	imShowValidated(winMask, imgMask);
	//imShowValidated(winThresh, imgThresh);
	//imShowValidated(winBlue, imgBlue);
	//imShowValidated(winRed, imgRed);
	//imShowValidated(winRed, imgGreen);
	//imShowValidated(winH, imgH);

	imShowValidated(winMain, imgResult);

}

void imShowValidated(String windowsName, InputArray mat) {
	if (!mat.empty()) {
		imshow(windowsName, mat);
	}
	else {

		destroyWindow(windowsName);
	}
}
