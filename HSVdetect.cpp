#include <cv.h>
#include <highgui.h>
#include <cxcore.h>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include <netinet/in.h>                                                         
#include <sys/socket.h>                                                         
#include <arpa/inet.h>  

#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#define PORT 1212

//IplImage *img;, *imgThreshed; 

using namespace std;
using namespace cv;

void quit(char* msg, int retval);

int serversock, clientsock; 

void send_img(int socket, Mat img,int kvalita,bool ok)
{
	if(ok == true){		
		vector<uchar> buff;
		vector<int> param = vector<int>(2);
        	param[0] = CV_IMWRITE_JPEG_QUALITY;
        	param[1] = kvalita;
		imencode(".jpg", img, buff, param);
		char len[10];
		sprintf(len, "%.8d", buff.size());
		/* sending length */
		send(socket, len, strlen(len), 0);
		/* send the image */
		send(socket, &buff[0], buff.size(), 0);
		buff.clear();
	}
	else{
                char len[10];
                sprintf(len, "%.8d", -1);
		send(socket, len, strlen(len), 0);
	}
}

int main(int argc, char** argv)
{
	IplImage *img;
	
	int iLowH = 0;
	int iHighH = 18;

	int iLowS = 200; 
	int iHighS = 255;

	int iLowV = 100;
	int iHighV = 200;

	Mat imgHSV;	
	Mat imgThresholded;
	
	CvCapture* camera = cvCaptureFromCAM(0);
	cvSetCaptureProperty( camera, CV_CAP_PROP_FRAME_WIDTH, 160);
	cvSetCaptureProperty( camera, CV_CAP_PROP_FRAME_HEIGHT, 120);

        struct sockaddr_in server;	
	char recvdata[128];

        if ((serversock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {             
		quit("socket() failed", 1);
	}    	

	/* setup server's IP and port */                                        
	memset(&server, 0, sizeof(server));                                     
	server.sin_family = AF_INET;                                            
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = INADDR_ANY;                                    
                       

	if (bind(serversock, (struct sockaddr *)&server, sizeof(server)) == -1) {     
		quit("bind() failed", 1);                                       
	}     
	

	/* wait for connection */                                               
	if (listen(serversock, 10) == -1) {                                     
		quit("listen() failed.", 1);                                    
	}                                                                       

	printf("Waiting for connection on port %d\n", PORT);
										
	/* accept a client */                                                   
	if ((clientsock = accept(serversock, NULL, NULL)) == -1) {              
		quit("accept() failed", 1);                                     
	}        
		

	printf("Connection ok\n");
	while(1) {
		img = cvQueryFrame(camera);

//		Canny( (Mat)img, imgHSV, 50, 200, 3 );
                cvtColor((Mat)img, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
//              inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image
		Mat test = imgHSV.clone();
		int indexX=-1;
		int maxX=0;
		int maxY=0;
		int indexY=0;
		int delX = 3;
		int delY = 3;
		int pocPixX[delX];
		int pocPixY[delY];
		for(int x=0;x<delX;x++)	pocPixX[x] = 0;
		for(int y=0;y<delY;y++) pocPixY[y] = 0;
//		printf("%d %d\n",imgHSV.cols,imgHSV.rows);
		for(int y=0;y<imgHSV.rows;y++){
    			for(int x=0;x<imgHSV.cols;x++){
        			// get pixel
        			Vec3b color = imgHSV.at<Vec3b>(Point(x,y));
				if(color[0] <= iHighH && iLowH <= color[0]&&
				   color[1] <= iHighS && iLowS <= color[1]&&
				   color[2] <= iHighV && iLowV <= color[2]
				 ){
					pocPixX[x/(imgHSV.cols/delX)]++;
					pocPixY[y/(imgHSV.rows/delY)]++;
					color[0] = 255;
					color[1] = 255;
					color[2] = 255;
			        	test.at<Vec3b>(Point(x,y)) = color;
				}
				else{
					color[0] = 0;
                                        color[1] = 0;
                                        color[2] = 0;
                                        test.at<Vec3b>(Point(x,y)) = color;
				}
				if((x%(imgHSV.cols/delX))==0){
                                        color[0] = 255;
                                        color[1] = 0;
                                        color[2] = 0;
                                        test.at<Vec3b>(Point(x,y)) = color;
//					printf("%d\n",(x/(imgHSV.cols/10)));
				}
                                if((y%(imgHSV.rows/delY))==0){
                                        color[0] = 0;
                                        color[1] = 0;
                                        color[2] = 255;
                                        test.at<Vec3b>(Point(x,y)) = color;
//                                      printf("%d\n",(x/(imgHSV.cols/10)));
                                }

			}
		}	
		for(int x=0;x<delX;x++){
			if(pocPixX[x] > maxX){ 
				maxX = pocPixX[x];
				indexX = x;
			}
		}
                for(int y=0;y<delY;y++){
                        if(pocPixY[y] > maxY){
                                maxY = pocPixY[y];
                                indexY = y;
                        }
                }

		if(maxY > 100 && maxX > 100)	printf("%d %d\n",indexX,indexY);
//		cvtColor((Mat)img, imgThresholded, CV_BGR2GRAY );		
  /// Reduce the noise so we avoid false circle detection
//  GaussianBlur( imgThresholded, imgThresholded, Size(9, 9), 2, 2 );
	              //          erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
                      //          dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
/*

  vector<Vec3f> circles;

  /// Apply the Hough Transform to find the circles
  HoughCircles( imgThresholded, circles, CV_HOUGH_GRADIENT, 1, 5, (double)a,(double)b, 0, 0 );

  /// Draw the circles detected
  int max_radius = 0;
  Point center_max;
  for( size_t i = 0; i < circles.size(); i++ )
  {
      Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
      int radius = cvRound(circles[i][2]);
      if(radius > max_radius){
	 max_radius = radius;
	 center_max = center;
      }
  }
	if(max_radius != 0){
      	// circle center
     	circle(imgHSV, center_max, 3, Scalar(0,255,0), -1, 8, 0 );
      	// circle outline
     	 circle(imgHSV, center_max, max_radius, Scalar(0,0,255), 3, 8, 0 );
	}
*/
		int bytes = recv(clientsock, recvdata, 128, 0);
//		printf("%d - %s\n\r",bytes,recvdata);
                if (bytes == 0){
                        printf("Connection should be closed now\n");
                        close(clientsock);

                        if ((clientsock = accept(serversock, NULL, NULL)) == -1) {
                                quit("accept() failed", 1);
                        }
                }
		if (strcmp(recvdata, "img") == 0){
//				printf("img sent\n");
  				//morphological opening (remove small objects from the foreground)
  				//erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
  				//dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 

   				//morphological closing (fill small holes in the foreground)
  				//dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
  				//erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );			
 //                               cvtColor((Mat)img, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
   //                             inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image
	
				if(img != NULL)	{
					send_img(clientsock,imgHSV,50,true);
				}	
				else{
					printf("problem img\n");
					send_img(clientsock,imgHSV,50,false);
 				}	
		}
		if (strcmp(recvdata, "img1") == 0){
//			printf("img1 sent\n");
			if(img != NULL) {
//                        	cvtColor((Mat)img, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
//				inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image
                                //morphological opening (remove small objects from the foreground)
                              //  erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
                              //  dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );

                                //morphological closing (fill small holes in the foreground)
                              //  dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
                              //  erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );

                                //send_img(clientsock,imgThresholded,50,true);
				send_img(clientsock,test,50,true);
                      	}
                        else{
                        	printf("problem img\n");
   //                             send_img(clientsock,imgThresholded,50,false);
                     	}
		}
		else if(strcmp(recvdata, "param") == 0){
			recv(clientsock, recvdata, 128, 0);			
			iLowH = atoi(recvdata);
                        recv(clientsock, recvdata, 128, 0);
                        iLowS = atoi(recvdata);
                        recv(clientsock, recvdata, 128, 0);
                        iLowV = atoi(recvdata);
                        recv(clientsock, recvdata, 128, 0);
                        iHighH = atoi(recvdata);
                        recv(clientsock, recvdata, 128, 0);
                        iHighS = atoi(recvdata);
                        recv(clientsock, recvdata, 128, 0);
                        iHighV = atoi(recvdata);
//			printf("%d %d\n",a,b);
		}
		else if(strcmp(recvdata, "cor") == 0){
			recv(clientsock, recvdata, 128, 0);
			int x = atoi(recvdata);
			recv(clientsock, recvdata, 128, 0);
			int y = atoi(recvdata);
			Vec3b hsv=imgHSV.at<Vec3b>(y,x);
 			printf("H:%i\n",hsv.val[0]);
			printf("S:%i\n",hsv.val[1]);
			printf("V:%i\n",hsv.val[2]);
		}
		else if (strcmp(recvdata, "q") == 0) {
			printf("Connection should be closed now\n");
			close(clientsock);

			if ((clientsock = accept(serversock, NULL, NULL)) == -1) {              
				quit("accept() failed", 1);                                     
			}        		
		}	
	}
	
	printf("This should not be printed\n");

	close(serversock);
	close(clientsock);
		


}

void quit(char* msg, int retval)
{
	fprintf(stderr, "%s\n", msg);

	exit(retval);
}

