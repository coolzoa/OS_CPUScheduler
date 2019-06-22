#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "funcionesAuxiliares.c"
#include "listaEspera.c"

nodo* inicio = NULL;
nodo* finalizado = NULL;
nodo* tmp = NULL;


//------VARS GLOBALES------
pthread_t hiloCPUscheduler;	//Hilo que ejecuta los procesos
pthread_t hilosJOBscheduler[100];	//Hilo que carga los procesos
pthread_t hiloClientes;	//Hilo que administraClientes

struct sockaddr_in direccionServidor;	//socket para servidor
int servidor;							//Puntero de servidor
int activado;							//Sirve para no tener que esperar 2 min a que socket
										//Se tenga que volver a levantar en siguiente corrida

int processId = 0;						//Id para asignar a un proceso durante creacion
int permitidoOperar = 1;
int modoOperacionJobScheduler = 0; 		//1: Manual 2: Automatico

int burstMin;
int burstMax;
int opcionAlgoritmo;
int quantum;

struct sockaddr_in direccionCliente;	//Usado para comunicacion entre sockets



int setup()
/*Funcion que monta el servidor y socket
 *Entradas: ninguna
 *Salidas: ninguna
 *Restricciones: ninguna
 */
{
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(8080); //puerto en donde va a escuchar nuestro servidor

	//creamos un socket que usa protocolo TCP
	servidor = socket(AF_INET, SOCK_STREAM, 0);
	activado = 1;
	setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado)); //es para que si se apaga el server, pueda
																			   //levantarlo sin tener que esperar

	//hay que validar bind, si falla tira distinto de 0
	if (bind(servidor, (void*) &direccionServidor, sizeof(direccionServidor)) != 0)
	{
			perror("Fallo el bind del socket");
			return 1;
	}

	listen(servidor, 100); //vamos a tener una cola de clientes maximo de 100



	return 0;
}



void cargarDatosBurst(char buffer[])
/*Funcion que saca informacion de burst minimo y maximo
 *Entradas: el buffer
 *Salidas: ninguna
 *Restricciones: ninguna
 */
{
	int i, j;
	j = 0;

	char strNumero[10];

	int largo = (int) strlen(buffer);
	for (i = 1; buffer[i] != ',' ; i++)
	{
		strNumero[i - 1] = buffer[i];
	}
	strNumero[i] = '\0';
	burstMin = atoi(strNumero);

	strcpy(strNumero, "");

	i++;

	while ( i < largo)
	{
		strNumero[j] = buffer[i];
		i++;
		j++;
	}
	strNumero[j] = '\0';
	burstMax = atoi(strNumero);
}



void agregarProcesoCola(int processId, int burst, int prioridad)
{
	if (permitidoOperar)
	{
		time_t tiempo;
		time(&tiempo);
		nodo* nuevo = crearNodo(processId, burst, prioridad, NULL,tiempo,0,0);
		if (inicio == NULL)
		{
			inicio = nuevo;
		}else{
			inicio = appendTiempo(inicio, processId, burst, prioridad,tiempo);
		}
		//mostrarLista(inicio);

	}

}

void agregarProcesoColaFin(int processId, int burst, int prioridad,double tat,
	double wt)
{
	if (permitidoOperar)
	{
		nodo* nuevo = crearNodo(processId, burst, prioridad, NULL,0,tat,wt);
		if (finalizado == NULL)
		{
			finalizado = nuevo;
		}else{
			finalizado = appendFinal(finalizado, processId, burst, prioridad,0,tat,wt);
		}
		//mostrarLista(inicio);

	}

}


void agregarProceso(int burst, int prioridad, int clienteActual)
/*Funcion que agrega nuevo proceso en lista enlazada y manda msg de
 * confirmacion
 *Entradas: un burst y prioridad
 *Salidas: ninguna
 *Restricciones: ninguna
 */
{
 //printf("Debo crear un proceso con %d, %d\n", burst, prioridad);
 agregarProcesoCola(processId, burst, prioridad);
 char mensaje[] = "";
 char strNumero[30];
 strcat(mensaje, miItoa(processId++, strNumero));
 strcat(mensaje, " fue agregado con exito\n");
 send(clienteActual, mensaje, strlen(mensaje), 0);
}







void procesarProceso( char buffer[],int clienteActual)
/*Funcion que extrae burst y pioridad de buffer recibido
 *Entradas: Buffer con formato P%s,%s
 *Salidas: ninguna
 *Restricciones: ninguna
 */
{
	//empezamos en 1 porque el buffer[0] es igual a 'P'
	int indice = 1;
	int indiceAux = 0;
	char entrada[10];
	while (buffer[indice] != ',')
	{
			entrada[indiceAux] = buffer[indice];
			indice++;
			indiceAux++;
	}
	entrada[indiceAux] = '\0';
	indice++;
	int burst = atoi(entrada);
	strcpy(entrada, "");
	indiceAux = 0;

	while (buffer[indice] != '\0')
	{
		entrada[indiceAux] = buffer[indice];
		indiceAux++;
		indice++;
	}
	entrada[indiceAux] = '\0';
	int prioridad = atoi(entrada);
	agregarProceso(burst, prioridad,clienteActual);
}



int cargarMenuAlgoritmos()
/*Funcion que muestra menu e algoritmos y pide opcion del usuario
 *Entradas: ninguna
 *Salidas: Mensaje que indica si se eleigio correctamente o error
 *Restricciones: ninguna
 */
{
	char buffer[20];
	printf("%s", limpiarPantalla);
	printf("1. First Comes First Served - FCFS\n");
	printf("2. Shortest Job First - SJF\n");
	printf("3. Highest Priority First - HPF\n");
	printf("4. Round Robin - RR\n");
	printf("Seleccione la opcion de que algoritmo debe correr el simulador: ");
	fgets(buffer, 20, stdin);
	buffer[strlen(buffer) - 1] = '\0';
	if (esNumerico(buffer))
	{
		opcionAlgoritmo = atoi(buffer);
		if (opcionAlgoritmo >= 5 || opcionAlgoritmo == 0)
		{
			printf("Oops parece que el dato ingresado es invalido\n");
			permitidoOperar = 0;
			return 0;
		}
		if (opcionAlgoritmo == 4)
		{
			strcpy(buffer, "");
			printf("Ingrese el quantum de tiempo en segundos: ");
			fgets(buffer, 20, stdin);
			buffer[strlen(buffer) - 1] = '\0';
			if (esNumerico(buffer))
			{
					quantum = atoi(buffer);
					printf("Configurando servidor con algoritmo especificado...\n");
					return 1;
			}else{
				printf("oops parece que el dato ingresado no es valido\n");
				permitidoOperar = 0;
				return 0;
			}
		}else{
			printf("Configurando servidor con algoritmo especificado...\n");
			return 1;
		}
	}else{
		printf("Oops parece que esa opcion no es valida\n");
		permitidoOperar = 0;
		return 0;
	}
}


void* CPUscheduler(void* vargp)
{
	int espera=0;
	int id=0;
	int burst=0;
	int prioridad = 0;
	double tat = 0;
	double wt = 0;
	printf("CPUscheduler iniciado con exito\n");
	while(1){
		if(inicio != NULL)
		{
			//FIFO
			if(opcionAlgoritmo==1)
			{

				printf("Ejecutando proceso %d, con burst de %d y prioridad %d\n",
					inicio->id,inicio->burst,inicio->prioridad);
					espera = inicio->burst;
					id = inicio->id;
					burst = inicio->burst;
					prioridad = inicio->prioridad;
					time_t tiempoInicio =inicio->tInicio;

					inicio = removerPrimero(inicio);
					time_t tiempoTrabajo;
					time(&tiempoTrabajo);
					sleep(espera);
					time_t tiempoFin;
					time(&tiempoFin);
					double tat = difftime(tiempoFin,tiempoInicio);
					double wt = difftime(tiempoFin,tiempoTrabajo);
					agregarProcesoColaFin(id,burst,prioridad,tat,wt);
					printf("Finalizo proceso: %d \n",id);

			}
			if(opcionAlgoritmo==2)
			{
				nodo* menor = buscarBurstMenor(inicio);
				if(menor!=NULL){
						printf("Ejecutando proceso %d, con burst de %d y prioridad %d\n",
						menor->id,menor->burst,menor->prioridad);

						espera = menor->burst;
						id = menor->id;
						burst = menor->burst;
						prioridad = menor->prioridad;
						time_t tiempoInicio =menor->tInicio;

						inicio = remover(inicio,menor);
						time_t tiempoTrabajo;
						time(&tiempoTrabajo);
						sleep(espera);
						time_t tiempoFin;
						time(&tiempoFin);
						double tat = difftime(tiempoFin,tiempoInicio);
						double wt = difftime(tiempoFin,tiempoTrabajo);
						agregarProcesoColaFin(id,burst,prioridad,tat,wt);
						printf("Finalizo proceso: %d\n",id);
				}
			}
			if(opcionAlgoritmo==3)
			{
					nodo* mayor = buscarPrioridadMayor(inicio);
					if(mayor!=NULL){
							printf("Ejecutando proceso %d, con burst de %d y prioridad %d\n",
							mayor->id,mayor->burst,mayor->prioridad);
							espera = mayor->burst;
							id = mayor->id;
							burst = mayor->burst;
							prioridad = mayor->prioridad;
							time_t tiempoInicio =mayor->tInicio;

							inicio = remover(inicio,mayor);
							time_t tiempoTrabajo;
							time(&tiempoTrabajo);
							sleep(espera);
							time_t tiempoFin;
							time(&tiempoFin);
							double tat = difftime(tiempoFin,tiempoInicio);
							double wt = difftime(tiempoFin,tiempoTrabajo);
							agregarProcesoColaFin(id,burst,prioridad,tat,wt);
							printf("Finalizo proceso: %d\n",id);
					}

				}
				if(opcionAlgoritmo==4)
				{
					printf("Ejecutando proceso %d, con burst de %d y prioridad %d quantum %d\n",
						inicio->id,inicio->burst,inicio->prioridad,quantum);
						espera = inicio->burst;
						id = inicio->id;
						burst = inicio->burst;
						prioridad = inicio->prioridad;
						time_t tiempoInicio =inicio->tInicio;
						double wtActual = inicio->wt;
						time_t tiempoTrabajo;
						time_t tiempoFin;
						int burstFaltante = inicio->burstFaltante;

						if(burstFaltante<= quantum){
							inicio = removerPrimero(inicio);
							time(&tiempoTrabajo);
							sleep(quantum);
							time(&tiempoFin);
							double tat = difftime(tiempoFin,tiempoInicio);
							double wt = difftime(tiempoFin,tiempoTrabajo);
							agregarProcesoColaFin(id,burst,prioridad,tat,wt+wtActual);
							printf("Finalizo proceso: %d\n",id);
						}else{
							inicio = removerPrimero(inicio);
							time(&tiempoTrabajo);
							sleep(quantum);
							time(&tiempoFin);
							double wt = difftime(tiempoFin,tiempoTrabajo);
							inicio = appendFinalReducirBurst(inicio,id,burst,burstFaltante-quantum,prioridad,
								tiempoInicio,0,wt+wtActual);
							printf("El proceso %d, pasa al final de la cola.\n",id);
						}



				}
		}

	}

}


void* JOBscheduler(void* arg)
{
	int cliente = *((int *) arg);
	free(arg);
	char buffer[1000];
	printf("JOBscheduler iniciado con exito\n");
	int salir = 0;
	while(1){
		int bytesRecibidos = recv(cliente, buffer, 1000, 0);
		if (bytesRecibidos <= 0)
		{
			perror("El cliente se desconecto");
			break;
		}
		buffer[bytesRecibidos] = '\0';
		printf("Entro: %s\n",buffer);
		procesarProceso(buffer,cliente);

	}
}



void* cargarClientes(void* vargp)
{
	int hilo =0;
	while(1){


		unsigned int tamanoDireccion;
		printf("Esperando cliente\n");
		int clienteNuevo = accept(servidor, (void*) &direccionCliente, &tamanoDireccion);
		int *arg =  malloc(sizeof(*arg));
		*arg = clienteNuevo;
		pthread_create(&hilosJOBscheduler[hilo++], NULL, JOBscheduler,arg);

	}
}
int main()
{
	int salida = 0;
	char entrada[100];

	if (cargarMenuAlgoritmos() == 0)
	{

			return 0;
	}
	printf("Entro\n");
	if (setup() == 0)
	{
		pthread_create(&hiloClientes, NULL, cargarClientes, NULL);
		pthread_create(&hiloCPUscheduler, NULL, CPUscheduler, NULL);


		while (salida == 0)
		{
			scanf("%s",entrada);
			if(strcmp(entrada,"M")==0){
				mostrarLista(inicio);
			}
			if(strcmp(entrada,"S")==0){
				salida = 1;
			}
		}

		mostrarListaFinal(finalizado);
		borrarLista(inicio);
	}
	return 0;
}
