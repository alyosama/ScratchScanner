#include "helper.h"
#include <iostream>
#include <fstream>
using namespace std;

int main(int argc, char** argv)
{
	bool t = extract("images/3"); // algorithm 1
	if (t)
	{
		system("tesseract im.tif out nobatch digits");

		cout << "-------------------------\n\n";

		string line;
		ifstream myfile("out.txt");
		if (myfile.is_open())
		{
			while (getline(myfile, line))
			{
				cout << line << '\n';
			}
			myfile.close();
		}

		else cout << "Unable to open file";
		cout << "\n\n\n";
		cv::waitKey(0);
	}
	else
	{
		cout << "Couldn't find image";
	}
	
	return 0;
}

