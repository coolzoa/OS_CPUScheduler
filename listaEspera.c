#include <time.h>

typedef struct nodo{
	int id;
	char estado; //C: completado, N: no completado, E: ejecutando
	int burst;
	int burstFaltante;
	int prioridad;
	double tat;
	double wt;
	time_t tInicio;
	struct nodo* siguiente;
}nodo;




//crear un ndo
nodo* crearNodo(int id, int burst, int prioridad, nodo* siguiente,time_t tiempoInicio
	,double tat, double wt)
{
	nodo* nuevo = (nodo*) malloc(sizeof(nodo));
	if (nuevo == NULL)
	{
		printf("Error al crear el nodo\n");
		exit(0);
	}
	//wt es workingTime
	//tat total de tiempo entre trabajo y espera
	nuevo->id = id;
	nuevo-> burst = burst;
	nuevo-> burstFaltante = burst;
	nuevo->prioridad = prioridad;
	nuevo->siguiente = siguiente;
	nuevo->tInicio = tiempoInicio;
	nuevo->tat = tat;
	nuevo->wt = wt;
	return nuevo;
}




//agregar nodo al fin de la lista
nodo* appendTiempo(nodo* inicio, int id, int burst, int prioridad, time_t tiempoInicio)
{
	if (inicio == NULL)
	{
		return NULL;
	}
	//nos vamos al ultimo nodo
	nodo* cursor = inicio;
	while (cursor->siguiente != NULL)
	{
		cursor = cursor->siguiente;
	}

	nodo* nuevo = crearNodo(id, burst, prioridad, NULL,tiempoInicio,0,0);
	cursor->siguiente = nuevo;
	return inicio;
}


//agregar nodo al fin de la lista
nodo* appendFinal(nodo* inicio, int id, int burst, int prioridad,time_t tiempoInicio,
	double tat, double wt)
{
	if (inicio == NULL)
	{
		return NULL;
	}
	//nos vamos al ultimo nodo
	nodo* cursor = inicio;
	while (cursor->siguiente != NULL)
	{
		cursor = cursor->siguiente;
	}

	nodo* nuevo = crearNodo(id, burst, prioridad, NULL,tiempoInicio,tat,wt);
	cursor->siguiente = nuevo;
	return inicio;
}

//agregar nodo al fin de la lista
nodo* appendFinalReducirBurst(nodo* inicio, int id, int burst,int burstFaltante, int prioridad,
	time_t tiempoInicio, double tat, double wt)
{
	if (inicio == NULL)
	{
		return NULL;
	}
	//nos vamos al ultimo nodo
	nodo* cursor = inicio;
	while (cursor->siguiente != NULL)
	{
		cursor = cursor->siguiente;
	}

	nodo* nuevo = crearNodo(id, burst, prioridad, NULL,tiempoInicio,tat,wt);
	nuevo-> burstFaltante = burstFaltante;
	cursor->siguiente = nuevo;
	return inicio;
}


void mostrarLista(nodo* inicio)
{
	nodo* cursor = inicio;
	while (cursor != NULL)
	{
		printf("ID: %d, burst: %d, prioridad %d, estado: %c\n",
		      cursor->id, cursor->burst, cursor->prioridad, cursor->estado);
		cursor = cursor->siguiente;
	}
}

//remover de inicio de lista
nodo* removerPrimero(nodo *inicio)
{
	if (inicio == NULL)
	{
		return NULL;
	}

	nodo* frente = inicio;
	inicio = inicio->siguiente;
	frente->siguiente = NULL;
	if (frente == inicio)
	{
		inicio = NULL;
	}
	free(frente);
	return inicio;
}

//remover de fin de lista
nodo* removerUltimo(nodo* inicio)
{
	if (inicio == NULL)
	{
		return NULL;
	}

	nodo* cursor = inicio;
	nodo* trasero = NULL;
	while (cursor->siguiente != NULL)
	{
		trasero = cursor;
		cursor = cursor->siguiente;
	}
	if (trasero != NULL)
	{
		trasero->siguiente = NULL;
	}

	//si este es el ultimo nodo en lista
	if (cursor == inicio)
	{
		inicio = NULL;
	}
	free(cursor);
	return inicio;
}




//remueve cualquier nodo de lista
nodo* remover(nodo* inicio, nodo* destino)
{
	if (destino == NULL)
	{
		return NULL;
	}

	if (destino == inicio)
	{
		return removerPrimero(inicio);
	}

	//si es el ultimo
	if (destino->siguiente == NULL)
	{
		return removerUltimo(inicio);
	}

	//en caso de estar en el medio
	nodo* cursor = inicio;
	while (cursor != NULL)
	{
		if (cursor-> siguiente == destino)
		{
			break;
		}
		cursor = cursor->siguiente;
	}
	if (cursor != NULL)
	{
		nodo* tmp = cursor->siguiente;
		cursor->siguiente = tmp->siguiente;
		tmp->siguiente = NULL;
		free(tmp);
	}
	return inicio;
}

//mostrar info de un nodo
void mostrarNodo(nodo* n)
{
	printf("%d \n", n->id);
}

nodo* buscarBurstMenor(nodo* inicio)
{
	int menor;
	nodo* nodoMenor = inicio;
	nodo* cursor = inicio;
	if(cursor!= NULL){
		menor = cursor->burst;
	}
	while (cursor != NULL)
	{
		if (cursor->burst < menor)
		{
			menor = cursor->burst;
			nodoMenor = cursor;
		}
		cursor = cursor->siguiente;
	}
	return nodoMenor;
}

//Se toma 1 como la prioridad mayor y 10 como la menor
nodo* buscarPrioridadMayor(nodo* inicio)
{
	int prioridadMayor;
	nodo* nodoPrioritario = inicio;
	nodo* cursor = inicio;
	if(cursor!= NULL){
		prioridadMayor = cursor->prioridad;
	}
	while (cursor != NULL)
	{
		if (cursor->prioridad < prioridadMayor)
		{
			prioridadMayor = cursor->prioridad;
			nodoPrioritario = cursor;
		}
		cursor = cursor->siguiente;
	}
	return nodoPrioritario;
}



//buscar un nodo en particular y retorna primera aparicion o null
nodo* buscar(nodo* inicio, int id)
{
	nodo* cursor = inicio;
	while (cursor != NULL)
	{
		if (cursor->id == id)
		{
			return cursor;
		}
		cursor = cursor->siguiente;
	}
	return NULL;
}


//borrar todos los nodos
void borrarLista(nodo* inicio)
{
	nodo* cursor, *temp;
	if (inicio != NULL)
	{
		cursor = inicio->siguiente;
		inicio->siguiente = NULL;
		while (cursor != NULL)
		{
			temp = cursor->siguiente;
			free(cursor);
			cursor = temp;
		}
	}
}



int contarElementos(nodo* inicio)
{
	nodo* cursor = inicio;
	int c = 0;
	while (cursor != NULL)
	{
		c++;
		cursor = cursor->siguiente;
	}
	return c;
}

double tiempoCPUocupado(nodo* inicio)
{
	nodo* cursor = inicio;
	double c = 0;
	while (cursor != NULL)
	{
		c += cursor->wt;
		cursor = cursor->siguiente;
	}
	return c;
}

double totalWaitingTime(nodo* inicio)
{
	nodo* cursor = inicio;
	double c = 0;
	while (cursor != NULL)
	{
		c += cursor->tat - cursor->wt;
		cursor = cursor->siguiente;
	}
	return c;
}

void mostrarListaFinal(nodo* inicio)
{
	int totalProcesos=contarElementos(inicio);
	double tiempoOcioso= tiempoCPUocupado(inicio);
	double waitingTimeTotal = totalWaitingTime(inicio);
	double promedioWT = waitingTimeTotal/totalProcesos;
	printf("\n\n\nTotal de procesos: %d\n",totalProcesos);
	printf("Tiempo ocioso: %.2f\n",tiempoOcioso);
	printf("Promedio WT: %.2f\n",promedioWT);
	nodo* cursor = inicio;
	while (cursor != NULL)
	{
		printf("ID: %d, burst: %d, prioridad %d, TAT: %.2f, WT: %.2f\n",
		      cursor->id, cursor->burst, cursor->prioridad, cursor->tat,cursor->tat-cursor->wt);
		cursor = cursor->siguiente;
	}
}
