#include <iostream>
#include <opencv2/opencv.hpp>

int main()
{
    std::string path = "D:/Realsense_L515/opencv/sources/samples/java/clojure/simple-sample/resources/images/lena.png";
    cv::Mat img = cv::imread(path);
    cv::imshow("img", img);
    cv::waitKey(0);

    return 0;
}
