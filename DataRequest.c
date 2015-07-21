#include <stdio.h>
#include <string.h> 
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <ctype.h>
#include <mysql.h>
#include <arpa/inet.h>

#define MAX 100
int end=0;
char c;
char atmymy[1000] = "";
int readserial = 0;
int count = 0;
int longitude = 0;
typedef enum { false, true } bool;
bool error=false;

int sd = 3;
char *serialPort = "/dev/ttyS0";
char str[MAX] = "";
char *aux;
int i=0;

int listenfd = 0, n;
int connfd;
struct sockaddr_in serv_addr; 
char recvBuff[1024], sendBuff[2048];

struct termios opciones;

int speed = B38400;

struct xbee_frame {
   char mac[20];
   char x[20];
   char y[20];
   char z[20];
   char temp[20];
   char bat[20];
};
struct xbee_frame strct_frame;

void parseFrame(struct xbee_frame *p_frame);
void showData();

int storeInADB(struct xbee_frame *p_frame);
char sql[700] = "";
MYSQL mysql;

void sendBroadcast(char *, char *);
void sendMac(char *, char *, char *);
bool sendToPhone=false;

char *clientHost;

bool exmut=true;
bool exmut2=true;

/**************************************************
* CODE FOR THREAD 1 (READ FROM SERIAL INTERFACE)
**************************************************/
void * read_serial( void * temp_pt )
{
   while(!end)
   {
      if (read(sd,&c,1)!=0)
      if ((isprint(c)!=0) || (c=='\n'))
      //fprintf(stderr, &c, 1);
      if(readserial)
      {
          atmymy[count] = c;
          count++;
      }
   }
    return 0;
}


/**************************************************
* CODE FOR THREAD 2 (READ FROM SOCKET)
**************************************************/
void * listener( void * temp_pt )
{
	int connfdLocal=connfd;
        char* clientHostLocal;
	clientHostLocal=(char*)malloc(20);
	strcpy(clientHostLocal,clientHost);
	while ( (n = read(connfdLocal, recvBuff, sizeof(recvBuff)-1)) > 0)
    	{

		char solicitud[12],solicitud2[16];		
		strncpy(solicitud,recvBuff,11);
		char *datos=(char*)malloc(1000);
		if(!strcmp(solicitud,"MAC REQUEST")){
			printf("%s ha realizado una solicitud de tipo: %s\n",clientHostLocal,solicitud);
			while(!exmut); //se garantiza exclusión mutua a sendBroadcast
			exmut=false;
			sendBroadcast("Hello mote", datos);	
			exmut=true;			
			datos[strlen(datos)]='\n'; //flushing the string datos, adding \n is similar to fflush
			write(connfdLocal, datos, strlen(datos));
			printf("Cadena enviada a %s: %s\n",clientHostLocal,datos);
		}else{ //recvBuff contiene la direccion mac del mote del cual se obtendra la informacion
			strncpy(solicitud2,recvBuff,16);
			printf("%s ha realizado una solicitud de tipo: GET SENSORS VALUES\n",clientHostLocal);
			while(!exmut2);
			exmut=false;
			sendMac(solicitud2, "Parameters request", datos);
			exmut=true;
			datos[strlen(datos)]='\n';
			write(connfdLocal, datos, strlen(datos));
			printf("Cadena enviada a %s: %s\n",clientHostLocal,datos);
		}
    	}

	if(n <= 0)
	{	
		printf("\nEl host %s se ha desconectado\n", clientHostLocal);
		close(connfdLocal);
		return 0;
	} 
    
}

int init(int argc, char *argv[])
{
   if ((sd = open(serialPort, O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1)
   {
      fprintf(stderr, "Unable to open the serial port %s - \n", serialPort);
      exit(-1);
   }
   else
   {
      if (!sd)
      {
         /* Sometimes the first time you call open it does not return the
          * right value (3) of the free file descriptor to use, for this
          * reason you can set manually the sd value to 3 or call again
          * the open function (normally returning 4 to sd), advised!
          */
         sd = open(serialPort, O_RDWR | O_NOCTTY | O_NONBLOCK);
      }

      //fprintf(stderr,"Serial Port open at: %i\n", sd);
      fcntl(sd, F_SETFL, 0);
   }

   tcgetattr(sd, &opciones);
   cfsetispeed(&opciones, speed);
   cfsetospeed(&opciones, speed);
   opciones.c_cflag |= (CLOCAL | CREAD);

   /*
   * No parity
   */
   opciones.c_cflag &= ~PARENB;
   opciones.c_cflag &= ~CSTOPB;
   opciones.c_cflag &= ~CSIZE;
   opciones.c_cflag |= CS8;

   /*
   * raw input:
   * making the applycation ready to receive
   */
   opciones.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

   /*
   *Ignore parity errors
   */
   opciones.c_iflag |= ~(INPCK | ISTRIP | PARMRK);
   opciones.c_iflag |= IGNPAR;
   opciones.c_iflag &= ~(IXON | IXOFF | IXANY | IGNCR | IGNBRK);
   opciones.c_iflag |= BRKINT;

   /*
   * raw output
   * making the applycation ready to transmit
   */
   opciones.c_oflag &= ~OPOST;

   /*
   * apply
   */
   tcsetattr(sd, TCSANOW, &opciones);


   pthread_t thread1;

   // initializing thread that scan serial port for incoming data.
   if( pthread_create( &thread1, NULL, read_serial, NULL ) != 0 )
   {
      printf("Cannot create thread1 , exiting \n ");
      exit(-1); // exit with errors
   }
}

void sendBroadcast(char *cadena, char *datos)
{

   char response[1000] = "";
   char mymy[15] = ""; 
   char *str_start;
   char *str_end;
   int index;
   // at mode, writting +++, xbee returns Ok, 
   // so then is posible write at commands to setup module
   write(sd,"+++",3);
   sleep(2);

   /*
    * An at command is composed by:
    * "at"+"parameter to configure"+"value"+"\r"+"\n" -> to set value
    * "at"+"parameter to configure"+"\r"+"\n" -> to get value
    */ 

   //set destination mac hi -> set 32 most significant bits
   write(sd,"atdh00000000\r\n",14);
   usleep(70000);

   //set destination mac low -> set 32 less significant bits
   write(sd,"atdl0000ffff\r\n",14);
   usleep(70000);

/*
   //mymy[15]->SEGMENTATION DEFAULT al ejecutar la función sendBroadcast 2 veces, se soluciona haciendo mymy [150] por ejemplo-> trozo de código inútil, atmy siempre devuelve 0 en capturer
   // save my atmy (my network id)
   // get the current value of my network id
   count = 0;
   readserial=1;
      write(sd,"atmy\r\n",6);
      //usleep(70000);
      usleep(170000);
      strcpy (mymy, atmymy);
   readserial=0;
*/
   // set atmy to ffff
   // to send in 64 bit mode, the network id has to be set to ffff
   write(sd,"atmyffff\r\n",10);
   usleep(70000);

   // leave at mode -> close connection
   write(sd,"atcn\r",5);
   usleep(70000);


   // send message -> write throught serial port
   // set the waspmote API headers
   int id = 48;
   int numFragmentos = 1;
   int sharp = 35;
   int idType = 2;
   char ni[] = "meshlium#";
   write(sd, &id, 1);
   write(sd, &numFragmentos, 1);
   write(sd, &sharp, 1);
   write(sd, &idType, 1);
   write(sd, ni, 9);
   // send message
   write (sd, cadena, strlen(cadena));

   // read response
   // get message that comes from waspmote
   count = 0;
   readserial=1;
      sleep(5);
      strcpy (response, atmymy);
      memset(atmymy,'\0',1000); //llenamos atmymy de '\0' para limpiar la cadena, que sera reutilizada
      if(!strcmp(response,mymy)) //if response==""
      {
         // module doesn't get response
         fprintf(stderr,"\nUN ERROR HA OCURRIDO. ASEGURESE DE QUE EXISTEN NODOS CONECTADOS A LA RED\n");
	 exit(0);
      }
      else
      {	   
	   aux=strchr(response,',');
           if(aux==NULL){  //Se recibe solo la direccion MAC de uno o varios motes
		if(datos!=NULL)
			strcpy(datos,response);
		else{
			int longitud=strlen(response);
			int i;
			int numMote=1;
			char mac[16];
			for(i=11;i<longitud;i=i+27){
				strncpy(mac,response+i,16);
				mac[16]='\0';
				fprintf(stderr, "\nMAC del Mote nº %d: %s\n",numMote,mac);
				numMote++;			
			}
		}
	   }else{ //Se reciben datos de uno o varios motes. Cada trama de cada mote empieza por "r#waspmote#"
 		str_start=response+11; //str_start apunta a MAC quitando la cabecera (r#waspmote#)
		while(1){
			str_end=strstr(str_start,"r#waspmote#");
			if(str_end!=NULL){
				index=str_end-str_start;
				strncpy(aux,str_start,index);
				aux[index]='\0';
			}else{
				aux=str_start;
				parseFrame(&strct_frame);			
				showData();
				break;
			}
			parseFrame(&strct_frame);			
			showData();
			str_start=str_end+11;	
                }
	      }   
      }
   readserial=0;

/*
  // at mode
   write(sd,"+++",3);
   sleep(2);

   // make atmy command to be restored
   char atmy[20] = "atmy";
   strcat (atmy, mymy);
   strcat (atmy, "\r\n");

   // restore atmy
   write(sd,atmy,10);
   usleep(70000);

   // leave at mode -> close connection
   write(sd,"atcn\r",5);
   usleep(70000);
*/
}

void sendMac(char *mac, char *cadena, char *datos)
{
   char mymy[150] = "";
   char response[200] = "";
   char macHigh[15] = "";
   char macLow[15] = "";

   // divide mac that comes from argv[] in:
   // 32 most significant bits and
   // 32 less significant bits
   strncpy(macHigh,mac,8);
   strncpy(macLow,mac+8,8);

   // at mode
   write(sd,"+++",3);
   sleep(2);

   /*
    * An at command is composed by:
    * "at"+"parameter to configure"+"value"+"\r"+"\n" -> to set value
    * "at"+"parameter to configure"+"\r"+"\n" -> to get value
    */ 

   // make atdh & atdl command
   // atdh -> set 32 most significant bits
   // atdl -> set 32 less significant bits
   char ath[20] = "atdh";
   strcat (ath, macHigh);
   strcat (ath, "\r\n");
   char atl[20] = "atdl";
   strcat (atl, macLow);
   strcat (atl, "\r\n");

   // set atdh
   write(sd,ath,14);
   usleep(70000);

   // set atdl
   write(sd,atl,14);
   usleep(70000);

   // set atmy as ffff (network id)
   // to send in 64 bit mode, the network id has to be set to ffff
   write(sd,"atmyffff\r\n",10);
   usleep(70000);

   // leave at mode -> close connection
   write(sd,"atcn\r",5);
   usleep(70000);

   // send message -> write throught serial port
   // set the waspmote API headers
   int id = 48;
   int numFragmentos = 1;
   int sharp = 35;
   int idType = 2;
   char ni[] = "meshlium#";
   write(sd, &id, 1);
   write(sd, &numFragmentos, 1);
   write(sd, &sharp, 1);
   write(sd, &idType, 1);
   write(sd, ni, 9);
   // send message
   write (sd, cadena, strlen(cadena));


   // read response
   // get message that comes from waspmote
   count = 0;
   readserial=1;
      sleep(5);
      strcpy (response, atmymy);
      memset(atmymy,'\0',1000); 
      if(!strcmp(response,mymy))
      {
         //module doesn't get response
         fprintf(stderr,"\nUN ERROR HA OCURRIDO. ASEGURESE DE QUE EXISTE UN NODO CON ESA MAC EN LA RED\n");
	 error=true;
      }
      else{
	 aux=response+11;//aux apunta a X (,MAC) quitando la cabecera (r#waspmote#)

	 if(datos!=NULL)
	    strcpy(datos,aux);
      }

   readserial=0;

}

void parseFrame(struct xbee_frame *p_frame) {
   char *_Start;
   char *_End;
   int index,error=0;
   int cont=1;
   //CADENA ENVIADA: ,0013A2004030C5D7,56,34,1023,28,97 (,MAC,X,Y,Z,TEMP,BAT) ([HEADER] r#waspmote# quitado)

    _Start=aux;
    for(;;){  
    _Start=strchr(_Start,','); //devuelve puntero a ,MAC ,56 ,34 etc...
     if (_Start != NULL) {
     	_Start++; //apunta al valor de MAC, luego de X, etc... 
   	_End=strchr(_Start,',');
	if (_End!=NULL){
     	   index=_End-_Start;//index contiene la longitud de la cadena
	}else{
	   index=strchr(_Start,'\0')-_Start;
	   strncpy(p_frame->bat,_Start,index);
	   p_frame->bat[20] = '\0';
	   break; //sale del bucle for
        }

	switch(cont)
	{
		case 1: //Introducimos MAC
			strncpy(p_frame->mac,_Start,index);
			p_frame->mac[20] = '\0';
			break;//sale del switch
		case 2: //Introducimos X
			strncpy(p_frame->x,_Start,index);
			p_frame->x[20] = '\0';
			break;
		case 3: //Introducimos Y
			strncpy(p_frame->y,_Start,index);
			p_frame->y[20] = '\0';
			break;
		case 4: //Introducimos Z
			strncpy(p_frame->z,_Start,index);
			p_frame->z[20] = '\0';
			break;
		case 5: //Introducimos TEMPERATURA
			strncpy(p_frame->temp,_Start,index);
			p_frame->temp[20] = '\0';
			break;

	}
       cont++;      

     }else
       break;
   }

}

void showData(){
	   printf("\nEl valor de la MAC es: %s\n",strct_frame.mac);
	   printf("\nEl valor de la X es: %s\n",strct_frame.x);
	   printf("\nEl valor de la Y es: %s\n",strct_frame.y);
	   printf("\nEl valor de la Z es: %s\n",strct_frame.z);
	   printf("\nEl valor de la TEMPERATURA es: %sCº\n",strct_frame.temp);
	   printf("\nEl valor de la BATERIA es: %s\%\n\n",strct_frame.bat);
}

int storeInAFile(char * fileName, struct xbee_frame *p_frame, char *ip) {
   //Get date time
   time_t _time = time(0);
   struct tm *tlocal = localtime(&_time);
   char output[128];
   strftime(output,128,"%y-%m-%d %H:%M:%S",tlocal);

   char * path=(char*) malloc(200);
   strcpy(path,"/mnt/user/zigbee_data/");
   strcat(path, fileName);

   FILE *fp;
   fp = fopen ( path, "a" );
   fputs( output, fp );

   if(p_frame!=NULL){
	   fputs( " -x:", fp );
	   fputs( p_frame->x, fp );
	   fputs( " -y:", fp );
	   fputs( p_frame->y, fp );
	   fputs( " -z:", fp );
	   fputs( p_frame->z, fp );
	   fputs( " -temp:", fp );
	   fputs( p_frame->temp, fp );
	   fputs( " -bat:", fp );
	   fputs( p_frame->bat, fp );
	   fputs( "\n", fp );	   
   }

   if(ip!=NULL){
	  fputs( " ", fp );
	  fputs( ip, fp );
	  fputs(" has established connection with server", fp );
	  fputs( "\n", fp );
   }
   
   fclose ( fp );

   return 0;
}


int storeInADB(struct xbee_frame *p_frame) {
   char localDBname[100]="";
   char localDBtable[100]="";
   char localDBip[100]="";
   char localDBport[100]="";
   char localDBuser[140]="";
   char localDBpass[140]="";

   char local_DBname[100]="";
   char local_DBtable[100]="";
   char local_DBip[100]="";
   char local_DBport[100]="";
   char local_DBuser[140]="";
   char local_DBpass[140]="";

   char * YYY;

   //Init mysql database
   mysql_init(&mysql);
   mysql_options(&mysql, MYSQL_READ_DEFAULT_GROUP, "your_prog_name");

   /*
   EXAMPLE OF CONNECTION INFO FILE

      MeshliumDB
      zigbeeData
      192.168.1.19
      3306
      root
      libelium2007
   */
   FILE *file;
   file = fopen("/mnt/lib/cfg/zigbeeDBSetup","r");

   //Get connection info
   if (file != NULL)
   {
      fgets(localDBname,100,file);
      fgets(localDBtable,100,file);
      fgets(localDBip,100,file);
      fgets(localDBport,100,file);
      fgets(localDBuser,100,file);
      fgets(localDBpass,100,file);
      fclose ( file );
   }

   //Clear data of connection of '\n' chars
   YYY=strchr(localDBname, '\n');
   int reend=YYY-localDBname;
   strncpy(local_DBname,localDBname,reend);

   YYY=strchr(localDBtable, '\n');
   reend=YYY-localDBtable;
   strncpy(local_DBtable,localDBtable,reend);

   YYY=strchr(localDBip, '\n');
   reend=YYY-localDBip;
   strncpy(local_DBip,localDBip,reend);

   YYY=strchr(localDBport, '\n');
   reend=YYY-localDBport;
   strncpy(local_DBport,localDBport,reend);

   YYY=strchr(localDBuser, '\n');
   reend=YYY-localDBuser;
   strncpy(local_DBuser,localDBuser,reend);

   YYY=strchr(localDBpass, '\n');
   reend=YYY-localDBpass;
   strncpy(local_DBpass,localDBpass,reend);

   //Insert data into selected database
   if (!mysql_real_connect(&mysql, local_DBip, local_DBuser, local_DBpass, local_DBname, atoi(local_DBport), NULL, 0))
   {
      fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(&mysql));
      exit(-1);
   }
   else
   {
      fprintf(stderr, "Connected to database: return: %s\n", mysql_error(&mysql));
   }

   sprintf(sql, "insert into %s values(NULL,now(),'%s','%s','%s','%s','%s','%s');", local_DBtable, p_frame->mac, p_frame->x, p_frame->y, p_frame->z, p_frame->temp, p_frame->bat);

   if (mysql_query(&mysql, sql) != 0) {
      fprintf(stderr, "BBDDFailed:%s\n", sql);
   } else {
      fprintf(stdout, "BBDDSucceeded:%s\n", sql);
      printf("\nDatos almacenados en la base de datos: %s, tabla: %s\n",local_DBname,local_DBtable);
   }

   mysql_close(&mysql);

   return 0;
}

int main(int argc, char *argv[])
{
   aux=(char*)malloc(1000);
   char mac[16];
   char cadena1[20]="Hello mote";
   char cadena2[30]="Parameters request";
   char arch[80];
   char c;
   init(argc, argv);
   printf("\n################################BIENVENIDO################################\n\n");
   printf("\nSi desea configurar Meshlium para que un teléfono móvil se pueda conectar a él y a través de él comunicarse con los Waspmotes introduzca 'p' y pulse enter, para comunicarse directamente con los Waspmotes pulse cualquier otra tecla\n");
   c=getchar();
   if(c=='p'){
	   printf("\n\n<Meshlium ha sido configurado como interfaz de comunicación entre el teléfono móvil y los Waspmotes. A continuación se mostrará información relativa a la conexión y desconexión de los teléfonos así como la información intercambiada entre ellos>\n");

	   listenfd = socket(AF_INET, SOCK_STREAM, 0);
	   memset(&serv_addr, '0', sizeof(serv_addr));
	   memset(sendBuff, '0', sizeof(sendBuff)); 
	   memset(recvBuff, '0',sizeof(recvBuff));

	   serv_addr.sin_family = AF_INET;
	   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	   serv_addr.sin_port = htons(5000); 

	   bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

	   listen(listenfd, 10);
	   pthread_t thread[10];
	   char *clientHostLocal;
	   clientHostLocal;
	   while(1)
	   {    
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);		
		struct sockaddr clientAddress;
   		socklen_t clientAddressLength;
    		struct sockaddr_in clientAddressInternet;
    		getpeername (connfd, (struct sockaddr*)&clientAddressInternet, &clientAddressLength);
    		clientHostLocal = inet_ntoa (clientAddressInternet.sin_addr);		
		printf("\nEl teléfono con ip %s ha sido conectado\n",clientHostLocal);
		clientHost=clientHostLocal;
		storeInAFile("logs",(struct xbee_frame*)NULL,clientHost);
		if( pthread_create(&thread[i], NULL, listener, NULL ) != 0 )
		{
			printf("Cannot create thread , exiting \n ");
			exit(-1);
		}
	   }

   }else{
	   printf("BUSCANDO NODOS CONECTADOS A LA RED...\n");
	   sendBroadcast(cadena1,(char*)NULL);
	   while(1){
		   printf("\nSi desea leer parámetros de todos los nodos introduzca '0', si desea hacerlo de un nodo en particular introduzca su dirección MAC y pulse enter:\n");
		   //fgets (mac, 18, stdin);
		   //mac[strlen(mac)-1]='\0';
		   scanf("%s",mac);
		   while(getchar()!='\n');
		   fflush(stdin);
		   
			   if(!strcmp(mac,"0"))
			      sendBroadcast(cadena2,(char*)NULL);
			   else if(strlen(mac)==16){
			      sendMac(mac,cadena2,(char*)NULL);
			      if(!error){
				   parseFrame(&strct_frame);
				   showData();
			   	   printf("\nPara guardar los datos en el archivo por defecto introduzca 'g' y pulse enter, para guardarlos en un archivo nuevo introduzca un nombre de archivo, en caso contrario pulse enter\n");	
		  	           fgets (arch, 80, stdin);	
				   if(strcmp(arch,"\n")){
					   arch[strlen(arch)-1]='\0'; 	
					   if(!strcmp(arch,"g")){
					   	storeInAFile("default",&strct_frame,(char*)NULL);
						printf("\nDatos almacenados en \"/mnt/user/zigbee_data/default\"\n");
					   }else{
						storeInAFile(arch,&strct_frame,(char*)NULL);
						printf("\nDatos almacenados en \"/mnt/user/zigbee_data/%s\"\n",arch);}		   
				   }
				   printf("\nPara guardar los datos en la base de datos local introduzca 'g', en caso contrario pulse enter\n");	
		  	           c=getchar();
				   if(c=='g')
					   storeInADB(&strct_frame);
			      } 
			      error=false;
			   }else
		   		printf("\nERROR: LA DIRECCION MAC DEBE CONTENER 16 CARACTERES\n");
		
	   }
   }

   close(sd);
   exit(0);
}
