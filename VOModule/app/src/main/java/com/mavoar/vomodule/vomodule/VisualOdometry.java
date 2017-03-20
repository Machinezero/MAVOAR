package com.mavoar.vomodule.vomodule;

import android.app.Activity;
import android.hardware.Camera;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;
import android.widget.TextView;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewListener;
import org.opencv.android.JavaCameraView;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Core;
import org.opencv.core.Mat;
import org.opencv.core.Point;
import org.opencv.core.Scalar;
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;

public class VisualOdometry extends Activity implements CameraBridgeViewBase.CvCameraViewListener2 {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("visualodometry");
    }

    private float fl;
    private CameraBridgeViewBase mOpenCvCameraView;
    private String TAG= "COISO";

    private int ratio= 8;

    private Size downscaled;
    private Size upscaled;
    private SensorListener msensorListener;
    double scale=0.0;


    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                {
                    Log.i(TAG, "OpenCV loaded successfully");
                    mOpenCvCameraView.enableView();
                } break;
                default:
                {
                    super.onManagerConnected(status);
                } break;
            }
        }
    };

    @Override
    public void onResume()
    {
        super.onResume();
        if (!OpenCVLoader.initDebug()) {
            Log.d("OpenCV", "Internal OpenCV library not found. Using OpenCV Manager for initialization");
            OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_3_0_0, this, mLoaderCallback);
        } else {
            Log.d("OpenCV", "OpenCV library found inside package. Using it!");
            mLoaderCallback.onManagerConnected(LoaderCallbackInterface.SUCCESS);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        fl= getIntent().getFloatExtra("fl",0.0f);
        Log.i(TAG, "focal length: "+ fl);


        msensorListener = new SensorListener(this);

        mOpenCvCameraView = new JavaCameraView(this, -1);
        setContentView(mOpenCvCameraView);
        mOpenCvCameraView.setCvCameraViewListener(this);
    }
    @Override
    public void onCameraViewStarted(int width, int height) {
        downscaled = new Size(width/ratio,height/ratio);
        upscaled = new Size(width,height);

        float ppx = ((float)width)/2.0f;
        float ppy = ((float)height)/2.0f;

        init(fl,ppx,ppy);
    }

    @Override
    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {
        Mat matGray = inputFrame.gray();
        double scale=getScalefromSensorListener();
        float[] rot=msensorListener.getRot();

        Imgproc.resize( matGray, matGray, downscaled );
        String message=processFrame(matGray.getNativeObjAddr(),scale,rot);
        Imgproc.resize( matGray, matGray, upscaled );


        Mat rgb= inputFrame.rgba();

        Imgproc.putText(rgb,message,new Point(10, 50),               // point
                Core.FONT_HERSHEY_SIMPLEX ,      // front face
                1,                               // front scale
                new Scalar(255, 0, 0),             // Scalar object for color
                4 );

        Imgproc.putText(rgb,scale+"",new Point(10, 200),               // point
                Core.FONT_HERSHEY_SIMPLEX ,      // front face
                1,                               // front scale
                new Scalar(255, 0, 0),             // Scalar object for color
                4 );

        Imgproc.putText(rgb,rot[0]+" " +rot[1]+" "+rot[2],new Point(10, 300),               // point
                Core.FONT_HERSHEY_SIMPLEX ,      // front face
                1,                               // front scale
                new Scalar(255, 0, 0),             // Scalar object for color
                4 );

        return rgb;
    }

    @Override
    public void onCameraViewStopped() {
        //mRgba.release();
    }

    private double getScalefromSensorListener(){
        scale = msensorListener.getScale();

        return scale;
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native void init(float focalLength,float ppx,float ppy);
    public native String processFrame(long matPointer,double scale,float[] rot);
}
