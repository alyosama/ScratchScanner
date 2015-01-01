#include "helper.h"
using namespace cv;
using namespace std;

void sortPointsClockwise(Point2f a[])
{

	Point2f b[4];

	Point2f ctr = (a[0] + a[1] + a[2] + a[3]);
	ctr.x /= 4;
	ctr.y /= 4;
	b[0] = a[0] - ctr;
	b[1] = a[1] - ctr;
	b[2] = a[2] - ctr;
	b[3] = a[3] - ctr;

	for (int i = 0; i<4; i++)
	{
		if (b[i].x < 0)
		{
			if (b[i].y < 0)
				a[0] = b[i] + ctr;
			else
				a[3] = b[i] + ctr;
		}
		else
		{
			if (b[i].y < 0)
				a[1] = b[i] + ctr;
			else
				a[2] = b[i] + ctr;
		}
	}

}

void dr(Mat& src, Point2f rect_points[4], Scalar t = Scalar(0, 0, 255))
{
	for (int j = 0; j < 4; j++)
		line(src, rect_points[j], rect_points[(j + 1) % 4], t, 2, 8);
}


void tst(Mat& src, Mat& src_inv, Mat& dst, Mat& kernel, Point& p)
{

	dst.at<uchar>(p.y, p.x) = 255;

	cv::Mat prev;
	/*cv::Mat kernel = (cv::Mat_<uchar>(3, 3) << 0, 1, 0, 1, 1, 1, 0, 1, 0);*/

	do {
		dst.copyTo(prev);
		cv::dilate(dst, dst, kernel);
		dst &= src_inv;
	} while (cv::countNonZero(dst - prev) > 0);

	src = src | dst;

}


void thinningIteration(cv::Mat& img, int iter)
{
	CV_Assert(img.channels() == 1);
	CV_Assert(img.depth() != sizeof(uchar));
	CV_Assert(img.rows > 3 && img.cols > 3);

	cv::Mat marker = cv::Mat::zeros(img.size(), CV_8UC1);

	int nRows = img.rows;
	int nCols = img.cols;

	if (img.isContinuous()) {
		nCols *= nRows;
		nRows = 1;
	}

	int x, y;
	uchar *pAbove;
	uchar *pCurr;
	uchar *pBelow;
	uchar *nw, *no, *ne;    // north (pAbove)
	uchar *we, *me, *ea;
	uchar *sw, *so, *se;    // south (pBelow)

	uchar *pDst;

	// initialize row pointers
	pAbove = NULL;
	pCurr = img.ptr<uchar>(0);
	pBelow = img.ptr<uchar>(1);

	for (y = 1; y < img.rows - 1; ++y) {
		// shift the rows up by one
		pAbove = pCurr;
		pCurr = pBelow;
		pBelow = img.ptr<uchar>(y + 1);

		pDst = marker.ptr<uchar>(y);

		// initialize col pointers
		no = &(pAbove[0]);
		ne = &(pAbove[1]);
		me = &(pCurr[0]);
		ea = &(pCurr[1]);
		so = &(pBelow[0]);
		se = &(pBelow[1]);

		for (x = 1; x < img.cols - 1; ++x) {
			// shift col pointers left by one (scan left to right)
			nw = no;
			no = ne;
			ne = &(pAbove[x + 1]);
			we = me;
			me = ea;
			ea = &(pCurr[x + 1]);
			sw = so;
			so = se;
			se = &(pBelow[x + 1]);

			int A = (*no == 0 && *ne == 1) + (*ne == 0 && *ea == 1) +
				(*ea == 0 && *se == 1) + (*se == 0 && *so == 1) +
				(*so == 0 && *sw == 1) + (*sw == 0 && *we == 1) +
				(*we == 0 && *nw == 1) + (*nw == 0 && *no == 1);
			int B = *no + *ne + *ea + *se + *so + *sw + *we + *nw;
			int m1 = iter == 0 ? (*no * *ea * *so) : (*no * *ea * *we);
			int m2 = iter == 0 ? (*ea * *so * *we) : (*no * *so * *we);

			if (A == 1 && (B >= 2 && B <= 6) && m1 == 0 && m2 == 0)
				pDst[x] = 1;
		}
	}

	img &= ~marker;
}


void thinning(const cv::Mat& src, cv::Mat& dst)
{
	dst = src.clone();
	dst /= 255;         // convert to binary image

	cv::Mat prev = cv::Mat::zeros(dst.size(), CV_8UC1);
	cv::Mat diff;

	do {
		thinningIteration(dst, 0);
		thinningIteration(dst, 1);
		cv::absdiff(dst, prev, diff);
		dst.copyTo(prev);
	} while (cv::countNonZero(diff) > 0);

	dst *= 255;
}

void getColor(Mat &src)
{
	//For Orange
	int OrangeLowH = 0;
	int OrangeHighH = 22;

	//For Blue
	int BlueLowH = 75;
	int BlueHighH = 130;

	//For Red
	int RedLowH = 160;
	int RedHighH = 179;

	int iLowS = 150;
	int iHighS = 255;

	int iLowV = 60;
	int iHighV = 255;

	int hist[3] = { 0 };
	Mat  hsv;
	cvtColor(src, hsv, COLOR_BGR2HSV);
	Mat imgThresholded;
	inRange(hsv, Scalar(RedLowH, iLowS, iLowV), Scalar(RedHighH, iHighS, iHighV), imgThresholded); //Threshold the image
	hist[0] = countNonZero(imgThresholded);

	inRange(hsv, Scalar(BlueLowH, iLowS, iLowV), Scalar(BlueHighH, iHighS, iHighV), imgThresholded); //Threshold the image
	hist[1] = countNonZero(imgThresholded);

	inRange(hsv, Scalar(OrangeLowH, iLowS, iLowV), Scalar(OrangeHighH, iHighS, iHighV), imgThresholded); //Threshold the image
	hist[2] = countNonZero(imgThresholded);


	cout << "The Color of Card is ";
	if (hist[0]>hist[1]){
		if (hist[0]>hist[2]){
			cout << "Red" << endl;
		}
		else{
			cout << "Orange" << endl;
		}
	}
	else{
		if (hist[1]>hist[2]){
			cout << "Blue" << endl;
		}
		else{
			cout << "Orange" << endl;
		}
	}
}





bool extract(string imageName)
{
	int thresh = 80;
	Mat src, src_gray, full, outImg;

	// read image
	src = imread(imageName + ".jpg");
	//keep a copy of full resolution image
	full = src.clone();
	// Blur and downscae the image for better processing
	GaussianBlur(src, src, Size(7, 7), 2);
	pyrDown(src, src);

	//Make gray scale copy of image for further processing
	cvtColor(src, src_gray, CV_BGR2GRAY);


	/// Detect edges using canny
	Mat canny_output;
	Canny(src_gray, canny_output, thresh, thresh * 3, 3);
	dilate(canny_output, canny_output, Mat());

	/// Find contours
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours(canny_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	/// Get Largest Contour
	int largest_area = 0;
	int largest_contour_index = 0;
	for (int i = 0; i< contours.size(); i++)
	{
		double a = fabs(contourArea(contours[i]));  //  Find the area of contour
		if (a>largest_area){
			largest_area = (int)a;
			largest_contour_index = i;                //Store the index of largest contour
		}
	}

	// Get Minimum bounding rotated rectangle and sort its corners clockwise
	RotatedRect rr = minAreaRect(contours[largest_contour_index]);
	Point2f rect_points[4];
	rr.points(rect_points);
	sortPointsClockwise(&(rect_points[0]));


	//Calculate homography from bounding rectangle to new image boundaries
	vector<Point2f> srcPts(4);
	vector<Point2f> dstPts(4);

	// Image points multiplied by 2 as it was scaled down by half
	srcPts[0] = rect_points[0] * 2;
	srcPts[1] = rect_points[1] * 2;
	srcPts[2] = rect_points[2] * 2;
	srcPts[3] = rect_points[3] * 2;

	// New size for card 
	Size outImgSize(800, 460);

	dstPts[0] = Point2f(0, 0);
	dstPts[1] = Point2f(outImgSize.width - 1, 0);
	dstPts[2] = Point2f(outImgSize.width - 1, outImgSize.height - 1);
	dstPts[3] = Point2f(0, outImgSize.height - 1);

	// Calculate the homography from the full resolution image
	Mat Homography = findHomography(srcPts, dstPts);
	warpPerspective(full, outImg, Homography, outImgSize, CV_INTER_CUBIC);

	getColor(outImg);
	//Make grayscale copy from new rectified image
	Mat gray(outImg.size(), CV_8UC1);
	cvtColor(outImg, gray, CV_BGR2GRAY);

	//Apply threshold (otsu)
	Mat thr;
	threshold(gray, thr, 0, 255, CV_THRESH_OTSU);

	// Try to remove small words and holes
	Mat tmp = thr.clone();
	erode(thr, thr, Mat::ones(Size(11, 11), CV_8UC1));

	// Now find the greatest 5 contours the area with numbers should be one of them
	vector<vector<Point> > contours2;
	vector<Vec4i> hierarchy2;
	findContours(thr, contours2, hierarchy2, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	int l_a[] = { 0, 0, 0, 0, 0 };
	int l_a_i[] = { 0, 0, 0, 0, 0 };

	for (int i = 0; i< contours2.size(); i++)
	{
		double a = fabs(contourArea(contours2[i]));  //  Find the area of contour
		for (int j = 0; j < 5; j++)
		{
			if (a > l_a[j])
			{
				l_a[j] = a;
				l_a_i[j] = i;
				break;
			}
		}

	}

	//try to choose one of the contour that is near the dimensions of the white area
	int chosen = -1;
	for (int i = 0; i < 5; i++)
	{
		if (l_a[i] < 80000 && l_a[i] > 25000)
		{
			chosen = i;
			break;
		}
	}

	//if not found give error
	if (chosen == -1)
	{
		RNG rng(12345);
		for (int i = 0; i < 5; i++)
		{
			rr = minAreaRect(contours2[l_a_i[i]]);

			Point2f rect_points2[4];
			rr.points(rect_points2);
			sortPointsClockwise(&(rect_points2[0]));


			dr(outImg, rect_points2, Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255)));
		}

		imshow("EROOOROROOROR", outImg);
		waitKey(0);
		return false;

	}


	// Get bounding rectangle for that contour
	rr = minAreaRect(contours2[l_a_i[chosen]]);

	Point2f rect_points2[4];
	rr.points(rect_points2);
	sortPointsClockwise(&(rect_points2[0]));



	//Calculate homography from bounding rectangle to new image boundaries
	// Image points
	srcPts[0] = rect_points2[0];
	srcPts[1] = rect_points2[1];
	srcPts[2] = rect_points2[2];
	srcPts[3] = rect_points2[3];

	// New size for white area 
	Size outImgSize2(800, 230);

	dstPts[0] = Point2f(0, 0);
	dstPts[1] = Point2f(outImgSize2.width - 1, 0);
	dstPts[2] = Point2f(outImgSize2.width - 1, outImgSize2.height - 1);
	dstPts[3] = Point2f(0, outImgSize2.height - 1);

	// Calculate the homography 
	Homography = findHomography(srcPts, dstPts);
	warpPerspective(outImg, tmp, Homography, outImgSize2, CV_INTER_CUBIC);
	cvtColor(tmp, outImg, CV_BGR2GRAY);;


	//Adaptive threshold the white area to get better numbers than using the OTSU produced
	// before
	int rows = outImg.rows;
	int cols = outImg.cols;
	adaptiveThreshold(outImg, outImg, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 35, 10);

	// inverse the image to make the numbers in white
	// And make morphological filling on object that interect with image boundary
	Mat inv_out = 255 - outImg;
	Mat dst = Mat::zeros(outImg.size(), CV_8U);
	Mat kernel = (Mat_<uchar>(3, 3) << 1, 1, 1, 1, 1, 1, 1, 1, 1);

	for (int i = 0; i < cols; i++){

		uchar clr = outImg.at<uchar>(0, i);
		if (clr == 0)
		{
			tst(outImg, inv_out, dst, kernel, Point(i, 0));
		}
		clr = outImg.at<uchar>(rows - 1, i);
		if (clr == 0)
		{
			tst(outImg, inv_out, dst, kernel, Point(i, rows - 1));
		}

	}

	for (int i = 0; i < rows; i++)
	{
		uchar clr = outImg.at<uchar>(i, 0);
		if (clr == 0)
		{
			tst(outImg, inv_out, dst, kernel, Point(0, i));
		}
		clr = outImg.at<uchar>(i, cols - 1);
		if (clr == 0)
		{
			tst(outImg, inv_out, dst, kernel, Point(cols - 1, i));
		}
	}



	// Close to remove small noise
	kernel = (Mat_<uchar>(3, 3) << 0, 1, 0, 1, 1, 1, 0, 1, 0);
	morphologyEx(outImg, outImg, MORPH_CLOSE, getStructuringElement(MORPH_RECT, Size(7, 7)));

	// Now thin the image to search for contours
	thinning(255 - outImg, outImg);


	// Remove contours with very small height
	{
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours(outImg.clone(), contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

		/// Get Largest Contour
		vector<int > selected;
		for (int i = 0; i < contours.size(); i++)
		{
			Rect_<double> t = boundingRect(contours[i]);
			if (t.height < 40)
				selected.push_back(i);


		}

		Mat g = Mat::zeros(outImg.size(), CV_8UC1);
		for (int i = 0; i < selected.size(); i++)
		{
			drawContours(g, contours, selected[i], 255, -1);
		}
		g = 255 - g;
		outImg &= g;


	}


	// Dilate the thinned numbers to be better read by OCR
	kernel = (Mat_<uchar>(3, 3) << 0, 1, 0, 1, 1, 1, 0, 1, 0);
	morphologyEx(outImg, outImg, MORPH_DILATE, kernel);
	morphologyEx(outImg, outImg, MORPH_DILATE, kernel);


	imshow("t", outImg);

	imwrite("im.tif", outImg);
	return true;


}


