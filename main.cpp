#include <opencv2\opencv.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <list>
#include <utility>
#include "module_match.h"

using namespace std;
using namespace cv;

Point center(const Rect2f &rect)
{
	const int Width = 1440;
	const int Height = 1080;
	Point p;
	p.x = (rect.x + rect.width/2.0)*Width;
	p.y = (rect.y + rect.height/2.0)*Height;
	return p;
}
struct HSVColor
{
	unsigned char h, s, v;
};
vector<HSVColor> generate_color_table()
{
	vector<HSVColor> table;
	int H[13] = { 180,120,60,160,100,40,150,90,30,140,80,20,10 };
	int S[3] = { 255,100,30 };
	int V[3] = { 255,180,90 };
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			for (int k = 0; k < 13; k++)
			{
				HSVColor c;
				c.h = H[k];
				c.s = H[j];
				c.v = H[i];
				table.push_back(c);
			}
		}
	}
	return table;	
}

void HSVtoRGB(unsigned char &r, unsigned char &g, unsigned char &b, int h, int s, int v)
{
	int i;

	float RGB_min, RGB_max;
	RGB_max = v * 2.55f;
	RGB_min = RGB_max * (100 - s) / 100.0f;

	i = h / 60;
	int difs = h % 60; // factorial part of h

	// RGB adjustment amount by hue 
	float RGB_Adj = (RGB_max - RGB_min)*difs / 60.0f;

	switch (i)
	{
	case 0:
		r = RGB_max;
		g = RGB_min + RGB_Adj;
		b = RGB_min;
		break;
	case 1:
		r = RGB_max - RGB_Adj;
		g = RGB_max;
		b = RGB_min;
		break;
	case 2:
		r = RGB_min;
		g = RGB_max;
		b = RGB_min + RGB_Adj;
		break;
	case 3:
		r = RGB_min;
		g = RGB_max - RGB_Adj;
		b = RGB_max;
		break;
	case 4:
		r = RGB_min + RGB_Adj;
		g = RGB_min;
		b = RGB_max;
		break;
	default:		// case 5:
		r = RGB_max;
		g = RGB_min;
		b = RGB_max - RGB_Adj;
		break;
	}
}

int main()
{
	size_t id = 3;
	string file_name{ "result_video" + to_string(id) + ".txt" };
	ifstream file(file_name);
	if (!file.is_open())
	{
		cout << "failed to open the file" << endl;
		return -1;
	}

	VideoCapture cap("test_" + to_string(id) + ".mp4");
	if (!cap.isOpened())
	{
		cout << "open video failed" << endl;
		return -2;
	}

	Size size(cap.get(CV_CAP_PROP_FRAME_WIDTH), cap.get(CV_CAP_PROP_FRAME_HEIGHT));
	cout << size << endl;
	VideoWriter writer("result_video" + to_string(id) + "_result.mp4",
		VideoWriter::fourcc('m', 'p', '4', 'v'),
		cap.get(CV_CAP_PROP_FPS), size);


	namedWindow("input", CV_WINDOW_NORMAL);
	namedWindow("result", CV_WINDOW_NORMAL);

	string line;
	vector<Rect2f>boxes;
	MatchPeopleModule m0;
	using MyVal = pair<int, list<Point>>;
	using MyMap = map<size_t, MyVal>;
	MyMap traces;

	vector<HSVColor> color_table = generate_color_table();

	size_t frame_id = 0;
	while (getline(file, line))
	{
		if (line[0] != '-')
		{
			stringstream s(line);
			float x, y, w, h;
			s >> x >> y >> w >> h;
			boxes.push_back(Rect2f(x - w/2.0, y - h / 2.0, w, h));
		}
		else
		{
			// one picture read done
			for (auto iter = traces.begin(); iter != traces.end(); ++iter)
			{
				iter->second.first = -1;
			}

			m0.update_module(boxes);
			vector<PeopleModule> res = m0.get_modules();
			for (int i = 0; i < res.size(); ++i)
			{
				//cout << "rect " << res[i].id() <<  ": " << res[i].get_rect() << endl;
				if (traces.find(res[i].id()) != traces.end())
				{
					traces[res[i].id()].first = res[i].id();
					traces[res[i].id()].second.push_back(center(res[i].get_rect()));
				}
				else
				{
					list<Point> tmp;
					tmp.insert(tmp.end(), center(res[i].get_rect()));
					traces.insert(pair<size_t, pair<int, list<Point>>>(res[i].id(), MyVal(res[i].id(), tmp)));
				}
			}

			//cout << endl;
			Mat frame;
			cap >> frame;
			if (frame.empty())
			{
				break;
			}
			imshow("input", frame);

			for (auto iter = traces.begin(); iter != traces.end(); )
			{
				if (iter->second.first == -1)
				{
					iter = traces.erase(iter);
				}
				else
				{
					++iter;
				}
			}
			for (auto iter = traces.begin(); iter != traces.end(); ++iter)
			{
				HSVColor color = color_table[iter->second.first % color_table.size()];
				unsigned char r, g, b;
				HSVtoRGB(r, g, b, color.h, color.s, color.v);
				list<int>a;
				auto last_p = iter->second.second.begin();
				auto p_iter = ++last_p;
				last_p--;
				for (; p_iter != iter->second.second.end(); ++p_iter, ++last_p)
				{
					cv::line(frame, *last_p, *p_iter, Scalar(b, g, r, 1), 3);
				}
			}

			imshow("result", frame);
			writer << frame;

			boxes.clear();
			frame_id++;
			waitKey(1);
		}
	}
	return 0;
}