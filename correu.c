//My first POP3 client. Any: 1998 ?

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>	/*Poder col.locar nom de servidor i no solament IP*/

#define TAMMAIL 750
char buffer[TAMMAIL];
char mark[]="-------------\n";
char fitxer[50]="/etc/correu.conf";

struct direccio{
	char servidor[50];
	int port;
	char usuari[50];
	char password[50];
	char arxiu[50];	/*Arxiu on guardem els mails */
	char borrar;	/* Esborro o no els mails */

} correu;

/* Descriptor del fitxer de configuració i de mails*/
FILE *con,*fp;

/* Elements de red*/
int sock, c;
struct sockaddr_in server;
struct hostent *serv;

void error(const char* miss)
{
	fputs("Error:",stdout);
	fputs(miss,stdout);
	exit(0);
}

/* Aquesta funció llegeix una linia del socket( fins a trobar linefeed)*/
/* Thanks to Socket Faq list */
void sockgets(void)
{
int bytes_read;
int total_count=0;
char *current_position;
char last_read = 0;

current_position=buffer;
while(last_read != '\n') {
	bytes_read = read(sock,&last_read,1);
	if(bytes_read <= 0 ) { error("En sockgets\n"); }
	if(total_count < TAMMAIL)
		{
		current_position[0]=last_read;
		current_position++;
		total_count++;
		}
	}
current_position[0]='\0';
}

void autentifica(void) {

	/* llegeixo el missatge inicial*/  		
	sockgets();
	 if( strncmp(buffer,"+OK",3) != 0 )
	 	 error("Server no preparat\n");
			
	fputs(buffer,stdout);

	 /*Nom d'usuari */	 	
	 sprintf(buffer,"user %s\n",correu.usuari);
	 send(sock,buffer,strlen(buffer),0);
	sockgets();
	 if( strncmp(buffer,"+OK",3) != 0 )
	 	{ fprintf(stderr,"Nom d'usuari incorrecte!\n"); close(sock);
				fclose(con); exit(0); }
	fputs(buffer,stdout);
       	
       	 	
         /*Password */
	 sprintf(buffer,"pass %s\n",correu.password);
	 send(sock,buffer, strlen(buffer),0);
		sockgets();
       	 if( strncmp(buffer,"+OK",3) != 0 )
	 	{ fprintf(stderr,"Password incorrecte!\n"); close(sock);
				fclose(con); exit(0); }
        	
}

/* Saber el nombre de missatges del servidor */
int nmissatges(void)
{
int n,tam;

	send(sock,"stat\n", 5,0);
	sockgets();
	if ( (sscanf(buffer,"+OK %i %i\n",&n,&tam)) == 2 ) {
 		fprintf(stdout,"\nTinc %i missatges\n",n);
	 	return n;
 	
 	} else {
 		fprintf(stderr,"Error en STAT\n");
		fclose(con);
		close(sock);
 		return 0;
 		}
}

void confserver(void)
{
	fgets(buffer,TAMMAIL,con);
	sscanf(buffer,"SERVIDOR=%s\n",correu.servidor);
	fgets(buffer,TAMMAIL,con);
	sscanf(buffer,"PORT=%i\n",&correu.port);
	fgets(buffer,TAMMAIL,con);
	sscanf(buffer,"USUARI=%s\n",correu.usuari);
	fgets(buffer,100,con);
	sscanf(buffer,"PASSWD=%s\n",correu.password);
	fgets(buffer,100,con);
	sscanf(buffer,"FITXER=%s\n",correu.arxiu);
	fgets(buffer,100,con);
	sscanf(buffer,"BORRA=%c\n", &correu.borrar);
	fgets(buffer,100,con);	/* Linia de separacio */
}

void borramail(int x)
{
	sprintf(buffer,"dele %i\n",x);
	send(sock,buffer,strlen(buffer),0);
		sockgets();
	if( strncmp(buffer,"+OK",3) != 0 )
		{ printf("Problemes borrant missatge %i\n",x); exit(0); }
	fprintf(stdout,"Mail borrat %i\n",x);
}

	
void fisessio(void) {

	send(sock,"quit\n",5,0);
		sockgets();
	fputs(buffer,stdout);
	 }
	
/*	RECEPCIO DE CADA MAIL  */

void rebremail(int x)
{
	int aux,tamany,t;

	/*fputs(mark,fp);*/
        sprintf(buffer,"list %i\n",x);
	send(sock,buffer,strlen(buffer),0);
		sockgets();
	assert ((sscanf(buffer,"+OK %i %i\n", &aux, &tamany)) == 2 );
	
	sprintf(buffer,"retr %i\n",x);
	send(sock,buffer,strlen(buffer),0);

		/* primera linia de retr --> +OK 1 3456 */
		sockgets();
	if (sscanf(buffer,"+OK %i octets\n", &t) != 1)
			 	 { fprintf(stderr,"Problemes en RETR\n"); exit(0); }
	if ( t != tamany )  	{ fprintf(stderr,"Tamanys differents..-->>ERROR estrany!\n"); exit(0); }
				 	
 	 /*	MAIL */
	while(tamany > 0)
		{
		if(tamany > TAMMAIL) {
			c = recv(sock,(void *)buffer, TAMMAIL, 0);
			tamany = tamany - c;
				}
		else {
			c = recv(sock,(void *)buffer, tamany, 0);
			tamany = tamany - c;
			}
		buffer[c] = '\0';
		fputs(buffer,fp);
		}
					
	 /*	PUNT FINAL */
		sockgets();
	fprintf(stdout,"Fi mail %i\n",x);
	
		
	}
/*********************************************************/

int fitxerconf(void)
{
int n;

if(!(con = fopen(fitxer,"r")))
	{ fprintf(stderr,"Error: No puc obrir fitxer de configuració\n");
		exit(0); }

fgets(buffer,TAMMAIL,con);
	
if (sscanf(buffer,"#SERVIDORS=%i\n",&n) != 1)
	{ fprintf(stderr,"#SERVIDORS no definit\n"); fclose(con); exit(0);}
	
fgets(buffer,TAMMAIL,con);	/*linia de separacio*/
	
return n;
}	


void conexio(void)
{
	/*obrir el socket */
	sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock < 0)
		{fprintf(stderr,"Error:(0) %s\n",strerror(errno));
			fclose(con); fclose(fp); exit(0);}

	/*armem servidor */
	bzero((char *) &server, sizeof(server));
	server.sin_family= AF_INET;
	server.sin_addr.s_addr = inet_addr(correu.servidor);
	if(server.sin_addr.s_addr == -1)
		{
		serv=gethostbyname(correu.servidor);
		if(serv !=NULL)
			{
bcopy(serv->h_addr, (char *) &server.sin_addr.s_addr, serv->h_length);
			}
		else {
			fprintf(stdout,"Host desconegut!\n"); fclose(con);
			close(sock); exit(0);
			}
		
		}

	
	server.sin_port = htons(correu.port);
	
	/*establir la conexio*/
	if ( (connect(sock,(struct sockaddr *)& server,sizeof(server))) < 0 )
		{
		fprintf(stderr,"Error:(1) %s\n",strerror(errno));
		close(sock);
		fclose(con);
		exit(0);
		}
}


	
int main(int argc,char *argv[])
{
int i , nombre=0, k, nservers,opt, c;

for( opt = 1; opt < argc ; opt ++)
	{
	c = getopt(argc,argv,"hc:");
	switch(c) {
		case('h'):
			fprintf(stdout,"Aqui printo l'ajuda \n");
			exit(0);
			break;
		case('c'):
			fprintf(stdout,"Vols utilitzar com a fitxer de configuració %s\n",optarg);
			exit(0);
			break;
		default:
			fprintf(stdout,"Paràmetre incorrecte\n");
			exit(0);
			break;
		}
			
	}

nservers = fitxerconf();

for(k = 1; k <= nservers; k++) {

	confserver();
	
	conexio();	
	
	autentifica();
		
	nombre = nmissatges();
	
	if(nombre > 0)
		{
		if(!(fp = fopen(correu.arxiu,"a")))
			{ fprintf(stderr,"Error: %s \n",strerror(errno)); exit(0); }		
		}
		
  	for(i = 1; i <= nombre; i++)
 		{
                rebremail(i);
		if(correu.borrar == 'S')  borramail(i);
       	        }
  	
        fisessio();
	close(sock);
	
	if(nombre > 0) 	fclose(fp);

	}
        fclose(con);
	return 0;
}
