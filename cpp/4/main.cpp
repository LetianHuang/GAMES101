#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>

std::vector<cv::Point2f> control_points;

void mouse_handler(int event, int x, int y, int flags, void *userdata)
{
    if (event == cv::EVENT_LBUTTONDOWN && control_points.size() < 4)
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", "
                  << y << ")" << '\n';
        control_points.emplace_back(x, y);
    }
}

void shading(cv::Mat& window, float _y, float _x, const cv::Vec3b& color, float anti=2.0)
{
    /**
     * if anti != 0, use antialiasing
     */
    float _c0 = color[0];
    float _c1 = color[1];
    float _c2 = color[2];

    std::set<std::pair<int, int>> st;

    for (float y = _y - anti; y <= _y + anti; ++y)
    {
        for (float x = _x - anti; x <= _x + anti; ++x)
        {
            if (st.count({int(x), int(y)})) 
                continue;
            st.emplace(int(x), int(y));
            float dis = (y - _y) * (y - _y) + (x - _x) * (x - _x);
            dis = std::sqrt(dis);
            float exp_dis = std::exp(dis);
            float c0 = _c0 / exp_dis;
            float c1 = _c1 / exp_dis;
            float c2 = _c2 / exp_dis;
            window.at<cv::Vec3b>(y, x) += cv::Vec3b(c0, c1, c2);
        }
    }
}

void naive_bezier(const std::vector<cv::Point2f> &points, cv::Mat &window)
{
    auto &p_0 = points[0];
    auto &p_1 = points[1];
    auto &p_2 = points[2];
    auto &p_3 = points[3];

    for (double t = 0.0; t <= 1.0; t += 0.001)
    {
        auto point = std::pow(1 - t, 3) * p_0 + 3 * t * std::pow(1 - t, 2) * p_1 +
                     3 * std::pow(t, 2) * (1 - t) * p_2 + std::pow(t, 3) * p_3;


        shading(window, point.y, point.x, {0, 0, 255});
    }
}

inline cv::Point2f lerp2f(const cv::Point2f &a, const cv::Point2f &b, float t)
{
    return a + (b - a) * t;
}

cv::Point2f recursive_bezier(const std::vector<cv::Point2f> &control_points, float t)
{
    if (control_points.size() == 1)
    {
        return control_points.back();
    }

    std::vector<cv::Point2f> lerp_points;
    for (size_t i = 1; i < control_points.size(); i++)
    {
        lerp_points.push_back(lerp2f(control_points[i - 1], control_points[i], t));
    }

    return recursive_bezier(lerp_points, t);
}

void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window)
{
    // TODO: Iterate through all t = 0 to t = 1 with small steps, and call de Casteljau's
    // recursive Bezier algorithm.
    for (double t = 0.0; t <= 1.0; t += 0.001)
    {
        auto point = recursive_bezier(control_points, t);

        shading(window, point.y, point.x, {0, 255, 0});
    }
}

int main()
{
    cv::Mat window = cv::Mat(700, 700, CV_8UC3, cv::Scalar(0));
    cv::cvtColor(window, window, cv::COLOR_BGR2RGB);
    cv::namedWindow("Bezier Curve", cv::WINDOW_AUTOSIZE);

    cv::setMouseCallback("Bezier Curve", mouse_handler, nullptr);

    int key = -1;
    while (key != 27)
    {
        for (auto &point : control_points)
        {
            cv::circle(window, point, 3, {255, 255, 255}, 3);
        }

        if (control_points.size() == 4)
        {
            naive_bezier(control_points, window);
            bezier(control_points, window);

            cv::imshow("Bezier Curve", window);
            cv::imwrite("my_bezier_curve.png", window);
            key = cv::waitKey(0);

            return 0;
        }

        cv::imshow("Bezier Curve", window);
        key = cv::waitKey(20);
    }

    return 0;
}
