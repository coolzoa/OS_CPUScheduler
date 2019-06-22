#include <ctype.h>

char limpiarPantalla[] = "\e[1;1H\e[2J";

//funcion que determina si hilera es solo numerica
int esNumerico(char s[])
{
	int i;
	int largo = strlen(s);

	for (i = 0; i < largo; i++)
	{
		if (!isdigit(s[i]))
		{
			return 0;
		}
	}
	return 1;
}

//funcion que convierte numero en ascii
char* miItoa(int num, char *s)
{
	if (s == NULL)
	{
		return NULL;
	}
	sprintf(s, "%d", num);
	return s;
}
