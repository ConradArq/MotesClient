package com.example.motesclient;

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import android.app.Application;
import android.content.Context;
import android.content.res.Configuration;
import android.util.Log;

public class MyApplication extends Application{
	
    public Socket socket;
    Context context;
    private Thread cThread;
    public boolean connected=false;
    
	@Override
	public void onConfigurationChanged(Configuration newConfig) {
	    super.onConfigurationChanged(newConfig);
	}

	@Override
	public void onCreate() {
	    super.onCreate();
	    context = getApplicationContext();
	}

	@Override
	public void onLowMemory() {
	    super.onLowMemory();
	}

	@Override
	public void onTerminate() {
	    super.onTerminate();
	}	
	
	
	public void establishConnection(){
		cThread = new Thread(new ClientThread());
        cThread.start();  
	}
		       
    
	public class ClientThread implements Runnable {

	    public void run() {
	    	Log.d("ClientActivity", "10.10.10.1");
	        InetAddress serverAddr;
			try {
				serverAddr = InetAddress.getByName("10.10.10.1");
				socket = new Socket(serverAddr, 5000);
				Log.d("ConnectActivity", "C: Connecting...");
				connected=true;
			} catch (UnknownHostException e) {
				e.printStackTrace();
			} catch (IOException e) {
				e.printStackTrace();
			}
	    };
 	   
    }
	
}
