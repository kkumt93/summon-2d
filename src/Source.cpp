#include "opencv2/opencv.hpp"
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/highgui/highgui.hpp"
#include<windows.h>
using namespace std;



int main(int argh, char* argv[])
{
	cv::VideoCapture cap(0);//デバイスのオープン
	
	if (!cap.isOpened())//カメラデバイスが正常にオープンしたか確認．
	{
		//読み込みに失敗したときの処理
		return -1;
	}

	cv::VideoCapture video("nora.mp4");//動画の読み込み

	cv::VideoWriter writer("Nora_AR.avi", CV_FOURCC_DEFAULT, 30, cv::Size(), true);//書き込み設定

	//オフセット変数
	int offsetx = 0;
	int offsety = 0;
	int red_x_1f = 0;
	int red_y_1f = 0;
	int blue_x_1f = 0;
	int blue_y_1f = 0;

	//スキャン変数
	int i, j;

	//後で消す
	int flag = 0;

	double sub_y       = 0.0;
	double sub_y_res   = 0.0;


	//Mat宣言
	cv::Mat video_f(720, 1280, CV_8UC3);
	cv::Mat cap_f(480, 640, CV_8UC3);
	cv::Mat base_f(720, 1280, CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Mat med_img1(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Mat med_img2(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Mat hsv_img;
	cv::Mat blue_b_img(480, 640, CV_8UC1);
	cv::Mat green_b_img(480, 640, CV_8UC1);
	cv::Mat red_b_img(480, 640, CV_8UC1);
	cv::Mat result_f(480, 640, CV_8UC3);

	while (1)//メイン処理
	{
		//ビデオ取得
		video >> video_f;//動画１フレーム読み出し

		//フレームが空か、ボタンが押された時か一周したときに出る。
		if (video_f.empty() || cv::waitKey(30) >= 0) {
			break;
		}

		//ねこリサイズ -> 640x360
		resize(video_f, video_f, cv::Size(), 0.5, 0.5);

		//ねこリサイズ -> 200x360
		offsetx = 220;
		offsety = 0;
		cv::Mat neko(360, 200, CV_8UC3);
		for (i = 0; i < neko.rows; i++) {
			for (j = 0; j < neko.cols; j++) {
				neko.at<cv::Vec3b>(i, j)[0] = video_f.at<cv::Vec3b>(i, j + offsetx)[0];
				neko.at<cv::Vec3b>(i, j)[1] = video_f.at<cv::Vec3b>(i, j + offsetx)[1];
				neko.at<cv::Vec3b>(i, j)[2] = video_f.at<cv::Vec3b>(i, j + offsetx)[2];
			}
		}

		//キャプチャ取得
		cap >> cap_f;

		//平滑化
		cv::medianBlur(cap_f, med_img1, 5);
		cv::medianBlur(med_img1, med_img1, 13);

		//2値化
		for (i = 0; i < med_img1.rows; i++) {
			for (j = 0; j < med_img1.cols; j++) {
				//赤マーカー
				if ((med_img1.at<cv::Vec3b>(i, j)[0] < 125 &&
					med_img1.at<cv::Vec3b>(i, j)[1] < 125 &&
					med_img1.at<cv::Vec3b>(i, j)[2] > 140)) {

					med_img2.at<cv::Vec3b>(i, j)[2] = 255;
				}
				else {

					med_img2.at<cv::Vec3b>(i, j)[2] = 0;
				}
			}
		}

		//座標算出
		int red_x_array[640] = {};
		int red_y_array[480] = {};
		int red_x = 0;
		int red_y = 0;
		int sum = 0;

		for (i = 0; i < 480; i++) {
			for (j = 0; j < 640; j++) {
				if (med_img2.at<cv::Vec3b>(i, j)[2] == 255) {
					red_y_array[i] = red_y_array[i] + 1;
					red_x_array[j] = red_x_array[j] + 1;
					sum++;
				}
			}
		}
		for (i = 0; i < 480; i++) {
			if (red_y_array[i] > red_y)
				red_y = i;
		}
		for (j = 0; j < 640; j++) {
			if (red_x_array[j] > red_x)
				red_x = j;
		}

		//printf("x=%d y=%d sum=%d\n", red_x, red_y, sum);
		if (flag == 0) {
			red_x_1f = red_x;
			red_y_1f = red_y;
			flag++;
		}
		if (abs(red_x - red_x_1f) > 50 || abs(red_y - red_y_1f) > 50) {
			red_x = red_x_1f;
			red_y = red_y_1f;
		}

		red_x_1f = red_x;
		red_y_1f = red_y;

		sub_y_res = sub_y_res*0.8 + sum*0.2;

		//ねこリサイズ -> ???
		double k = 0.5 + (double)(sub_y_res-100)*0.0003;
		resize(neko, neko, cv::Size(), k, k);

		printf("%f\n", k);
		
		//base_f + cap_f 合成
		offsetx = 320;
		offsety = 120;
		for (i = 0; i < cap_f.rows; i++) {
			for (j = 0; j < cap_f.cols; j++) {
				base_f.at<cv::Vec3b>(i + offsety, j + offsetx)[0] = cap_f.at<cv::Vec3b>(i, j)[0];
				base_f.at<cv::Vec3b>(i + offsety, j + offsetx)[1] = cap_f.at<cv::Vec3b>(i, j)[1];
				base_f.at<cv::Vec3b>(i + offsety, j + offsetx)[2] = cap_f.at<cv::Vec3b>(i, j)[2];
			}
		}

		//base_f + neko 合成
		offsetx = 320 + red_x - neko.cols / 2;
		offsety = 120 + red_y - neko.rows;
		for (i = 0; i < neko.rows; i++) {
			for (j = 0; j < neko.cols; j++) {
				if (!(neko.at<cv::Vec3b>(i, j)[0] > 165 &&
					neko.at<cv::Vec3b>(i, j)[1] < 135 &&
					neko.at<cv::Vec3b>(i, j)[2] < 135)) {

					base_f.at<cv::Vec3b>(i + offsety, j + offsetx)[0] = neko.at<cv::Vec3b>(i, j)[0];
					base_f.at<cv::Vec3b>(i + offsety, j + offsetx)[1] = neko.at<cv::Vec3b>(i, j)[1];
					base_f.at<cv::Vec3b>(i + offsety, j + offsetx)[2] = neko.at<cv::Vec3b>(i, j)[2];
				}
			}
		}

		offsetx = 320;
		offsety = 120;
		for (i = 0; i < result_f.rows; i++) {
			for (j = 0; j < result_f.cols; j++) {
				result_f.at<cv::Vec3b>(i, j)[0] = base_f.at<cv::Vec3b>(i + offsety, j + offsetx)[0];
				result_f.at<cv::Vec3b>(i, j)[1] = base_f.at<cv::Vec3b>(i + offsety, j + offsetx)[1];
				result_f.at<cv::Vec3b>(i, j)[2] = base_f.at<cv::Vec3b>(i + offsety, j + offsetx)[2];
			}
		}


		cv::imshow("med_img2", med_img2);//画像を表示
		cv::imshow("base_f", base_f);//画像を表示
		cv::imshow("result_f", result_f);//画像を表示
		writer << result_f;//動画書き込み
	}

	cv::destroyAllWindows();//window開放
	return 0;

}

