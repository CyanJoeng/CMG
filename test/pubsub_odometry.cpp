/**
 * Author: Cyan
 * Date: Wed Dec  8 16:33:46 CST 2021
 */
#include <cstdio>
#include <cstdlib>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <condition_variable>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <opencv2/core.hpp>
#include <opencv2/core/eigen.hpp>
#include <opencv2/viz.hpp>

#include "cmg/cmg.hpp"
#include "messages/nav_msgs/Odometry.hpp"

#include "arg_parse.hpp"

using namespace cmg;

using Widget = cv::viz::WPolyLine;
using WidgetPtr = std::shared_ptr<Widget>;

static constexpr auto name_odo = "odometry";


auto create_odo(int id) -> cmg::nav_msgs::Odometry {

	return {};
}

auto draw_odo_3d(const cmg::nav_msgs::OdometryConstPtr &odometry) -> std::pair<WidgetPtr, cv::Affine3d> {

	static std::vector<cv::Point3d> hist_points;
	auto &pos = odometry->pose.position;
	auto &ori = odometry->pose.orientation;

	hist_points.emplace_back(pos.x, pos.y, pos.z);

	auto polyline = std::make_shared<Widget>(hist_points);

	cv::Affine3d pose;
	cv::Mat rot;
	Eigen::Matrix3d m = Eigen::Quaterniond(ori.w, ori.x, ori.y, ori.z).matrix();
	cv::eigen2cv(m, rot);

	pose.rotation(rot);
	pose.translation(cv::Vec3d(pos.x, pos.y, pos.z));

	return {polyline, pose};
}

static std::optional<cv::viz::Viz3d> window;
static WidgetPtr widget_odo;
static cv::Affine3d cam_pose;

static std::mutex widget_mt;
static std::condition_variable widget_cv;
static std::thread work_loop;

void ui_refresh() {

	window.emplace(name_odo);
	window->showWidget("Coord", cv::viz::WCoordinateSystem(10.));

	cv::Matx33f K(700, 0, 320, 0, 700, 240, 0, 0);
	auto cam = cv::viz::WCameraPosition(K,  10);

	while (true) {

		{
			std::unique_lock<std::mutex> lck(widget_mt);
			widget_cv.wait(lck, []() {
					return widget_odo;
					});

			window->showWidget(name_odo, *widget_odo);
			window->showWidget("Cam", cam, cam_pose);
			lck.unlock();
		}

		window->spinOnce();
	}
}

static bool save_img = false;

auto cb_odo(const cmg::nav_msgs::OdometryConstPtr &odometry) {

	auto &position = odometry->pose.position;
	printf("cb odometry stamp %f (%10.4f %10.4f %10.4f)\n", odometry->header.stamp.toSec(),
			position.x, position.y, position.z);

	auto [odo, pose] = draw_odo_3d(odometry);
	{
		std::lock_guard<std::mutex> lock(widget_mt);
		widget_odo.swap(odo);
		cam_pose = pose;
	}
	widget_cv.notify_all();
}

int main(int argc, char *argv[]) {

	std::string server_proc_name = "server";
	std::string client_proc_name = "client";

	auto args = args_parser(argc, argv);

	char mode = args["mode"].as<char>();
	std::string proc = args["proc"].as<std::string>();
	std::string topic = args["topic"].as<std::string>();
	std::string cfg = args["cfg"].as<std::string>();

	const char *proc_args[] = {
		argv[0],
		cfg.data()
	};

	if (mode == 's') {

		cmg::init(2, proc_args, server_proc_name.c_str());

		cmg::NodeHandle n("~");

		auto pub_odo = n.advertise<nav_msgs::Odometry>("odometry", 1000);

		for (auto i = 0; i >= 0; ++i) {

			auto msg_odo = create_odo(i);

			pub_odo.publish(msg_odo);
			printf("pub odometry\n");
			std::this_thread::sleep_for(std::chrono::duration<double>(.5));
		}

		cmg::spin();

	} else if (mode == 'c') {

		cmg::init(2, proc_args, client_proc_name.c_str());

		cmg::NodeHandle n("~");

		std::string proc_topic = "/" + proc + "/" + topic;

		work_loop = std::thread {[&]() {

			auto sub = n.subscribe(proc_topic, 1000, cb_odo);
			cmg::spin();
		}};
		ui_refresh();
	}

	return 0;
}
