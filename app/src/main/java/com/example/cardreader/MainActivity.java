/* Aly Osama and Kareem AlTarabishy
 *
 * ScratchScanner
 * 
 * Handles camera input, preprocessing, OCR, parsing, and call the number
 */

package com.example.cardreader;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.net.Uri;
import android.os.Bundle;
import android.provider.ContactsContract;
import android.provider.MediaStore;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;


import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.Mat;

import com.googlecode.tesseract.android.TessBaseAPI;



public class MainActivity extends Activity {
	
	//initialize RectifyCard function from native code
	public native void RectifyCard(
			long addr_inImg,
			long addr_outRectImg,
			long addr_outImg_gray,
			long addr_outImg_binarized,
			long addr_addTextRoi);

	private static final int CAPTURE_IMAGE_ACTIVITY_REQUEST_CODE = 100;
	private static final String TAG = "CardReader";
	private static final String PIPELINE_STAGE = "PIPELINE_STAGE";	
	private Bitmap capturedBitmap = null;
	private Bitmap rectifiedBitmap = null;
	private Bitmap binarizedBitmap = null;
	int textRoi[][] = null;
	int numROI;
	private static final String temp_path = "/sdcard/temp_CardReader.jpg";
	
	public static final String lang = "eng";
	
	//set to true to capture from camera, false to read from sd card
	private static final boolean captureFromCamera = true;
	
	//initialize OCR baseAPI
	private TessBaseAPI baseApi = new TessBaseAPI();
	
	//variables to store parsed information
    private String DisplayNumbers = "";

	private static int pipeline_stage = 0;

	//code to load OpenCV library
	private BaseLoaderCallback  mLoaderCallback = new BaseLoaderCallback(this) {
	      @Override
	      public void onManagerConnected(int status) {
	          switch (status) {
	              case LoaderCallbackInterface.SUCCESS:
	              {
	                  Log.i(TAG, "OpenCV loaded successfully");

	                   //Load native library after(!) OpenCV initialization
	                  System.loadLibrary("cardreader");
	                  Log.i(TAG, "Native library loaded successfully");

	              } break;
	              default:
	              {
	                  super.onManagerConnected(status);
	              } break;
	          }
	      }
	  };

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.d(TAG, "in onCreate()");

		super.onCreate(savedInstanceState);
		
		//begin app pipeline
		pipeline_stage = 0;
		pipelineNextStep();
		
	}
	
	
	public class ButtonClickHandler implements View.OnClickListener {
		public void onClick(View view) {
			
			Log.v(TAG, "in OnClickListener()");
			
			// run the pipeline
			pipelineNextStep();
		}
	}

	//state machine to handle app processes
	private void pipelineNextStep()
	{
		Log.d(TAG, "in pipelineNextStep(): stage = " + pipeline_stage);
		
		switch( pipeline_stage )
		{
		//state displaying start button
		case 0:
		{
			pipeline_stage = pipeline_stage + 1;
			
			setContentView(R.layout.activity_main);
			Button _button = (Button) findViewById(R.id.button);
	        _button.setOnClickListener(new ButtonClickHandler());
			break;
		}
		//state that starts camera intent
		case 1: 
		{
			// capture the image using device's camera application
			pipeline_stage = pipeline_stage + 1;
			if(captureFromCamera)
				captureImage();
			//reads image from sd card
			else
				readImageFromSDCard();
			break;
		}
		case 2: 
		{
			// rectify image
			pipeline_stage = pipeline_stage + 1;
			rectifyImage();
			break;
		}
		case 3:
		{
			// perform OCR
			pipeline_stage = pipeline_stage + 1;
			performOCR();
			break;
			
		}
	
		case 4:
		{
			pipeline_stage = pipeline_stage + 1;
			
			// popup to prompt user to add contact
			new AlertDialog.Builder(this)
			  .setMessage(DisplayNumbers)
			  .setPositiveButton("Call", new DialogInterface.OnClickListener() {
			    public void onClick(DialogInterface dialog, int whichButton) {
			      Log.d(TAG,"Clicked Dialog: Yes!");
			      
			      // calls function to generate intent to add contact
			      callScratchNumber();
			    }
			  }) 
			  .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			    public void onClick(DialogInterface dialog, int whichButton) {
			    	Log.d(TAG,"Clicked Dialog: Cancel");
			    }
			  })
			  .show();   		    
						
			break;
		}
		default: 
		{
			//should never reach this stage
			Log.d(TAG, "Wrong Stage");
			pipeline_stage = 0;
		}

		}
	}

	//code to read input image from sd card
	private void readImageFromSDCard() {
		BitmapFactory.Options options = new BitmapFactory.Options();
		options.inSampleSize = 4;
		capturedBitmap = BitmapFactory.decodeFile("/sdcard/0004.jpg", options);
		Log.d(TAG, "Captured Bitmap: " + capturedBitmap.getWidth() + "X" + capturedBitmap.getHeight());
		pipelineNextStep();
	}

	//function to perform OCR
	private void performOCR() {
		
		//create "tessdata" folder on SD card
		String[] paths = new String[] {"/sdcard/", "/sdcard/" + "tessdata/" };

		for (String path : paths) {
			File dir = new File(path);
			if (!dir.exists()) {
				if (!dir.mkdirs()) {
					Log.v(TAG, "ERROR: Creation of directory " + path + " on sdcard failed");
					return;
				} else {
					Log.v(TAG, "Created directory " + path + " on sdcard");
				}
			}

		}
		
		// copy all training files to SD card from assets folder
		String[] ocrFileList = new String[] {".cube.bigrams", ".cube.fold", ".cube.lm", ".cube.nn",
				".cube.params", ".cube.size", ".cube.word-freq", ".tesseract_cube.nn",
				".traineddata"};
		for( int i=0; i<ocrFileList.length; i++ )
		{
			if (!(new File("/sdcard/" + "tessdata/" + lang + ocrFileList[i])).exists()) {
				try {
	
					AssetManager assetManager = getAssets();
					InputStream in = assetManager.open("tessdata/" + lang + ocrFileList[i]);
					OutputStream out = new FileOutputStream("/sdcard/"
							+ "tessdata/" + lang + ocrFileList[i]);
	
					// Transfer bytes from in to out
					byte[] buf = new byte[1024];
					int len;
					while ((len = in.read(buf)) > 0) {
						out.write(buf, 0, len);
					}
					in.close();
					out.close();
					
					Log.v(TAG, "Copied " + lang + ocrFileList[i]);
				} catch (IOException e) {
					Log.e(TAG, "Was unable to copy " + lang + ocrFileList[i] + e.toString());
				}
			}
		}
		
		//initialize tesseract OCR
		baseApi.init("/sdcard/", lang);
		baseApi.setDebug(true);
		
		
		String allText = "";
		
		//perform OCR on each text bounding box
		for( int i=0; i<numROI; i++ )
		{
			
			Bitmap croppedBitmap = Bitmap.createBitmap(rectifiedBitmap,
					textRoi[i][0], textRoi[i][1],
					textRoi[i][2], textRoi[i][3]);
			Log.d(TAG, "Calling OCR with ROI " + (i+1) + " of " + numROI);
			
			
			try {
		  	       FileOutputStream out = new FileOutputStream("/sdcard/temp_box" + (i+1) + ".png");
		  	       croppedBitmap.compress(Bitmap.CompressFormat.PNG, 90, out);
		  	       out.close();
		  	} catch (Exception e) {
		  	       e.printStackTrace();
		  	}
            //For example if we want to only detect numbers
            baseApi.setVariable(TessBaseAPI.VAR_CHAR_WHITELIST, "1234567890");
			//set image that OCR runs on
			baseApi.setImage(croppedBitmap);
			//line to actually get OCR text result
			String textResult = baseApi.getUTF8Text();

			allText = allText + "\n" + textResult;
			baseApi.clear();
	
			Log.d(TAG, "Text result: " + textResult);
			croppedBitmap.recycle();
		}
		
		//parse OCR output
		extractInformation(allText);

		pipelineNextStep();
	}
	
	//function to call native code to rectify image
	private void rectifyImage() {
		
		Log.d(TAG, "in rectifyImage()");
		
		// show the captured image
		setContentView(R.layout.rectified_image_view);
		ImageView imgView = (ImageView) findViewById(R.id.imgView_rectified);
		imgView.setImageBitmap(capturedBitmap);
		
		
		int height = capturedBitmap.getHeight();
		int width = capturedBitmap.getWidth();		
		Log.d(TAG, "height: " + height + ", width: " + width);
		
		// call the rectification function
		Mat mat = new Mat();
  	  	Utils.bitmapToMat(capturedBitmap, mat);
  	  	
  	  	// images to be returned
  	  	Mat matout = new Mat();
  	  	Mat matout_gray = new Mat();
  	  	Mat matout_binary = new Mat();
  	  	Mat matout_textROI = new Mat();

  	  	
  	  	Log.d(TAG, "Before rectifyCard call");
  	  	RectifyCard(mat.getNativeObjAddr(), matout.getNativeObjAddr(), 
  	  		matout_gray.getNativeObjAddr(), matout_binary.getNativeObjAddr(),
  	  		matout_textROI.getNativeObjAddr());
		Log.d(TAG, "After rectifyCard call");
  	  	
  	  	height = matout_binary.rows();
	  	width = matout_binary.cols();
		
	  	numROI = matout_textROI.height();
		textRoi = new int[numROI][4];		
		for( int i=0; i<numROI; i++ )
		{
			for( int j=0; j<4; j++ )
			{
				double[] data = matout_textROI.get(i, j);
				textRoi[i][j] = (int)(data[0]);
			}
			Log.d(TAG, "ROI " + i + ": " + 
					textRoi[i][0] + " " + " " + textRoi[i][1] 
							+ " " + textRoi[i][2] + " " + textRoi[i][3]);
		}
		
  	  	
		rectifiedBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
		Utils.matToBitmap(matout_binary, rectifiedBitmap);
  	  	imgView.setImageBitmap(rectifiedBitmap);
  	  	
	  	try {
	  	       FileOutputStream out = new FileOutputStream("/sdcard/rectified.png");
	  	       rectifiedBitmap.compress(Bitmap.CompressFormat.PNG, 90, out);
	  	       out.close();
	  	} catch (Exception e) {
	  	       e.printStackTrace();
	  	}
		
		pipelineNextStep();
	}


	private void captureImage() {

		Log.d(TAG, "in captureImage()");

		File file = new File(temp_path);
		Uri fileUri = Uri.fromFile(file);

		// create Intent to take a picture and return control to the calling application
		Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
		intent.putExtra(MediaStore.EXTRA_OUTPUT, fileUri); // set the image file name

		// start the image capture Intent
		startActivityForResult(intent, 0);

		Log.d(TAG, "Camera Intent sent");
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {

		Log.d(TAG, "in onActivityResult() " + requestCode + " " + resultCode);
		if (requestCode == 0) {
			if (resultCode == RESULT_OK) {
				Log.d(TAG, "Image captured successfully");
				BitmapFactory.Options options = new BitmapFactory.Options();
				options.inSampleSize = 4;
				capturedBitmap = BitmapFactory.decodeFile(temp_path, options);
				
				pipelineNextStep();

			} else if (resultCode == RESULT_CANCELED) {
				Toast.makeText(this, "Image capture cancelled", Toast.LENGTH_LONG).show();
				Log.d(TAG, "Image capture cancelled");
			} else {
				Toast.makeText(this, "Image Capture Failed", Toast.LENGTH_LONG).show();
				Log.d(TAG, "Image capture Failed");
			}
		}
	}

	@Override
	protected void onResume() {
		Log.d(TAG, "in onResume()");
		super.onResume();
		
		OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_6, this, mLoaderCallback);
	}

	@Override
	protected void onSaveInstanceState(Bundle outState) {
		Log.d(TAG, "in onSaveInstanceState()");
	}

	@Override
	protected void onRestoreInstanceState(Bundle savedInstanceState) {
		Log.d(TAG, "in onRestoreInstanceState()");
	}
	
	//code to parse OCR output using regular expressions
	private void extractInformation(String str) {

//	    Pattern p;
//	    Matcher m;
//
//	    /*
//	     * Phone-matching Expression - Matches: 1234567890 (650) 720-5678
//	     * 650-720-5678 650.720.5678 - Does not match: 12345 12345678901 720-5678
//	     */
//        p = Pattern.compile("(?:^|\\D)(\\d{3})[)\\-. ]*?(\\d{3})[\\-. ]*?(\\d{3})(?:$|\\D)");
//        m = p.matcher(str);
//
//	    if (m.find()) {
//            DisplayNumbers = m.group().toString();
//	    }
//        else{
//            DisplayNumbers = "Can't Find The Number";
//        }

        DisplayNumbers=str;
        // For Testing
	    Log.d(TAG, "Input: " + str);
	    Log.d(TAG,DisplayNumbers);
  }

	//calls intent to add call number
	private void callScratchNumber() {
            Intent intent = new Intent(Intent.ACTION_CALL, Uri.parse("tel:" + "*556*" + DisplayNumbers + Uri.encode("#")));
            this.startActivity(intent);
	}

}
