package com.example.motesclient;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;

import android.os.Bundle;
import android.os.Handler;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.GridView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.drawable.AnimationDrawable;

public class ShowMotes extends Activity{

    MyApplication app;
    private String response="";
    private Handler handler = new Handler();
    private TextView lista_motes;
    //private ListView mainListView;
    //private ArrayAdapter<String> listAdapter; 
    private ImageButton refresh_button;
    private GridView gridview;
    CharSequence text;
    int duration = Toast.LENGTH_SHORT;
    Context context;
    String[] macs=new String [5];
    String mac;
	int nMacs;
	AnimationDrawable frameAnimation;
	Thread myDataThread,connectThread;
	PrintWriter out;
	BufferedReader in;
	ImageView view;
	public final static String EXTRA_MESSAGE = "com.example.motesclient.MESSAGE";
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_show_motes);
        InitializeUI();
        app = (MyApplication) getApplication();
        context = getApplicationContext();    
        //mainListView = (ListView) findViewById( R.id.mainListView ); 
        /*
        mainListView.setOnItemClickListener(new OnItemClickListener() {
        	@Override
        	public void onItemClick(AdapterView<?> parent, View view,
					int position, long id) {
			    mac=((TextView)view).getText().toString();
			    new Thread(new DataThread()).start();               
			}
		});
        */     
                
        new Thread(){
        	public void run(){
        
				try {
					out = new PrintWriter(new BufferedWriter(new OutputStreamWriter(app.socket
					        .getOutputStream())), true);
			
					out.println("MAC REQUEST");
					Log.d("ShowMotes", "C: Sent.");
					handler.post(new Runnable() {
		                @Override
		                public void run() {
		                	text="Buscando nodos conectados a la red...";
		                	Toast toast = Toast.makeText(context, text, Toast.LENGTH_LONG);		                    	
		                	toast.setGravity(Gravity.CENTER, 0, 0);
		                	toast.show();
		                }
		        	});
					in = new BufferedReader(new InputStreamReader(app.socket.getInputStream()));                   
					
			        if ((response = in.readLine()) != null) {
			        	parseMacString(response);
			        	//ArrayList<String> macList = new ArrayList<String>();
			        	//macList.addAll(Arrays.asList(macs));
			        	//listAdapter = new ArrayAdapter<String>(context, R.layout.simplerow, macList); 
			            handler.post(new Runnable() {
			                @Override
			                public void run() {    
					        	//mainListView.setAdapter(listAdapter);;
			                	gridview.setAdapter(new ImageAdapter(context));
			                }
			            });
			        }else{
			        	app.socket.close();
			        	handler.post(new Runnable() {
			                @Override
			                public void run() {
			                	text="Conexión perdida. Asegúrese de que existen nodos conectados a la red";
			                	Toast toast = Toast.makeText(context, text, duration);		                    	
			                	toast.setGravity(Gravity.CENTER, 0, 0);
			                	toast.show();
			                }
			        	});
			        	finish();
			        }
				} catch (Exception e) {
					Log.e("ShowMotes", "C: Error", e);
		            app.connected = false;
		            try {
		            	if(app.socket!=null){
		            		app.socket.close();
		            		handler.post(new Runnable() {
			                    @Override
			                    public void run() {
			                    	text="Conexión perdida";
			                    	Toast toast = Toast.makeText(context, text, duration);		                    	
			                    	toast.setGravity(Gravity.CENTER, 0, 0);
			                    	toast.show();
			                    }
		            		});
		            	}
					} catch (IOException e1) {
						e1.printStackTrace();
					}
					
				}
        	};
        }.start();
        
        /*
        MyTimerTask myTask = new MyTimerTask();
        Timer myTimer = new Timer();
        
        myTimer.schedule(myTask, 0, 10000); 
        
        class MyTimerTask extends TimerTask {
        }
        */
    }
    
    public void InitializeUI(){
    	
    	lista_motes= (TextView) findViewById(R.id.lista_motes);
        lista_motes.setMovementMethod(new ScrollingMovementMethod());
        refresh_button= (ImageButton) findViewById(R.id.refresh_button);  
        
        refresh_button.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
            	connectThread=new Thread(new ConnectThread());
        	    connectThread.start();
            }
       });
           
        gridview = (GridView) findViewById(R.id.gridview);      
        
        gridview.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View v, int position, long id) {
                //Toast.makeText(context, "" + position, Toast.LENGTH_SHORT).show();
            	mac=macs[position];
            	view=(ImageView)v;
            	/*frameAnimation = (AnimationDrawable) v.getBackground();
            	frameAnimation.onAnimationFinish();*/
            	
                view.setBackgroundResource(R.drawable.connect_animation);
                frameAnimation = (AnimationDrawable) view.getBackground();
            	frameAnimation.start();
			
            	/*
		        view.post(new Runnable() {

		            @Override
		            public void run() {

		                if(frameAnimation.getCurrent() != frameAnimation.getFrame(frameAnimation.getNumberOfFrames() - 1))
		                {
		                    view.post(this);
		                }else
		                {
		                    view.removeCallbacks(this);
		                    view.setBackgroundResource(R.drawable.waspmote_button);
		                }

		            }
		        });
		        */
        	    myDataThread=new Thread(new DataThread());
			    myDataThread.start();
            }
        });
    }
    
    public class ConnectThread implements Runnable {

    	public void run(){ 		
            
			try {
				out.println("MAC REQUEST");
				Log.d("ShowMotes", "C: Sent.");
				handler.post(new Runnable() {
	                @Override
	                public void run() {
	                	text="Buscando nodos conectados a la red...";
	                	Toast toast = Toast.makeText(context, text, Toast.LENGTH_LONG);		                    	
	                	toast.setGravity(Gravity.CENTER, 0, 0);
	                	toast.show();
	                }
	        	});	
		        if ((response = in.readLine()) != null) {
		        	parseMacString(response);
		            handler.post(new Runnable() {
		                @Override
		                public void run() {    
		                	lista_motes.setText("");
		                	gridview.setAdapter(new ImageAdapter(context));
		                }
		            });
		        }else{
		        	app.socket.close();
		        	handler.post(new Runnable() {
		                @Override
		                public void run() {
		                	text="Conexión perdida. Asegúrese de que existen nodos conectados a la red";
		                	Toast toast = Toast.makeText(context, text, duration);		                    	
		                	toast.setGravity(Gravity.CENTER, 0, 0);
		                	toast.show();
		                }
		        	});
		        	finish();
		        }
			} catch (Exception e) {
				Log.e("ShowMotes", "C: Error", e);
	            app.connected = false;
	            try {
	            	if(app.socket!=null){
	            		app.socket.close();
	            		handler.post(new Runnable() {
		                    @Override
		                    public void run() {
		                    	text="Conexión perdida";
		                    	Toast toast = Toast.makeText(context, text, duration);		                    	
		                    	toast.setGravity(Gravity.CENTER, 0, 0);
		                    	toast.show();
		                    }
	            		});
	            		finish();
	            	}
	            	finish();
				} catch (IOException e1) {
					e1.printStackTrace();
				}				
			}
    	};
    }
    
  
    public class DataThread implements Runnable {

        public void run() {
        	try {
				out.println(mac);
				Log.d("ShowMotes", "C: Sent.");
				handler.post(new Runnable() {
	                @Override
	                public void run() {	                    
	                	text="Obteniendo los valores del nodo...";
	                	Toast toast = Toast.makeText(context, text, Toast.LENGTH_LONG);		                    	
	                	toast.setGravity(Gravity.CENTER, 0, 0);
	                	toast.show();
	                }
	        	});
				
		        if ((response = in.readLine()) != null) {
		        	frameAnimation.stop();
		        	if(response.length()<27){	
		        	    connectThread=new Thread(new ConnectThread());
		        	    connectThread.start();
		        		handler.post(new Runnable() {
			                @Override
			                public void run() {	     
			                	lista_motes.setText("");
			                	text="El nodo ha sido desconectado";
			                	Toast toast = Toast.makeText(context, text, Toast.LENGTH_LONG);		                    	
			                	toast.setGravity(Gravity.CENTER, 0, 0);
			                	toast.show();
			                }
			        	});		        	
		        	}else{
		        		int display_mode = getResources().getConfiguration().orientation;
		                if (display_mode == 1){
		                    Intent intent = new Intent(context, ShowInfo.class);		               
		                    intent.putExtra(EXTRA_MESSAGE, response);
		                    startActivity(intent); 
		                }else {    
			        		parseDataString(response);
				            handler.post(new Runnable() {
				                @Override
				                public void run() {   						        	
				                	lista_motes.setText("");
				                	lista_motes.append("Valor de la MAC: "+SensorsValues.getMac()); 	
				                	lista_motes.append("\n");
				                	lista_motes.append("Valor de la X: "+SensorsValues.getX());
				                	lista_motes.append("\n");
				                	lista_motes.append("Valor de la Y: "+SensorsValues.getY());
				                	lista_motes.append("\n");
				                	lista_motes.append("Valor de la Z: "+SensorsValues.getZ());
				                	lista_motes.append("\n");
				                	lista_motes.append("Valor de la temperatura: "+SensorsValues.getTemp()+" Cº");
				                	lista_motes.append("\n");
				                	lista_motes.append("Valor de la batería: "+SensorsValues.getBat()+" %");
				                }
				            });
		                }
					}
		        }else{
		        	app.socket.close();
		        	handler.post(new Runnable() {
		                @Override
		                public void run() {
		                	text="Conexión perdida";
		                	Toast toast = Toast.makeText(context, text, duration);		                    	
		                	toast.setGravity(Gravity.CENTER, 0, 0);
		                	toast.show();
		                }
		        	});
		        	finish();
		        }
			} catch (Exception e) {
				Log.e("ShowMotes", "C: Error", e);
	            app.connected = false;
	            try {
	            	if(app.socket!=null){
	            		app.socket.close();
	            		handler.post(new Runnable() {
		                    @Override
		                    public void run() {
		                    	text="Conexión perdida";
		                    	Toast toast = Toast.makeText(context, text, duration);		                    	
		                    	toast.setGravity(Gravity.CENTER, 0, 0);
		                    	toast.show();
		                    }
	            		});
	            		finish();
	            	}
				} catch (IOException e1) {
					e1.printStackTrace();
				}
			}
		   
        }
    }

    public class ImageAdapter extends BaseAdapter {
        private Context mContext;
        int i;
        Integer[] mThumbIds=new Integer [nMacs];

        public ImageAdapter(Context c) {
            mContext = c;	
            for (i=0;i<nMacs;i++){
            	mThumbIds[i]=R.drawable.waspmote;
            }
        }

        public int getCount() {
            return mThumbIds.length;
        }

        public Object getItem(int position) {
            return null;
        }

        public long getItemId(int position) {
            return 0;
        }

        // create a new ImageView for each item referenced by the Adapter
        
        public View getView(int position, View convertView, ViewGroup parent) {
            ImageView imageView;
            if (convertView == null) {  // if it's not recycled, initialize some attributes
                imageView = new ImageView(mContext);
                int display_mode = getResources().getConfiguration().orientation;
                if (display_mode == 1) {
                	imageView.setLayoutParams(new GridView.LayoutParams(220, 340));
                    imageView.setScaleType(ImageView.ScaleType.CENTER_CROP);
                    imageView.setPadding(0, 0, 0, 0);
                } else {
                	imageView.setLayoutParams(new GridView.LayoutParams(120, 190));
                    imageView.setScaleType(ImageView.ScaleType.CENTER_CROP);
                    imageView.setPadding(0, 0, 0, 0);
                }                        
                
            } else {
                imageView = (ImageView) convertView;
            }
            
            imageView.setImageResource(mThumbIds[position]);
            return imageView;
        }

        
        
        /*// references to our images
        private Integer[] mThumbIds = {
                R.drawable.ic_launcher, R.drawable.ic_launcher,
                R.drawable.ic_launcher, R.drawable.ic_launcher,
                R.drawable.ic_launcher
        };
        */
    }
    
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        setContentView(R.layout.activity_show_motes);
        InitializeUI();
        gridview.setAdapter(new ImageAdapter(context));
        lista_motes.setText("");
        if((SensorsValues.getMac()!=null)&&(newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE)){
	    	lista_motes.append("Valor de la MAC: "+SensorsValues.getMac()); 	
	    	lista_motes.append("\n");
	    	lista_motes.append("Valor de la X: "+SensorsValues.getX());
	    	lista_motes.append("\n");
	    	lista_motes.append("Valor de la Y: "+SensorsValues.getY());
	    	lista_motes.append("\n");
	    	lista_motes.append("Valor de la Z: "+SensorsValues.getZ());
	    	lista_motes.append("\n");
	    	lista_motes.append("Valor de la temperatura: "+SensorsValues.getTemp()+" Cº");
	    	lista_motes.append("\n");
	    	lista_motes.append("Valor de la batería: "+SensorsValues.getBat()+" %");
        }
    }
    
    
    void parseMacString(String response){
    	int start=0;
    	int index;
    	nMacs=0;
    	while((index=response.indexOf("r#waspmote#", start))!=-1){
    		start=index+11;
    		macs[nMacs]=response.substring(start, start+16);
    		nMacs++;
    	}
    }
    
    static void parseDataString(String response){
    	int start=0;
    	int end=0;
    	int cont=1;
    	while(true){
    		if((start=response.indexOf(",", start))!=-1){
	    		start++;
	    		if((end=response.indexOf(",", start))!=-1){
		    		switch(cont)
		    		{
		    			case 1:
		    				SensorsValues.setMac(response.substring(start,end));
		    				break;
		    			case 2:
		    				SensorsValues.setX(response.substring(start,end));
		    				break;
		    			case 3:
		    				SensorsValues.setY(response.substring(start,end));
		    				break;
		    			case 4:
		    				SensorsValues.setZ(response.substring(start,end));
		    				break;
		    			case 5:
		    				SensorsValues.setTemp(response.substring(start,end));
		    				break;
		
		    		}
		    	       cont++;     
	    		}else{
	    	    	SensorsValues.setBat(response.substring(start,response.length()));
	    	    	break;
	    		}
    		}else{
    			break;
    		}
    	}
    }
    
    public static class SensorsValues{
    	
    	static private String mac,x,y,z,temp,bat;
    	
    	static void setMac(String pmac){
    		mac=pmac;
    	}
    	static void setX(String px){
    		x=px;
    	}
    	static void setY(String py){
    		y=py;
    	}
    	static void setZ(String pz){
    		z=pz;
    	}
    	static void setTemp(String ptemp){
    		temp=ptemp;
    	}
    	static void setBat(String pbat){
    		bat=pbat;
    	}
    	
    	static String getMac(){
    		return mac;
    	}
    	static String getX(){
    		return x;
    	}
    	static String getY(){
    		return y;
    	}
    	static String getZ(){
    		return z;
    	}
    	static String getTemp(){
    		return temp;
    	}
    	static String getBat(){
    		return bat;
    	}
    }

}
