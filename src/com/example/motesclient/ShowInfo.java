package com.example.motesclient;

import com.example.motesclient.ShowMotes;
import com.example.motesclient.ShowMotes.SensorsValues;

import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.TextView;
import android.app.Activity;
import android.content.Intent;

public class ShowInfo extends Activity {
	
	private TextView info_motes;
	private ImageButton back_button;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Intent intent = getIntent();
        String message = intent.getStringExtra(ShowMotes.EXTRA_MESSAGE);
        setContentView(R.layout.activity_show_info);
        info_motes= (TextView) findViewById(R.id.show_motes);
        back_button= (ImageButton) findViewById(R.id.back_button);
        
        back_button.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
            	finish();
            }
       });
        
	    ShowMotes.parseDataString(message);
		info_motes.setText("");
		info_motes.append("Valor de la MAC: "+SensorsValues.getMac()); 	
		info_motes.append("\n");
		info_motes.append("Valor de la X: "+SensorsValues.getX());
		info_motes.append("\n");
		info_motes.append("Valor de la Y: "+SensorsValues.getY());
		info_motes.append("\n");
		info_motes.append("Valor de la Z: "+SensorsValues.getZ());
		info_motes.append("\n");
		info_motes.append("Valor de la temperatura: "+SensorsValues.getTemp()+" Cº");
		info_motes.append("\n");
		info_motes.append("Valor de la batería: "+SensorsValues.getBat()+" %");	
		
    }
    
   
}
