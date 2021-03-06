/**
 * Author: Cyan
 * Date: Wed Dec  8 16:14:11 CST 2021
 */
#include "Image.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>

#include "cmg/message/codex.hpp"
#include "cmg/utils/log.hpp"
#include "sensor_msgs.pb.h"

namespace cmg { namespace sensor_msgs{

    using namespace google::protobuf;

    auto Image::serialize(std::ostream &out) const -> unsigned long {

        cmg_pb::Image msg;

        auto header = msg.mutable_header();
        header->set_stamp_sec(this->header.stamp.sec);
        header->set_stamp_nsec(this->header.stamp.nsec);
        header->set_frame_id(this->header.frame_id);

        msg.set_height(this->rows);
        msg.set_width(this->cols);
        msg.set_channels(this->channels);
        auto len = this->rows * this->cols * this->channels;
        msg.set_data(this->data.data(), len);

        std::string msg_data;

        if (!msg.SerializeToString(&msg_data)) {

            CMG_WARN("Image serialize failed\n");
            return 0;
        }

        //msg_data = Codex::encode64(msg_data);
//        msg_data = Zip::compress(msg_data);
        out.write(msg_data.data(), msg_data.length());

        return msg_data.length();
    }

    auto Image::parse(std::istream &in) -> unsigned long {

        cmg_pb::Image msg;

        std::stringstream &ss = dynamic_cast<std::stringstream&>(in);

        std::string msg_data = ss.str();
//        msg_data = Zip::decompress(msg_data);
        //msg_data = Codex::decode64(msg_data);

        if (!msg.ParseFromString(msg_data)) {

            CMG_WARN("Image parse failed\n");
            return 0;
        }

        this->header.stamp.sec = msg.header().stamp_sec();
        this->header.stamp.nsec = msg.header().stamp_nsec();
        this->header.frame_id = msg.header().frame_id();

        this->rows = msg.height();
        this->cols = msg.width();
        this->channels = msg.channels();
        auto *data = msg.mutable_data()->data();
        this->data.resize(this->rows * this->cols * this->channels);
        memcpy(this->data.data(), data, this->data.size());

        return msg_data.length();
    }


    Image::Image(int _rows, int _cols, int _channels, char *_data) :
        rows(_rows), cols(_cols), channels(_channels) {

        this->data.resize(this->rows * this->cols * this->channels);
        std::copy(_data, _data + this->data.size(), this->data.begin());
    }

    auto Image::setData(int rows, int cols, int channels, char *_data) -> int {

        this->rows = rows;
        this->cols = cols;
        this->channels = channels;

        this->data.resize(this->rows * this->cols * this->channels);
        std::copy(_data, _data + this->data.size(), this->data.begin());

        return this->data.size();
    }
}}
