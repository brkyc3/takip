#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
#include <opencv/cv.hpp>
#include <opencv2/opencv.hpp>
#include <deque>
using namespace std;
using namespace cv;

void pushmyq(deque<Mat> &q ,Mat &m){

    if(q.size()==10){
        q.pop_back();

    }
    q.push_front(m.clone());
}

Mat getfromq(deque<Mat> &q ){
    Mat ret;
    if(!q.empty()){
        ret =q.front();


        q.pop_front();


    }
        return ret;



}

bool drawing=false;
Point start;
Mat frame1;
void draw(int event, int x, int y, int flags, void* param){
    Mat& img= *((cv::Mat * )param);

    Mat cln =img.clone();
    if (event == CV_EVENT_LBUTTONDOWN){
        drawing = true;
        start ={x,y};
    } else if (event == CV_EVENT_MOUSEMOVE){
        if (drawing) {
            rectangle(cln, start, {x, y}, {0, 0, 255});
            imshow("Select object to track", cln);
        }
    } else if (event == CV_EVENT_LBUTTONUP){
        drawing = false;

        frame1 = img(Rect(start, Point{x, y})).clone();

        rectangle(cln, start, {x, y}, {0, 0, 255});
    }
}
int main() {

    deque<Mat> last5;
    Ptr<xfeatures2d::SurfFeatureDetector> detector = xfeatures2d::SurfFeatureDetector::create(300,5,5, false,true );
    VideoCapture cap("output.wmv");

    Mat selectObject;
    cap>>selectObject;

    imshow("Select object to track",selectObject);
    setMouseCallback("Select object to track",draw,&selectObject);
    waitKey(0);
    destroyWindow("Select object to track");

   Mat frame2;

   /*
    frame1 =imread("ucak.png");
    if(frame1.empty()) {
        cout << "dosya bulunamadi" << endl;
return 0;
    }

    */



//
//cap>>frame1;
//    imshow("cikti", frame1);
//    waitKey(0);


    Rect window;
    bool firstTimePass= true;
    Point2f start;
    Mat lastframe;
    while(1) {

        cv::imshow("ucak", frame1);
        cap >> frame2;

        if(frame2.empty()){
            cv::imshow("bitti", frame1);
            cv::waitKey(0);
        }

        vector<KeyPoint> keypoints1, keypoints2;
        Mat desctriptors1, desctriptors2;

        Mat gray2;
        cvtColor(frame2, gray2, COLOR_RGB2GRAY);
        detector->detectAndCompute(gray2, noArray(), keypoints2, desctriptors2);
        eskileridene:
        detector->setHessianThreshold(10);
        Mat gray1;
        cvtColor(frame1, gray1, COLOR_RGB2GRAY);
        detector->detectAndCompute(gray1, noArray(), keypoints1, desctriptors1);



        vector<DMatch> mathces;

        BFMatcher matcher;
        matcher.match(desctriptors1, desctriptors2, mathces, noArray());
        sort(mathces.begin(), mathces.end(), [](DMatch d1, DMatch d2) {
            return d1.distance < d2.distance;

        });

//        for (auto m:mathces) {
//            cout << m.distance << endl;
//        }

        int numOfGoodPoints=30;
        if(mathces.size() <30)
            numOfGoodPoints=mathces.size();


            vector<DMatch> goodMatches(mathces.begin(), mathces.begin() + numOfGoodPoints);
        Mat draw;
        drawMatches(gray1, keypoints1, gray2, keypoints2, goodMatches, draw);
        //imshow("Eslesmeler", draw);

        if(goodMatches.size()<15){

                if(!last5.empty()) {
                    frame1 = getfromq(last5);


                    goto eskileridene;
                }


        }


        if(goodMatches.empty())continue;
        vector<Point2f> query;
        vector<Point2f> train;

        for (auto match :goodMatches) {

                query.push_back(keypoints1[match.queryIdx].pt);
                train.push_back(keypoints2[match.trainIdx].pt);

        }
        cv::Mat H = estimateRigidTransform(query, train, true);
        cout<<H<<endl;
        if(H.empty()){
            imshow("warp", frame2);
            waitKey(1);
            continue;


        }

        cout<<"estimated transform  "<<endl<< H<<endl;
        Mat out;

//            warpAffine(frame1, out, H, frame2.size(), INTER_LINEAR, BORDER_CONSTANT);
//
//
//            cout<<"frame size"<<frame1.size()<<endl;


        vector<Point2f> corners = {Point2f(0, 0),
                                   Point2f(frame1.cols, 0),
                                   Point2f(0, frame1.rows),

                                   Point2f(frame1.cols, frame1.rows)
        };
        vector<Point2f> warpedCorners;


        Mat h3 = Mat::zeros(1, 3, H.type());
        h3.at<double>(0, 2) = 1;
        Mat H33;
        vconcat(H, h3, H33);

        perspectiveTransform(corners, warpedCorners, H33);
//        cout << H33 << endl;
//        cout << corners << endl;
//        cout << warpedCorners << endl;
        float sumx=0;
        float sumy=0;
        for_each(warpedCorners.begin(),warpedCorners.end(),[&sumx,&sumy](Point2f p){
            sumx+=p.x;
            sumy+=p.y;
        });
        Rect first = boundingRect(warpedCorners);
        float a=(float)H.at<double>(0,0);
        float b = (float) H.at<double>(0, 1);
        float c =(float)H.at<double>(1,0);
        float d = (float)H.at<double>(1,1);
        float xscale =sqrt(a*a+c*c) ;
        float yscale =sqrt(b*b+d*d);


        cout<<xscale<<"!!"<<yscale<<endl;
        Point2f center(sumx / 4, sumy / 4);



        start.x= center.x-(frame1.cols/2)*xscale;
        start.y = center.y - (frame1.rows / 2)*yscale;


        cout<<"start point"<<start<<endl;
        cout<<"center point"<<center<<endl;
        window= Rect (start,Size(frame1.cols*xscale,frame1.rows*yscale));

        if(window.x <0) window.x =0;
        if(window.x+window.width >frame2.cols) window.width =frame2.cols-window.x;

        if(window.y<0)window.y=0;
        if(window.y+window.height >frame2.rows) window.height =frame2.rows-window.y;
        cout<<"srect"<<window<<endl;



        Mat frame2imshow = frame2.clone();
        circle(frame2imshow, center, 3, Scalar(0, 0, 255),5);

        imshow("warp", frame2imshow);

        frame2(window).copyTo(frame1);
        pushmyq(last5, frame1);
        //imshow("son", frame1);
        waitKey(1);


    }
return 0;

}
