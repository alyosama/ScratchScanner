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

void extract(string imageName)
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

	imshow("t", src);

	//Make gray scale copy of image for further processing
	cvtColor(src, src_gray, COLOR_BGR2GRAY);
	

	/// Detect edges using canny
	Mat canny_output;
	Canny(src_gray, canny_output, thresh, thresh * 3, 3);
	dilate(canny_output, canny_output, Mat());

	/// Find contours
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours(canny_output, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

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
	vector<Point2i> srcPts(4);
	vector<Point2i> dstPts(4);

	// Image points multiplied by 2 as it was scaled down by half
	srcPts[0] = rect_points[0] * 2;
	srcPts[1] = rect_points[1] * 2;
	srcPts[2] = rect_points[2] * 2;
	srcPts[3] = rect_points[3] * 2;

	// New size for card 
	Size outImgSize(1000, 600);

	dstPts[0] = Point2f(0, 0);
	dstPts[1] = Point2f(outImgSize.width - 1, 0);
	dstPts[2] = Point2f(outImgSize.width - 1, outImgSize.height - 1);
	dstPts[3] = Point2f(0, outImgSize.height - 1);

	// Calculate the homography from the full resolution image
	Mat Homography = findHomography(srcPts, dstPts);
	warpPerspective(full, outImg, Homography, outImgSize, INTER_CUBIC);

	//Make grayscale copy from new rectified image
	Mat gray(outImg.size(), CV_8UC1);
	cvtColor(outImg, gray, COLOR_BGR2GRAY);

	//Apply threshold (otsu)
	Mat thr;
	threshold(gray, thr, 0, 255, THRESH_OTSU);

	// Try to remove small words and holes
	Mat tmp = thr.clone();
	erode(thr, thr, Mat::ones(Size(11, 11), CV_8UC1));

	// Now find the greatest contour it should be the area with the numbers in it
	vector<vector<Point> > contours2;
	vector<Vec4i> hierarchy2;
	findContours(thr, contours2, hierarchy2, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));
	largest_area = 0;
	largest_contour_index = 0;
	for (int i = 0; i< contours2.size(); i++)
	{
		double a = fabs(contourArea(contours2[i]));  //  Find the area of contour
		if (a>largest_area){
			largest_area = (int)a;
			largest_contour_index = i;                //Store the index of largest contour
		}
	}
	// Get bounding rectangle for that contour
	rr = minAreaRect(contours2[largest_contour_index]);
	Point2f rect_points2[4];
	rr.points(rect_points2);
	sortPointsClockwise(&(rect_points2[0]));



	//Calculate homography from bounding rectangle to new image boundaries
	// Image points
	srcPts[0] = rect_points2[0];
	srcPts[1] = rect_points2[1];
	srcPts[2] = rect_points2[2];
	srcPts[3] = rect_points2[3];

	// New size for card 
	Size outImgSize2(500, 100);

	dstPts[0] = Point2f(0, 0);
	dstPts[1] = Point2f(outImgSize2.width - 1, 0);
	dstPts[2] = Point2f(outImgSize2.width - 1, outImgSize2.height - 1);
	dstPts[3] = Point2f(0, outImgSize2.height - 1);

	// Calculate the homography from the full resolution image
	Homography = findHomography(srcPts, dstPts);
	warpPerspective(tmp, outImg, Homography, outImgSize2, INTER_CUBIC);

	int offsetx = 20;
	int offsety = 10;
	Mat roi(outImg, Rect(offsetx, offsety, outImg.cols - offsetx, outImg.rows - offsety));


	morphologyEx(roi, roi, MORPH_ERODE, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)));
	
	//Show final image
	namedWindow(imageName + "processed", WINDOW_AUTOSIZE);
	imshow(imageName + "processed", roi);
	imwrite("out_" + imageName + ".tif", roi);

	waitKey(0);
}

