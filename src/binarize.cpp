#include <opencv2/opencv.hpp>

#include <iostream>

namespace {

void method_1(const cv::Mat& source_image, cv::Mat& dest_image){
    cv::Mat gray_image;
    cv::cvtColor(source_image, gray_image, CV_RGB2GRAY);

    cv::threshold(gray_image, dest_image, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
}

void method_2(const cv::Mat& source_image, cv::Mat& dest_image){
    cv::Mat gray_image;
    cv::cvtColor(source_image, gray_image, CV_RGB2GRAY);

    cv::Mat blurred_image;
    cv::GaussianBlur(gray_image, blurred_image, cv::Size(5,5), 0, 0);

    cv::threshold(blurred_image, dest_image, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
}

void method_3(const cv::Mat& source_image, cv::Mat& dest_image){
    cv::Mat gray_image;
    cv::cvtColor(source_image, gray_image, CV_RGB2GRAY);

    cv::adaptiveThreshold(gray_image, dest_image, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 11, 2);
}

void method_4(const cv::Mat& source_image, cv::Mat& dest_image){
    cv::Mat gray_image;
    cv::cvtColor(source_image, gray_image, CV_RGB2GRAY);

    cv::Mat blurred_image;
    cv::medianBlur(gray_image, blurred_image, 3);

    cv::adaptiveThreshold(blurred_image, dest_image, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 11, 2);
}

void hough(const cv::Mat& source_image, cv::Mat& dest_image){
    cv::Mat binary_image;
    method_4(source_image, binary_image);

    cv::Mat lines_image;
    cv::Canny(binary_image, lines_image, 50, 200, 3);

    std::vector<cv::Vec2f> lines;
    HoughLines(lines_image, lines, 1, CV_PI/180, 125, 0, 0 );

    dest_image = source_image.clone();

    for(size_t i = 0; i < lines.size(); i++){
        auto rho = lines[i][0];
        auto theta = lines[i][1];
        double a = cos(theta);
        double b = sin(theta);
        double x0 = a*rho;
        double y0 = b*rho;

        cv::Point pt1(cvRound(x0 + 1000 * -b), cvRound(y0 + 1000 * a));
        cv::Point pt2(cvRound(x0 - 1000 * -b), cvRound(y0 - 1000 * a));

        cv::line(dest_image, pt1, pt2, cv::Scalar(0,0,255), 3, CV_AA);
    }
}

void probabilistic_hough(const cv::Mat& source_image, cv::Mat& dest_image){
    cv::Mat binary_image;
    method_4(source_image, binary_image);

    cv::Mat lines_image;
    cv::Canny(binary_image, lines_image, 50, 200, 3);

    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(lines_image, lines, 1, CV_PI/360, 50, 100, 10 );

    dest_image = source_image.clone();

    for(auto& l : lines){
        cv::line( dest_image, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0,0,255), 3, CV_AA);
    }
}

double distance(const cv::Vec2f& l1, const cv::Vec2f& l2){
    double a = cos(l1[1]);
    double b = sin(l1[1]);
    double c1 = l1[0];
    double c2 = l2[0];

    return abs(c2 - c1) / sqrt(a * a + b * b);
}

void draw_line(cv::Mat& dest_image, const cv::Vec2f& line){
    auto rho = line[0];
    auto theta = line[1];

    double a = cos(theta);
    double b = sin(theta);
    double x0 = a*rho;
    double y0 = b*rho;

    cv::Point pt1(cvRound(x0 + 1000 * -b), cvRound(y0 + 1000 * a));
    cv::Point pt2(cvRound(x0 - 1000 * -b), cvRound(y0 - 1000 * a));

    cv::line(dest_image, pt1, pt2, cv::Scalar(0,0,255), 2, CV_AA);
}

double average_distance(std::vector<cv::Vec2f>& group, std::vector<std::vector<size_t>>& distance_groups){
    double average = 0;
    for(size_t i = 0 ; i < distance_groups.size() - 1; ++i){
        auto d = distance(group[distance_groups[i][0]], group[distance_groups[i+1][0]]);

        average += d;
    }

    average /= distance_groups.size();
    return average;
}

void sudoku_lines(const cv::Mat& source_image, cv::Mat& dest_image){
    dest_image = source_image.clone();

    cv::Mat binary_image;
    method_3(source_image, binary_image);

    constexpr const size_t CANNY_THRESHOLD = 50;
    cv::Canny(binary_image, binary_image, CANNY_THRESHOLD, CANNY_THRESHOLD * 3, 3);

    std::vector<cv::Vec2f> lines;
    HoughLines(binary_image, lines, 1, CV_PI/180, 125, 0, 0 );

    for(auto& line : lines){
        draw_line(dest_image, line);
    }

    return;

    constexpr const size_t PARALLEL_RESOLUTION = 10;
    constexpr const size_t BUCKETS = 180 / PARALLEL_RESOLUTION;

    constexpr const size_t DISTANCE_RESOLUTION = 5;

    std::vector<std::vector<cv::Vec2f>> groups(BUCKETS);

    for(auto& line : lines){
        auto theta = line[1];

        auto theta_deg = static_cast<size_t>(theta * (180 / CV_PI)) % 360;

        auto group = theta_deg / PARALLEL_RESOLUTION;
        groups[group].push_back(line);

        printf("theta:%f theta_deg=%ld group:%ld\n", theta, theta_deg, group);
    }

    std::copy(groups.back().begin(), groups.back().end(), std::back_inserter(groups.front()));
    groups.back().clear();

    for(size_t i = 0; i < groups.size(); ++i){
        auto& group = groups[i];

        if(group.size() < 9){
            continue;
        }

        std::cout << "group(" << i << "), size=" << group.size() << std::endl;

        auto angle_first = i * PARALLEL_RESOLUTION;
        auto angle_last = angle_first + PARALLEL_RESOLUTION - 1;

        std::vector<std::vector<size_t>> distance_groups;

        for(size_t i = 0 ; i < group.size(); ++i){
            auto& line = group[i];

            bool found = false;
            for(auto& distance_group : distance_groups){
                for(auto j : distance_group){
                    if(distance(line, group[j]) < 10){
                        distance_group.push_back(i);
                        found = true;
                        break;
                    }
                }

                if(found){
                    break;
                }
            }

            if(!found){
                distance_groups.emplace_back();
                distance_groups.back().push_back(i);
            }
        }

        size_t ei = 0;
        size_t ej = 0;
        size_t max_d = 0;

        for(size_t i = 0 ; i < distance_groups.size() - 1; ++i){
            for(size_t j = i ; j < distance_groups.size() - 1; ++j){
                auto d = distance(group[distance_groups[i][0]], group[distance_groups[j][0]]);
                if(d > max_d){
                    ei = i;
                    ej = j;
                    max_d = d;
                }
            }
        }

        auto extreme = group[distance_groups[ei][0]];
        std::sort(distance_groups.begin(), distance_groups.end(), [&group,&extreme](auto& lhs, auto& rhs){
            auto d1 = distance(extreme, group[lhs[0]]);
            auto d2 = distance(extreme, group[rhs[0]]);

            return d1 > d2;
        });

        while(true){
            double average = average_distance(group, distance_groups);

            size_t max = 0;
            size_t max_i = 0;

            for(size_t i = 0 ; i < distance_groups.size() - 1; ++i){
                auto d = distance(group[distance_groups[i][0]], group[distance_groups[i+1][0]]);
                if(d > max){
                    max_i = i;
                    max = d;
                }
            }

            std::cout << "average " << average << std::endl;
            std::cout << "max " << max << std::endl;

            if(max > average * 1.20){
                if(max_i == 0){
                    distance_groups.erase(distance_groups.begin() + max_i);
                    continue;
                }

                if(max_i+1 == distance_groups.size() - 1){
                    distance_groups.erase(distance_groups.begin() + max_i + 1);
                    continue;
                }
            }

            double min = 1111111111;
            size_t min_i = 0;

            for(size_t i = 0 ; i < distance_groups.size() - 1; ++i){
                auto d = distance(group[distance_groups[i][0]], group[distance_groups[i+1][0]]);
                if(d < min){
                    min_i = i;
                    min = d;
                }
            }

            std::cout << "average " << average << std::endl;
            std::cout << "min " << min << std::endl;

            if(min < average * 0.80){
                if(min_i == 0){
                    distance_groups.erase(distance_groups.begin() + min_i);
                    continue;
                }

                if(min_i+1 == distance_groups.size() - 1){
                    distance_groups.erase(distance_groups.begin() + min_i + 1);
                    continue;
                }
            }

            break;
        }

        for(auto& distance_group : distance_groups){
            draw_line(dest_image, group[distance_group[0]]);
        }

    /*    auto it = distance_groups.begin();
        auto end = distance_groups.end();

        while(it != end){
            if(it == distance_groups.begin()){
                auto right = it + 1;
                if(right != end){
                    auto d = distance(group[(*it)[0]], group[(*right)[0]]);

                    std::cout << "distance from right " << d << std::endl;

                    if(d < average * 0.90 || d > average * 1.10){
                        distance_groups.erase(it);

                        it = distance_groups.begin();
                        end = distance_groups.end();
                        average = average_distance(group, distance_groups);
                        std::cout << "average " << average << std::endl;
                        continue;
                    }
                }
            }

            ++it;
        }*/
    }

    /*for(size_t i = 0; i < groups.size(); ++i){
        auto& group = groups[i];

        if(group.size() < 9){
            continue;
        }

        auto angle_first = i * PARALLEL_RESOLUTION;
        auto angle_last = angle_first + PARALLEL_RESOLUTION - 1;

        //TODO TEMPORARY
        if(angle_first < 90){
            continue;
        }

        std::cout << "size[" << angle_first << "," << angle_last << "]=" << group.size() << std::endl;

        double max_d = 0;
        for(size_t i = 0 ; i < group.size() - 1; ++i){
            for(size_t j = i + 1; j < group.size(); ++j){
                max_d = std::max(distance(group[i],group[j]), max_d);
            }
        }

        auto buckets = static_cast<size_t>(max_d) / DISTANCE_RESOLUTION + 1;
        std::vector<std::vector<std::pair<size_t, size_t>>> pairs(buckets);

        for(size_t i = 0 ; i < group.size() - 1; ++i){
            for(size_t j = i + 1; j < group.size(); ++j){
                auto d = distance(group[i], group[j]);

                pairs[d / DISTANCE_RESOLUTION].emplace_back(i, j);
            }
        }

        auto min_tot = std::min(source_image.rows, source_image.cols);

        for(size_t i = 0 ; i < pairs.size() - 1; ++i){
            auto& pair_group = pairs[i];

            auto d_first = i * DISTANCE_RESOLUTION;
            auto d_last = d_first + DISTANCE_RESOLUTION - 1;

            if(pair_group.size() < 9){
                continue;
            }

            if(d_last < min_tot / 25){
                continue;
            }

            std::cout << i << std::endl;

            std::cout << "pair_group[" << d_first << ", " << d_last << "].size()=" << pair_group.size() << std::endl;

            for(auto& pair : pair_group){
                draw_line(dest_image, group[pair.first]);
                draw_line(dest_image, group[pair.second]);
            }
        }
    }*/
}

} //end of anonymous namespace

int main(int argc, char** argv ){
    if(argc < 2){
        std::cout << "Usage: binarize <image>..." << std::endl;
        return -1;
    }

    if(argc == 2){
        std::string source_path(argv[1]);

        cv::Mat source_image;
        source_image = cv::imread(source_path.c_str(), 1);

        if (!source_image.data){
            std::cout << "Invalid source_image" << std::endl;
            return -1;
        }

        cv::Mat dest_image;
        probabilistic_hough(source_image, dest_image);

        cv::namedWindow("Sudoku Grid", cv::WINDOW_AUTOSIZE);
        cv::imshow("Sudoku Grid", dest_image);

        cv::waitKey(0);
    } else {
        for(size_t i = 1; i < argc; ++i){
            std::string source_path(argv[i]);

            cv::Mat source_image;
            source_image = cv::imread(source_path.c_str(), 1);

            if (!source_image.data){
                std::cout << "Invalid source_image" << std::endl;
                continue;
            }

            cv::Mat dest_image;
            sudoku_lines(source_image, dest_image);

            source_path.insert(source_path.rfind('.'), ".lines");
            imwrite(source_path.c_str(), dest_image);
        }
    }

    return 0;
}