#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <regex.h>
#include <time.h>

#define VERSION		24
#define BUFSIZE		8096
#define ERROR		42
#define LOG			44
#define PROHIBIDO	403
#define NOENCONTRADO	404


struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },
	{"jpg", "image/jpg" },
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	{"ico", "image/ico" },
	{"zip", "image/zip" },
	{"gz",  "image/gz"  },
	{"tar", "image/tar" },
	{"htm", "text/html" },
	{"html","text/html" },
	{0,0} };

void debug(int log_message_type, char *message, char *additional_info, int socket_fd)
{
	int fd ;
	char logbuffer[BUFSIZE*2];

	switch (log_message_type) {
		case ERROR: (void)sprintf(logbuffer,"ERROR: %s:%s Errno=%d exiting pid=%d",message, additional_info, errno,getpid());
			break;
		case PROHIBIDO:
			// Enviar como respuesta 403 Forbidden
			(void)sprintf(logbuffer,"FORBIDDEN: %s:%s",message, additional_info);
			break;
		case NOENCONTRADO:
			// Enviar como respuesta 404 Not Found
			(void)sprintf(logbuffer,"NOT FOUND: %s:%s",message, additional_info);
			break;
		case LOG: (void)sprintf(logbuffer," INFO: %s:%s:%d",message, additional_info, socket_fd); break;
	}

	if((fd = open("webserver.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
		(void)write(fd,logbuffer,strlen(logbuffer));
		(void)write(fd,"\n",1);
		(void)close(fd);
	}
	if(log_message_type == ERROR || log_message_type == NOENCONTRADO || log_message_type == PROHIBIDO) exit(3);
}

int checkFicheroExiste(const char * fichero) //si encuentra el fichero, devuelve 0, si no existe, devuelve 1
{
    FILE *file;
    if (file = fopen(fichero, "r"))
    {
        fclose(file);
        return 0;
    }
    return 1;
}


void procesar_respuesta200OK(int descriptorFichero){
	FILE *fichero;
	size_t len=0; //
	char * line=NULL; //
	ssize_t bytes_leidos;

	char * primeraLineaRespuesta="HTTP/1.1 200 OK\r\n";
	char * lineaServer="Server: laBestia696\r\n";
	char * lineaContentType="Content-Type: text/html\r\n";
	char * lineaContentLength="Content-Length: 781\r\n"; //TODO CUIDADOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO EL NUMERO se debe calcular
	char * lineaKeepAlive="Connection: keep-alive\r\n";
	char * lineaParametrosKeepAlive="Keep-Alive: 20\r\n"; //Carefull también
	char * finPaqueteLinea="\r\n";

	int bytes_enviados=write(descriptorFichero,primeraLineaRespuesta,strlen(primeraLineaRespuesta));
	bytes_enviados=write(descriptorFichero,lineaServer,strlen(lineaServer));
//para calcular la fecha
	time_t tiempo=time(0);
	struct tm *tlocal=localtime(&tiempo);
	char salidaTiempo[128];
	strftime(salidaTiempo,128,"Date: %a, %d %b %Y %H:%M:%S GMT+1\r\n",tlocal);
	bytes_enviados=write(descriptorFichero,&salidaTiempo,strlen(salidaTiempo));
/////////////////////

	bytes_enviados=write(descriptorFichero,lineaContentType,strlen(lineaContentType));
	bytes_enviados=write(descriptorFichero,lineaContentLength,strlen(lineaContentLength));
	bytes_enviados=write(descriptorFichero,lineaKeepAlive,strlen(lineaKeepAlive));
	bytes_enviados=write(descriptorFichero,lineaParametrosKeepAlive,strlen(lineaParametrosKeepAlive));
	bytes_enviados=write(descriptorFichero,finPaqueteLinea,strlen(finPaqueteLinea));

	//Al acabar cabecera, enviamos el fichero html:
	fichero=fopen("index.html","r");
	while((bytes_leidos=getline(&line, &len, fichero))!=-1){
		int bytes_enviados=write(descriptorFichero, line,strlen(line));
	}
	fclose(fichero);
	close(descriptorFichero);

}

void procesarErrorBadRequest(int descriptorFichero){
	char * primeraLineaRespuesta="HTTP/1.1 400 Bad Request\r\n";
	char * lineaServer="Server: laBestia696\r\n";
	char * lineaContentType="Content-Type: text/html\r\n";
	char * lineaContentLength="Content-Length: 781\r\n"; //TODO CUIDADOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO EL NUMERO se debe calcular
	char * lineaKeepAlive="Connection: keep-alive\r\n";
	char * lineaParametrosKeepAlive="Keep-Alive: 20\r\n"; //Carefull también
	char * finPaqueteLinea="\r\n";

	int bytes_enviados=write(descriptorFichero,primeraLineaRespuesta,strlen(primeraLineaRespuesta));
	bytes_enviados=write(descriptorFichero,lineaServer,strlen(lineaServer));
//para calcular la fecha
	time_t tiempo=time(0);
	struct tm *tlocal=localtime(&tiempo);
	char salidaTiempo[128];
	strftime(salidaTiempo,128,"Date: %a, %d %b %Y %H:%M:%S GMT+1\r\n",tlocal);
	bytes_enviados=write(descriptorFichero,&salidaTiempo,strlen(salidaTiempo));
/////////////////////
	bytes_enviados=write(descriptorFichero,lineaContentType,strlen(lineaContentType));
	bytes_enviados=write(descriptorFichero,lineaContentLength,strlen(lineaContentLength));
	bytes_enviados=write(descriptorFichero,lineaKeepAlive,strlen(lineaKeepAlive));
	bytes_enviados=write(descriptorFichero,lineaParametrosKeepAlive,strlen(lineaParametrosKeepAlive));
	bytes_enviados=write(descriptorFichero,finPaqueteLinea,strlen(finPaqueteLinea));
	//close(descriptorFichero);
}

void procesarLogo(int descriptorFichero){
	FILE *fichero;
	size_t len=0; //
	char * line=NULL; //
	ssize_t bytes_leidos;

	char * primeraLineaRespuesta="HTTP/1.1 200 OK\r\n";
	char * lineaServer="Server: laBestia696\r\n";
	char * lineaContentType="Content-Type: image/jpg\r\n";
	char * lineaContentLength="Content-Length: 9781\r\n"; //TODO CUIDADOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO EL NUMERO se debe calcular
	char * lineaKeepAlive="Connection: keep-alive\r\n";
	char * lineaParametrosKeepAlive="Keep-Alive: 20\r\n"; //Carefull también
	char * finPaqueteLinea="\r\n";

	int bytes_enviados=write(descriptorFichero,primeraLineaRespuesta,strlen(primeraLineaRespuesta));
	bytes_enviados=write(descriptorFichero,lineaServer,strlen(lineaServer));
//para calcular la fecha
	time_t tiempo=time(0);
	struct tm *tlocal=localtime(&tiempo);
	char salidaTiempo[128];
	strftime(salidaTiempo,128,"Date: %a, %d %b %Y %H:%M:%S GMT+1\r\n",tlocal);
	bytes_enviados=write(descriptorFichero,&salidaTiempo,strlen(salidaTiempo));
/////////////////////

	bytes_enviados=write(descriptorFichero,lineaContentType,strlen(lineaContentType));
	bytes_enviados=write(descriptorFichero,lineaContentLength,strlen(lineaContentLength));
	bytes_enviados=write(descriptorFichero,lineaKeepAlive,strlen(lineaKeepAlive));
	bytes_enviados=write(descriptorFichero,lineaParametrosKeepAlive,strlen(lineaParametrosKeepAlive));
	bytes_enviados=write(descriptorFichero,finPaqueteLinea,strlen(finPaqueteLinea));

	//Al acabar cabecera, enviamos el fichero html:
	char buffer[BUFSIZE*2];
	fichero=fopen("logo-um.jpg","r");


	fscanf(fichero,"%s",buffer);
	strcat(buffer,"\0");

	bytes_enviados=write(descriptorFichero, buffer,strlen(buffer));


	// while((bytes_leidos=getline(&line, &len, fichero))!=-1){
	// 	int bytes_enviados=write(descriptorFichero, line,strlen(line));
	// }


	fclose(fichero);
	close(descriptorFichero);

}

void comprobarExpresion(int descriptorFichero, char * line){
	regex_t regex; //variable donde crearemos regex
	int valorComprobacion=1;
	int valorComprobacion2=1;
	valorComprobacion=regcomp(&regex,"GET \/index.html HTTP\/1.1\r\n",0);
	valorComprobacion2=regcomp(&regex,"GET \/ HTTP\/1.1\r\n",0);
	//
	// if(valorComprobacion==0){
	// 	printf("funciona la ER\n");
	// }

	valorComprobacion=regexec(&regex,"GET \/index.html HTTP\/1.1\r\n",0,NULL,0);
	valorComprobacion2=regexec(&regex,"GET \/ HTTP/1.1\r\n",0,NULL,0);


	char * getIndex="GET /index.html HTTP/1.1";
	char * getSinIndex="GET / HTTP/1.1";
	char * getLogo="GET /logo-um.jpg HTTP/1.1";
	//if(valorComprobacion==0 || valorComprobacion2==0){
	//(PRIMER GET)
	if(strcmp(line,getIndex)==0 || strcmp(line,getSinIndex)==0){
//		printf("ha hecho match\n");//----------------------------------------------------
//		line=line+'\0';
		procesar_respuesta200OK(descriptorFichero);
		return;
	}
	//PETICION DE LOGO
	if(strcmp(line,getLogo)==0){
		line=line+'\0';
		printf("me pide el logo sin problema y habrá que hacer el metodo\n");
		//TODO procesar el logo y enviarlo
		procesarLogo(descriptorFichero);

		return;
	}
	//CUANDO NO EMPIEZA POR GET
	valorComprobacion=regexec(&regex,"^GET",0,NULL,0);
	if(valorComprobacion==1){
		printf("no hace match\n");
		procesarErrorBadRequest(descriptorFichero);
		return;
	}
	//
	// else{
	// 	printf("no hace match\n");
	// 	valorComprobacion=regexec(&regex,"^GET",0,NULL,0);
	// 	if (valorComprobacion==1 || valorComprobacion2==1){
	// 		procesarErrorBadRequest(descriptorFichero);
	// 	}
	//}
}

void process_web_request(int descriptorFichero)
{
	debug(LOG,"request","Ha llegado una peticion",descriptorFichero);
	//
	// Definir buffer y variables necesarias para leer las peticiones
	//
	char buffer[BUFSIZE];
	//uint_8
	//
	// Leer la petición HTTP
	//
	ssize_t bytes_read;
	size_t len=0;
	char * line=NULL;
	int contador=0;
	FILE *fd;
	char aux[BUFSIZE];
	char delimitador[]="\r\n";


	fd=fdopen(descriptorFichero,"r");
	// while ((bytes_read=getline(&line, &len, fd))!=-1){ //&& contador!=1) {
	// 	printf("%s", line);
	// 	//comprobarExpresion(descriptorFichero, line);
	// 	//contador++;

	contador=read(descriptorFichero,buffer,BUFSIZE);
	//printf("%s",buffer);
	char *token=strtok(buffer,delimitador);
	while(token!=NULL){
		comprobarExpresion(descriptorFichero,token); //se pasan todas las lineas;
		printf("%s\n", token);
		token=strtok(NULL,delimitador); //solo se pasa una vez el buffer

	}
	// }

	//
	// Comprobación de errores de lectura
	//


	//
	// Si la lectura tiene datos válidos terminar el buffer con un \0
	//


	//
	// Se eliminan los caracteres de retorno de carro y nueva linea
	//


	//
	//	TRATAR LOS CASOS DE LOS DIFERENTES METODOS QUE SE USAN
	//	(Se soporta solo GET)
	//


	//
	//	Como se trata el caso de acceso ilegal a directorios superiores de la
	//	jerarquia de directorios
	//	del sistema
	//


	//
	//	Como se trata el caso excepcional de la URL que no apunta a ningún fichero
	//	html
	//


	//
	//	Evaluar el tipo de fichero que se está solicitando, y actuar en
	//	consecuencia devolviendolo si se soporta u devolviendo el error correspondiente en otro caso
	//


	//
	//	En caso de que el fichero sea soportado, exista, etc. se envia el fichero con la cabecera
	//	correspondiente, y el envio del fichero se hace en blockes de un máximo de  8kB
	//

	close(descriptorFichero);
	exit(1);
}

int main(int argc, char **argv)
{
	int i, port, pid, listenfd, socketfd;
	socklen_t length;
	static struct sockaddr_in cli_addr;		// static = Inicializado con ceros
	static struct sockaddr_in serv_addr;	// static = Inicializado con ceros
	fd_set rfds;
	struct timeval tv;
	int retval;


	FD_ZERO(&rfds);
	FD_SET(0,&rfds);

	tv.tv_sec=5;
	tv.tv_usec=0;
	//  Argumentos que se esperan:
	//
	//	argv[1]
	//	En el primer argumento del programa se espera el puerto en el que el servidor escuchara
	//
	//  argv[2]
	//  En el segundo argumento del programa se espera el directorio en el que se encuentran los ficheros del servidor
	//
	//  Verficiar que los argumentos que se pasan al iniciar el programa son los esperados
	//

	//
	//  Verficiar que el directorio escogido es apto. Que no es un directorio del sistema y que se tienen
	//  permisos para ser usado
	//

	if(chdir(argv[2]) == -1){
		(void)printf("ERROR: No se puede cambiar de directorio %s\n",argv[2]);
		exit(4);
	}
	// Hacemos que el proceso sea un demonio sin hijos zombies
	if(fork() != 0)
		return 0; // El proceso padre devuelve un OK al shell

	(void)signal(SIGCHLD, SIG_IGN); // Ignoramos a los hijos
	(void)signal(SIGHUP, SIG_IGN); // Ignoramos cuelgues

	debug(LOG,"web server starting...", argv[1] ,getpid());

	/* setup the network socket */
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0)
		debug(ERROR, "system call","socket",0);

	port = atoi(argv[1]);

	if(port < 0 || port >60000)
		debug(ERROR,"Puerto invalido, prueba un puerto de 1 a 60000",argv[1],0);

	/*Se crea una estructura para la información IP y puerto donde escucha el servidor*/
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /*Escucha en cualquier IP disponible*/
	serv_addr.sin_port = htons(port); /*... en el puerto port especificado como parámetro*/

	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		debug(ERROR,"system call","bind",0);

	if( listen(listenfd,64) <0)
		debug(ERROR,"system call","listen",0);

	while(1){
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			debug(ERROR,"system call","accept",0);
		if((pid = fork()) < 0) {
			debug(ERROR,"system call","fork",0);
		}
		else {
			if(pid == 0) { 	// Proceso hijo
				(void)close(listenfd);
				process_web_request(socketfd); // El hijo termina tras llamar a esta función

				retval=select(socketfd,&rfds,NULL,NULL,&tv);

				if(!retval){
					close(socketfd);
				}
			} else { 	// Proceso padre
				(void)close(socketfd);
			}
		}
	}
}
