package com.example.motesclient;

import java.io.IOException;

import android.os.Bundle;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.Toast;

public class ConnectActivity extends Activity {

    MyApplication app;
    private ImageButton connectButton;
    Context context;
    CharSequence text;
    int duration = Toast.LENGTH_SHORT;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_connect);
        context = getApplicationContext();
    	app = (MyApplication) getApplication();
        connectButton = (ImageButton) findViewById(R.id.connect_button);
        connectButton.setOnClickListener(connectListener);
    }
    
    @Override
    public void onStart(){
    	super.onStart();
    	try {
    		if(app.socket!=null){
    			app.socket.close();
    			app.connected=false;
    			Log.d("ConnectActivity", "cerrando socket");
    		}
		} catch (IOException e) {
			e.printStackTrace();
		}
    	
    }
    
    private OnClickListener connectListener = new OnClickListener() {

        @Override
        public void onClick(View v) {
            if (!app.connected){    
            	app.establishConnection();
            	try {
            		synchronized (this) {
            			this.wait(200);
            		}
					
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
            	if(app.connected){
            		text="La conexión ha sido establecida";
            		Intent intent = new Intent(context, ShowMotes.class);
            		startActivity(intent);   
            	}else 
            		text="No se ha podido establecer la conexión";
          	    
           	    Toast toast = Toast.makeText(context, text, duration);		                    	
        	    toast.setGravity(Gravity.CENTER, 0, 0);
        	    toast.show();     
	        }      	         
        }
   };
   
}
