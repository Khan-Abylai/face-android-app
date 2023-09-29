//
// Created by yeleussinova on 9/27/23.
//

#ifndef FACE_DET_MOBAPP_FACE_H
#define FACE_DET_MOBAPP_FACE_H
#include <opencv2/core.hpp>
#include <android/asset_manager.h>
#include <net.h>

struct Point{
    float _x;
    float _y;
};
struct bbox{
    float x1;
    float y1;
    float x2;
    float y2;
    float s;
    Point point[5];
};

struct box{
    float cx;
    float cy;
    float sx;
    float sy;
};

class FaceDetection
{
public:
    int loadModel(AAssetManager* mgr);

    void Detect(cv::Mat& bgr, std::vector<bbox>& boxes);

    bbox chooseOneFace(std::vector<bbox> &inpt_boxes, float scale);

    cv::Mat alignFace(cv::Mat img, bbox input_box);

    int draw(cv::Mat& rgb, bbox box, cv::Mat face);

private:
    const float iou = 0.4;
    const float _threshold = 0.75;
    const float _mean_val[3] = {104.f, 117.f, 123.f};
    ncnn::Net retina;
    const int outputHeight =112;
    const int outputWidth = 112;

    const bool default_square = true;
    const float inner_padding_factor = 0.25;
};


#endif //FACE_DET_MOBAPP_FACE_H
