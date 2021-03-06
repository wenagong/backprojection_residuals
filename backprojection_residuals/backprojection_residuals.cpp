// backprojection_residuals.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include<pcl/io/pcd_io.h>
#include<pcl/point_types.h>
#include<opencv.hpp>
#include<pcl/visualization/cloud_viewer.h>

using namespace std;
using namespace cv;

#define Lcx 670.892	           
#define Lcy 473.301    //左相机的光心位置

#define fx1 3357.59	
#define fy1 3359.12	   //左相机的焦距

#define La1 -0.0588861	
#define La2 0.578368	
#define La3 0.000882497		
#define La4 -0.000283374   //左相机的四个畸变参数

//#define Lcx 665.408	           
//#define Lcy 462.975    //右相机
//
//#define fx1 3354.72	
//#define fy1 3355.86	   
//
//#define La1 -0.0531101	
//#define La2 0.517685
//#define La3 0.000446413		
//#define La4 0.00139215  


struct MyPointType
{
	PCL_ADD_POINT4D
	double p_res;  //反投影残差
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
}EIGEN_ALIGEN16;

POINT_CLOUD_REGISTER_POINT_STRUCT(MyPointType, //注册点类型宏
(double, x, x)
(double, y, y)
(double, z, z)
(double, p_res, p_res)
)

int main()
{

	//相机参数
	double m1[3][3] = { { fx1,0,Lcx },{ 0,fy1,Lcy },{ 0,0,1 } };
	Mat m1_matrix(Size(3, 3), CV_64F, m1);  //左相机的内参矩阵

	//double r[3][1] = { { -0.0150308},{ 0.0837883 },{ -0.0479945 } };
 //   Mat R(Size(1, 3), CV_64F, r); //旋转向量

	//Mat T = (Mat_<double>(3, 1) << -93.0651, 3.0822, -3.78523);   //平移向量
	double r[3][3] = { { 1,0,0 },{ 0,1,0 },{ 0,0,1 } };
	Mat R1(3, 3, CV_64F, r); //旋转矩阵
	Mat R(3, 1, CV_64F); //旋转向量
	Rodrigues(R1, R);
	Mat T = (Mat_<double>(3, 1) << 0, 0, 0);   //平移向量

	Mat dist1 = (Mat_<double>(4, 1) << La1, La2, La3, La4); //左相机畸变参数(k1,k2,p1,p2,k3)

	ifstream TD_filename, matcher_filename;
	TD_filename.open("Result3D_Points.TXT", ios::in);
	matcher_filename.open("MatchResult2D_LeftPoints.TXT", ios::in);
	//matcher_filename.open("MatchResult2D_RightPoints.TXT", ios::in);

	ofstream dev_txt;
	dev_txt.open("dev_datas.txt");

 //   //验证实验
	//ifstream TD_filename, matcher_filename;
	//TD_filename.open("tem\\datas_3d.txt", ios::in);
	//matcher_filename.open("tem\\datas_2d.txt", ios::in);

	//ofstream dev_txt;
	//dev_txt.open("tem\\lab_datas.txt");

	//double m_tem[3][3] = { { 1,0,1 },{ 0,1,1 },{ 0,0,1 } };
	//Mat m1_matrix(Size(3, 3), CV_64F, m_tem);  //左相机的内参矩阵

	//double r[3][3] = { { 1,0,0 },{ 0,1,0 },{ 0,0,1 } };
	//Mat R1(3, 3, CV_64F, r); //旋转矩阵
	//Mat R(3, 1, CV_64F); //旋转向量
	//Rodrigues(R1, R);
	//Mat T = (Mat_<double>(3, 1) << 0, 0, 0);   //平移向量
	//Mat dist1 = (Mat_<double>(4, 1) << 0, 0, 0, 0); //左相机畸变参数(k1,k2,p1,p2,k3)

	
	if (!TD_filename.is_open() || !matcher_filename.is_open()) {
		cout << "open file failed!" << endl;
	}

	int total_num=-1;
	char str[200];
	while (!TD_filename.eof()) {
		TD_filename.getline(str, sizeof(str));
		total_num++;
	}

	TD_filename.clear();
	TD_filename.seekg(0,ios::beg);

	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
	cloud->width = total_num;
	cloud->height = 1;
	cloud->is_dense = false;
	cloud->resize(cloud->width*cloud->height);

	pcl::PointCloud<pcl::PointXYZRGB>::Ptr m_cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
	m_cloud->width=total_num;
	m_cloud->height = 1;
	m_cloud->is_dense = false;
	m_cloud->resize(m_cloud->width*m_cloud->height);

	pcl::PointCloud<MyPointType>::Ptr p_cloud(new pcl::PointCloud<MyPointType>);
	p_cloud->width = total_num;
	p_cloud->height = 1;
	p_cloud->is_dense = false;
	p_cloud->resize(p_cloud->width*p_cloud->height);


	double dev_min=INT_MAX;
	double dev_max=INT_MIN;

	for (int i = 0; i < total_num; i++) {
		
		double num[3];
		//匹配点坐标
		double match[2];

		//三维点坐标
		vector<Point3d>coord_points;

		TD_filename >> num[0]; 
		TD_filename >> num[1];
		TD_filename >> num[2];

		matcher_filename >> match[0];
		matcher_filename >> match[1];

		//匹配点二维坐标(像素坐标)
		vector<Point2d>match_2d;
		Mat tem1 = (Mat_<double>(2, 1) << (match[0]-Lcx)/fx1, (match[1]-Lcy)/fy1);
		match_2d = Mat_<Point2d>(tem1);

		Mat cord = (Mat_<double>(3, 1) << num[0] , num[1] , num[2]);
		coord_points = Mat_<Point3d>(cord);

		////透视投影
		//Mat res_1(3,1, CV_64F);
		//res_1 = m1_matrix * cord/num[2];
		//double res1_x = res_1.at<double>(0, 0);// / res_1.at<double>(2, 0);
		//double res1_y = res_1.at<double>(1, 0);// / res_1.at<double>(2, 0);
		//Mat res_2 = (Mat_<double>(2, 1) << res1_x, res1_y);
		//res_2d = Mat_<Point2d>(res_2);

		//生成的重投影二维坐标
		vector<Point2d>res_2d;

		//计算重投影坐标（输出的是图像坐标）
		projectPoints(coord_points, R, T, m1_matrix, dist1, res_2d);

		res_2d[0].x = (res_2d[0].x - Lcx) / fx1;
		res_2d[0].y = (res_2d[0].y - Lcy) / fy1;

		//重投影误差大小
		double dev;
		dev = norm(res_2d,match_2d, CV_L2);

		//dev= pow(res_2d.at(0).x - match[0], 2) + pow(res_2d.at(0).y - match[1], 2);

		if (dev < dev_min) dev_min = dev;
		else if (dev >= dev_max) dev_max = dev;

		dev_txt << "coord_3D=" << coord_points << " ; " << " res_2d=" << res_2d << " ; " << "matcher=" << match_2d << " ; " << "dev=" << dev <<endl;  //输出偏差值
		
		cloud->points[i].x = num[0];
		cloud->points[i].y = num[1];
		cloud->points[i].z = num[2];

		m_cloud->points[i].x = num[0];
		m_cloud->points[i].y = num[1];
		m_cloud->points[i].z = num[2];
		
		p_cloud->points[i].x = num[0];
		p_cloud->points[i].y = num[1];
		p_cloud->points[i].z = num[2];
		p_cloud->points[i].p_res = dev;

		////测试语句，添加不同颜色
		//if (i < 100000) {
		//	m_cloud->points[i].r = 0;
		//	m_cloud->points[i].g = 255;
		//	m_cloud->points[i].b = 0;
		//}
		//else {
		//	m_cloud->points[i].r = 255;
		//	m_cloud->points[i].g = 0;
		//	m_cloud->points[i].b = 0;

		//}
	}

	dev_txt << "dev_max:" << dev_max << " ; " << "dev_min:" << dev_min << endl;

	double interval = (dev_max - dev_min) / 10;

	for (size_t j = 0; j < total_num; j++) {

		double temp = p_cloud->points[j].p_res;  //取出求得的残杀大小，并进行分类赋色
		
		if (9 * interval + dev_min < temp && temp <= dev_max) { 
			m_cloud->points[j].r = 0;
			m_cloud->points[j].g = 0;
			m_cloud->points[j].b = 0;
		}
		//else {
		//	m_cloud->points[j].r = 255;
		//	m_cloud->points[j].g = 255;
		//	m_cloud->points[j].b = 255;

		//}
		if (4 * interval + dev_min < temp && temp <= dev_max) { //第五等：白色
			m_cloud->points[j].r = 255;   
			m_cloud->points[j].g = 255;
			m_cloud->points[j].b = 255;
		}
		if (3 * interval + dev_min < temp && temp <= 4 * interval + dev_min) { //第四等：黄色
			m_cloud->points[j].r = 255;
			m_cloud->points[j].g = 255;
			m_cloud->points[j].b = 0;
		}
		if (2 * interval + dev_min < temp && temp <= 3 * interval + dev_min) { //第三等：绿色
			m_cloud->points[j].r = 0;
			m_cloud->points[j].g = 255;
			m_cloud->points[j].b = 0;
		}
		if (interval + dev_min < temp && temp <= 2 * interval + dev_min) { //第二等：黑 色
			m_cloud->points[j].r = 0;
			m_cloud->points[j].g = 0;
			m_cloud->points[j].b = 0;
		}
		else { //第一等：红色
			m_cloud->points[j].r = 255;
			m_cloud->points[j].g = 0;
			m_cloud->points[j].b = 0;
		}
	}

	pcl::io::savePCDFileASCII("cloud.pcd", *cloud);
	pcl::io::savePCDFileASCII("cloud_color.pcd", *m_cloud);
	
	boost::shared_ptr<pcl::visualization::PCLVisualizer>viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
	viewer->initCameraParameters();

	int v1(0);
	//createViewPort是用于创建新视口的函数，所需的4个参数分别是视口在X轴的最小值，Y轴的最小值，X的最大值，Y的最大值，取值在0-1.
	viewer->createViewPort(0.0, 0.0, 0.5, 1.0, v1);
	viewer->setBackgroundColor(0, 0, 0, v1);
	//添加各视窗文字介绍
	viewer->addText("original_datas", 10, 10, "v1 text", v1);
	//设置点云颜色
	//pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> source_color(cloud, 0, 255, 0)
	viewer->addPointCloud<pcl::PointXYZ>(cloud,"black", v1);

	int v2(0);
	viewer->createViewPort(0.5, 0.0, 1.0, 1.0, v2);
	viewer->setBackgroundColor(0.3, 0.3, 0.3, v2);
	viewer->addText("color_datas", 10, 10, "v2 text", v2);
	//pcl::visualization::PointCloudColorHandlerRGBField<pcl::PointXYZRGB> rgb(m_cloud);
	viewer->addPointCloud<pcl::PointXYZRGB>(m_cloud,"color", v2);

	viewer->spin();//可旋转

	TD_filename, matcher_filename, dev_txt.close();

    return 0;
}

