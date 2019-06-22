#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "funcionesAuxiliares.c"

//------VARS GLOBALES------
int burstMin;			//burstMinimo especificado por usuario
int burstMax;			//burstMaximo especificado por usuario

int tasaProcesos;	   	/*Usado para saber cada cuantos segundos crear un proceso
						en modo automatico*/

int continuar = 1;  	/*Usado para determinar si debe continuar operando el cliente
						1: True, 0: False
						*/

char * buffer;			/*Usado por el hilo de escuchar servidor para
						guardar mensajes*/

pthread_t hiloCrear;	//Hilo que crea procesos aleatorios
pthread_t hiloEscuchar; //hilo que escucha los mensajes del servidor
pthread_t hiloRecibe;	//hilo que espera input del usuario para determinar exit


struct sockaddr_in direccionServidor; //socket para cliente
int cliente;			//puntero para mandar msg por socket

FILE *fp;



void* crearProcesoAleatorio(void* vargp)
/*Funcion de hilo que crea procesos cada tasaProcesos indicado por modo automatico
 *Entradas: ninguna
 *Salidas: ninguna
 *Restricciones: Se debe haber levantado el servidor primero
 */
{
	printf("Entre a crear procesos\n\n\n");
	int i = 0 ;
	while (continuar)
	{

		int burst = burstMin + rand() % (burstMax + 1 - burstMin);
		int prioridad = 1 + rand() % (11 - 1);
		char strProceso[] = "P";
		char strBurst[20];
		char strPrioridad[20];

		if (miItoa(burst, strBurst) != NULL && miItoa(prioridad, strPrioridad) != NULL)
		{
			strcat(strProceso, strBurst);
			strcat(strProceso, ",");
			strcat(strProceso, strPrioridad);
			printf("Vamos a mandar: %s\n", strProceso);

			send(cliente, strProceso, strlen(strProceso), 0);

			sleep(tasaProcesos);
		}
	}
	return NULL;
}



void* escucharServidor(void* argp)
/*Funcion de hilo que escucha los mensajes que manda el servidor
 *Entradas: ninguna
 *Salidas: El mensaje que recibio del servidor se muestra en pantalla
 *Restricciones: Se debe haber levantado el servidor primero
 */
{
	buffer = malloc(1000);
	while (continuar)
	{
		int bytesRecibidos = recv(cliente, buffer, 1000, 0);
		if (bytesRecibidos <= 0 || !continuar)
		{
			perror("El servidor se desconecto\n");
			break;
		}

		buffer[bytesRecibidos] = '\0';
		printf("ID proceso: %s\n", buffer);
	}
	free(buffer);
	return NULL;
}



void* esperarEntrada(void* argp)
/*Funcion de hilo que espera 'exit' para terminar el programa
 *Entradas: ninguna
 *Salidas: ninguna
 *Restricciones: Se debe haber levantado el servidor primero
 */
{
	char mensaje[1000];
	while (continuar)
	{
		fgets(mensaje, 1000, stdin);
		mensaje[ strlen(mensaje) - 1] = '\0';
		printf("Mi msg es: %s\n", mensaje);

		if (strcmp(mensaje, "exit") == 0)
		{
			printf("Debemos parar\n");
			strcpy(mensaje, "EXIT");
			continuar = 0;
			break;
		}
	}
	return NULL;
}



void procesarLineaArchivo( char linea[])
/*Funcion que procesa linea de archivo y manda a crear un proceso
 * con los datos procesados
 *Entradas: la linea leida del archivo
 *Salidas: ninguna o un mensaje de error
 *Restricciones: ninguna
 */
{
	int burstProceso;
	int prioridadProceso;

	char strProceso[] = "P";
	char strNumero[100];
	char strBurst[100];
	char strPrioridad[100];
	int i = 0;
	int j = 0;
	strcpy(strNumero, "");

	while (!isspace(linea[i]))
	{
		strNumero[i] = linea[i];
		i++;
	}
	strNumero[i] = '\0';
	i++;

	if (esNumerico(strNumero) )
	{
		burstProceso = atoi(strNumero);
		if (burstProceso >= burstMin && burstProceso <= burstMax)
		{

			strcat(strProceso, strNumero);
			strcat(strProceso, ",");

			strcpy(strNumero, "");

			while (!isspace(linea[i]))
			{
				strNumero[j] = linea[i];
				i++;
				j++;
			}
			strNumero[j] = '\0';
			if (esNumerico(strNumero))
			{
				prioridadProceso = atoi(strNumero);
			}else{
				prioridadProceso = 5;
			}
			miItoa(prioridadProceso, strPrioridad);
			strcat(strProceso, strPrioridad);
			send(cliente, strProceso, strlen(strProceso), 0);
		}
	}
}



void* leerArchivoProcesos(void* vNombreArchivo )
/*Funcion que carga lo que le de una linea de arhivo en un buffer y
 * luego lo manda a procesar
 *Entradas: Nombre de archivo
 *Salidas: ninguna o un mensaje de error
 *Restricciones: ninguna
 */
{

	char *nombreArchivo = (char *)vNombreArchivo;
	int i;
	char buffer[1000];
	char c;

	if (access(nombreArchivo, F_OK) == -1)
	{
		printf("Oops parece que ese archivo no pudo ser accesado\n");
		continuar = 0;
		return NULL;

	}else{
			fp = fopen(nombreArchivo, "r");
			c = getc(fp);
			i = 0;
			strcpy(buffer, "");

			while (c != EOF && continuar)
			{
				while (c != '\n')
				{
					buffer[i] = c;
					i++;
					c = getc(fp);
				}
				buffer[i] = '\0';
				if (continuar)
				{
					printf("Analizando linea de archivo...\n");
					procesarLineaArchivo(buffer);
					int tiempoEspera = 3 + rand() % (5);
					sleep(tiempoEspera);
				}else{
					break;
				}
				strcpy(buffer, "");
				i = 0;
				c = getc(fp);
			}
			printf("He finalizado de leer el archivo\n");
			fclose(fp);
			return NULL;
	}

}



int conectar()
/*Funcion que establece conexion con servidor por medio de socket
 *Entradas: ninguna
 *Salidas: Un entero que indica si la conexion fue exitosa o no
 *Restricciones: ninguna
 */
{
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(8080);
    cliente = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(cliente, (void* ) &direccionServidor, sizeof(direccionServidor)) != 0)
	{
		perror("No se pudo conectar");
		return 0;
	}
	printf("Cliente se ha conectado con servidor exitosamente!\n");
	return 1;
}



void cargarParametros()
/*Funcion que pide al usuario el burst minimo y maximo para el funcionamiento del programa
 *Entradas: ninguna
 *Salidas: ninguna
 *Restricciones: Se debe haber levantado el servidor primero
 * Nota: modifica estado de variable global continuar en caso de error
 */
{
	char bufferMin[50];
	char bufferMax[50];
	printf("Ingrese el valor minimo para los bursts de procesos: ");
	fgets(bufferMin, 50, stdin);
	bufferMin[ strlen(bufferMin) - 1] = '\0';

	printf("Ingrese el valor maximo para los bursts de procesos: ");
	fgets(bufferMax, 50, stdin);
	bufferMax[ strlen(bufferMax) - 1] = '\0';

	if (esNumerico(bufferMin) && esNumerico(bufferMax))
	{
		burstMin = atoi(bufferMin);
		burstMax = atoi(bufferMax);
		if (burstMin >= burstMax)
		{
			printf("El rango minimo debe ser menor que el rango maximo\n");
			continuar = 0;
		}
	}else{
			printf("Whoops parece que tenemos un problema con los valores ingresados :/\n");
			printf("Asegurate de ingresar valores numericos positivos y enteros \n");
			continuar = 0;
	}
}



int cargarMenu()
/*Funcion que muestra un menu en pantalla y
 * Pide al usuario en que modo debe operar el servidor
 *Entradas: ninguna
 *Salidas: Un enetero que retorna opcion seleccionada por el usuario
 *Restricciones: ninguna
 */
{
	if (continuar)
	{
		char opcion[50];
		printf("======Simulador de CPU scheduling v1.0======\n");
		printf("1. Modo manual\n");
		printf("2. Modo automatico\n");
		printf("Seleccione una opcion: ");
		fgets(opcion, 50, stdin);
		opcion[ strlen(opcion) - 1] = '\0';

		if (esNumerico(opcion))
		{
				int modo = atoi(opcion);
				if (modo == 1 || modo == 2)
				{
					return modo;

				}else{
					printf("Whoops esa opcion no esta dentro las incluidas en el programa\n");
				}
		}else{
			printf("Whoops parece que no podemos procesar esa opcion\n");
			exit(0);

		}
		continuar = 0;
	}
	return 0;
}



void cargarModoManual()
/*Funcion que carga modo manual donde se pide un nombre de archivo y se
 *  manda al servidor y se crea el hilo de escuchar mensajes
 *Entradas: ninguna
 *Salidas: ninguna
 *Restricciones: ninguna
 */
{
	char nombreArchivo[20];
	printf("Ingrese el nombre del archivo: ");
	fgets(nombreArchivo, 20, stdin);
	nombreArchivo[ strlen(nombreArchivo) - 1] = '\0';

	char* copiaArchivo = strdup(nombreArchivo);
	pthread_create(&hiloEscuchar, NULL, escucharServidor, NULL);
	pthread_create(&hiloCrear, NULL, leerArchivoProcesos, (void *) copiaArchivo);
}



void cargarModoAutomatico()
/*Funcion que carga modo automatico donde se pide la tasa de creacion
 * de procesos y le manda un mensaje al servidor
 *Entradas: ninguna
 *Salidas: un mensaje que muestra cada cuanto se crea un proceso
 * o un mensaje de error
 *Restricciones: ninguna
 */
{
	char entrada[20];
	printf("Ingrese el numero de segundos (entero) para la tasa de creacion: ");
	fgets(entrada, 20, stdin);
	entrada[ strlen(entrada) - 1] = '\0';
	if (esNumerico(entrada))
	{
		tasaProcesos = (int) atoi(entrada);
		printf("Vamos a crear procesos cada %d segundos\n", tasaProcesos);

		pthread_create(&hiloEscuchar, NULL, escucharServidor, NULL);
		pthread_create(&hiloCrear, NULL, crearProcesoAleatorio, NULL);


	}else{
		printf("Whoops parece que hubo un problema leyendo el numero de la entrada\n");
		continuar = 0;
	}
}



void cargarHiloInput()
/*Funcion que crea el hilo que se encarga de recibir input del usuario
 *Entradas: ninguna
 *Salidas: el hilo creado
 *Restricciones: ninguna
 */
{
	pthread_create(&hiloRecibe, NULL, esperarEntrada, NULL);
	pthread_join(hiloRecibe, NULL);
}

int main()
{
	if (conectar())
	{
		cargarParametros();
		int opcion = cargarMenu();


		if (opcion == 1)
		{
			cargarModoManual();
		}else if (opcion == 2)
		{
			cargarModoAutomatico();
		}

		//cargarHiloInput();
	}
	char entrada[100];
	int salida = 0;
	while(salida == 0){
		scanf("%s",entrada);
		if(strcmp(entrada,"S")==0){
			salida = 1;
		}
	}
	return 0;

}
