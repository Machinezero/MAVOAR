#include "mvo.h"
#include "mvo_features.h"

#include <jni.h>
#include <string>
#include <android/log.h>

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;

Stage stage;
struct Frames frames;
struct Camera camera;
struct Matrices matrices;

extern "C"
JNIEXPORT void JNICALL
Java_com_mavoar_vomodule_vomodule_VisualOdometry_init(
        JNIEnv *env,
        jobject /* this */,jfloat focalLength,jfloat ppx,jfloat ppy) {
    LOGD("init");
    stage=WAITING_FIRST_FRAME;
    camera.focal = (float) focalLength;
    camera.pp.x= (float) ppx;
    camera.pp.y= (float) ppy;
}



void initialFix(){
    try
        {
        vector<Point2f> points1, points2; //vectors to store the coordinates of the feature points
        featureDetection(frames.prev_frame, points1); //detect features in img_1

        if(points1.size()>0){
            vector < uchar > status;
            featureTracking(frames.prev_frame, frames.curr_frame, points1, points2, status); //track those features to img_2

            if(points2.size()>0){

                matrices.essential = findEssentialMat(points2, points1, camera.focal, camera.pp, RANSAC, 0.999, 1.0, matrices.mask);
                recoverPose(matrices.essential, points2, points1, matrices.rotation, matrices.translation, camera.focal, camera.pp, matrices.mask);

                frames.prev_features = points2;

                matrices.total_rotation = matrices.rotation.clone();
                matrices.total_translation = matrices.translation.clone();

                stage=WAITING_FRAME;

            }
        }

        if(points2.size()==0 | points1.size()==0){
            stage=WAITING_FIRST_FRAME;
        }
    }
    catch( cv::Exception& e )
    {
        LOGD("Exception caught. resetting stages.");
        stage=WAITING_FIRST_FRAME;
    }
}

void mvoDetectAndTrack(){
    try{
        vector < uchar > status;
        featureTracking(frames.prev_frame, frames.curr_frame, frames.prev_features, frames.curr_features, status);

        if(frames.curr_features.size()>0){

            matrices.essential = findEssentialMat(frames.curr_features, frames.prev_features, camera.focal, camera.pp, RANSAC, 0.999,
                    1.0, matrices.mask);
            recoverPose(matrices.essential, frames.curr_features, frames.prev_features, matrices.rotation, matrices.translation, camera.focal, camera.pp, matrices.mask);

            //Mat prevPts(2, frames.prev_features.size(), CV_64F), currPts(2, currFeatures.size(),
            //        CV_64F);

            /*for (int i = 0; i < frames.prev_features.size(); i++) { //this (x,y) combination makes sense as observed from the source code of triangulatePoints on GitHub
                prevPts.at<double>(0, i) = frames.prev_features.at(i).x;
                prevPts.at<double>(1, i) = frames.prev_features.at(i).y;

                currPts.at<double>(0, i) = currFeatures.at(i).x;
                currPts.at<double>(1, i) = currFeatures.at(i).y;
            }*/
            float scale=1.0;
            if ((scale > 0.1) && (matrices.translation.at<double>(2) > matrices.translation.at<double>(0))
                    && (matrices.translation.at<double>(2) > matrices.translation.at<double>(1))) {


                matrices.total_translation = matrices.total_translation + scale * (matrices.total_rotation * matrices.translation);
                matrices.total_rotation = matrices.rotation * matrices.total_rotation;

            }
            frames.prev_features = frames.curr_features;
        }
        else{
            stage=WAITING_FIRST_FRAME;
        }
    }
    catch( cv::Exception& e )
    {
        LOGD("Exception caught. resetting stages.");
        stage=WAITING_FIRST_FRAME;
    }
}

jstring returnMessage(JNIEnv *env,char str[]){

 char buffer [100];
 sprintf (buffer, "%s - %f %f %f",str,matrices.total_translation.at<double>(0),
 matrices.total_translation.at<double>(1),matrices.total_translation.at<double>(2));

 LOGD("%s",buffer);

 return env->NewStringUTF(buffer);
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_mavoar_vomodule_vomodule_VisualOdometry_processFrame(
        JNIEnv *env,
        jobject /* this */,
        jlong matAddrGray) {
    LOGD("Received Frame");

    jstring returnString;
    switch(stage){
        case WAITING_FIRST_FRAME:
            LOGD("First Frame");
            stage=WAITING_SECOND_FRAME;

            frames.prev_frame = ((Mat*)matAddrGray)->clone();
            returnString = env->NewStringUTF("Fixing");

            break;
        case WAITING_SECOND_FRAME:
            LOGD("Second Frame");

            frames.curr_frame = (*(Mat*)matAddrGray);

            initialFix();
            returnString = returnMessage(env,"Fixing");



            break;
        case WAITING_FRAME:
            LOGD("Normal Frame");
            frames.prev_frame = frames.curr_frame.clone();
            frames.curr_frame = *(Mat *) matAddrGray;
            mvoDetectAndTrack();
            returnString = returnMessage(env,"Tracking");

            break;
        }
        return returnString;
}


