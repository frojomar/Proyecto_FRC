//============================================================================
// ----------- PRÁCTICAS DE FUNDAMENTOS DE REDES DE COMUNICACIONES -----------
// ---------------------------- CURSO 2016/17 --------------------------------
// --------------------- FRANCISCO JAVIER ROJO MARTIN ------------------------
// ----------------------- GUILLERMO SIESTO SANCHEZ --------------------------
// --------------------------- 2ºCURSO - GIIIS -------------------------------
//============================================================================

#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include "PuertoSerie.h"

using namespace std;

HANDLE PuertoCOM;

bool F5=false;

typedef char cadena[802];

struct TramaControl {
	unsigned char S; // Sincronismo = SYN =22
	unsigned char D; // Direccion=(En principio fijo a ’T’)
	unsigned char C; // Control = (05 (ENQ), 04 (EOT), 06 (ACK), 21 (NACK))
	unsigned char NT; //Numero de Trama = (En principio fijo a ‘0’)
};

struct TramaDatos {
	unsigned char S; // Sincronismo = SYN =22;
	unsigned char D; // Direccion=’T’;
	unsigned char C; // Control = STX = 02;
	unsigned char NT; //NumTrama = (En principio fijo a ‘0’);
	unsigned char L; // (Longitud del campo de datos);
	char Datos[255]; // Datos[255];
	unsigned char BCE; //(Entre 1 y 254);
};

struct variablesRecepcion {
	int control;
	int ContDatos;
	int flagFic;
	ofstream fsal;
};

struct funcionF4 {
	bool maestro;
	unsigned char NT = '0';
	int D;
	bool iniciada;
	bool fin;
	ifstream fent;
	ofstream fsal;
};

void mostrarTramaControl(TramaControl * tAux) {

	switch (tAux->C) {
	case 04:
		printf("Se ha recibido una trama EOT \n");
		break;
	case 05:
		printf("Se ha recibido una trama ENQ \n");
		break;
	case 06:
		printf("Se ha recibido una trama ACK \n");
		break;
	case 21:
		printf("Se ha recibido una trama NACK \n");
		break;
	}

}

void copiarAtramaControl(TramaDatos * tDatos, TramaControl * tControl) {
	/*delete tControl;
	 tControl= new TramaControl;*/

	tControl->S = tDatos->S;
	tControl->D = tDatos->D;
	tControl->C = tDatos->C;
	tControl->NT = tDatos->NT;
}
unsigned calcularBCE(TramaDatos * tDatos) {
	unsigned calculo = tDatos->Datos[0];
	for (int i = 1; i < tDatos->L; i++) {
		calculo = calculo ^ tDatos->Datos[i];
	}
	if (calculo == 0 || calculo == 255) {
		calculo = 1;
	}
	return calculo;
}

void mostrarMensaje(char datos[], int longitud) {
	for (int i = 0; i < longitud; i++) {
		printf("%c", datos[i]);
	}
}

void procesarTramaDatos(TramaDatos * tDatos, int flagFichero, ofstream & fsal) {
	unsigned calculo = calcularBCE(tDatos);

	if ((unsigned char)tDatos->BCE != (unsigned char) calculo) {
		printf("Ha habido un error. Mensaje corrupto. \n");
	} else {
		if (flagFichero == 0)
			mostrarMensaje(tDatos->Datos, tDatos->L);
		if (flagFichero == 1)
			fsal.write(tDatos->Datos, tDatos->L);
	}
}

void recibir(int &campo, TramaControl * tControl, TramaDatos * tDatos,variablesRecepcion * vRec, funcionF4 *f4) {
	char car;

	car = RecibirCaracter(PuertoCOM);
	if (car)
		switch (campo) {
		case 1:
			switch (car) {
			case 22:
				/*delete tDatos;
				 tDatos=new TramaDatos;*/
				tDatos->S = car;
				campo++;
				break;
			case '#':
				printf("Recibiendo fichero. \n");
				vRec->flagFic = 1;
				vRec->fsal.open("FRC-R.txt", ios::trunc); //abrimos flujo de salida
				if (!vRec->fsal.is_open())
					printf(
							"ERROR: No se ha podido abrir el fichero de escritura.\n");
				break;
			case '@':
				vRec->flagFic = 0;
				vRec->fsal.close(); //cerramos flujo de salida
				printf("Fichero recibido.\n");
				break;

			case '(': //esta eligiendo el otro compañero que quiere ser
				printf("Eligiendo tipo el otro PC..espera\n");
				car = RecibirCaracter(PuertoCOM);
				while(!car){
					car = RecibirCaracter(PuertoCOM);
				}
				switch (car) {
				case 'E':
					printf("Soy esclavo \n");
					f4->maestro = false;
					break;
				case 'M':
					printf("Soy maestro \n");
					f4->maestro = true;
					break;
				}
				f4->iniciada=true;
				break;
			}
			break;
		case 2:
			tDatos->D = car;
			campo++;
			break;
		case 3:
			tDatos->C = car;
			if (car != 02) { //si es trama de control..
				vRec->control = 1;
			}
			campo++;
			break;
		case 4:
			tDatos->NT = car;
			if (vRec->control == 1) { //es trama de control
				campo = 1;
				copiarAtramaControl(tDatos, tControl);
				mostrarTramaControl(tControl);
				vRec->control = 0;
			} else { //es trama de datos
				campo++;
			}
			break;
		case 5:
			tDatos->L = (unsigned char) car;
			campo++;
			break;
		case 6:
			tDatos->Datos[vRec->ContDatos] = car;
			if (vRec->ContDatos < tDatos->L - 1) {
				vRec->ContDatos++;
			} else {
				tDatos->Datos[vRec->ContDatos + 1] = '\0';
				campo++;
				vRec->ContDatos = 0; //para resetear para siguiente cadena.
			}
			break;
		case 7:
			tDatos->BCE = car;
			campo = 1;
			procesarTramaDatos(tDatos, vRec->flagFic, vRec->fsal);
			break;

		}
}

void seleccionTrama(TramaControl * tAux) {
	int opcion;

	tAux->S = 22; //seleccion SINCRONISMO
	tAux->D = 'T'; //seleccion DIRECCION
	tAux->NT = '0'; //seleccion NUMERO DE TRAMA

	printf("Trama de control a enviar: \n"); //seleccion CONTROL
	printf("	1: Trama ENQ. \n");
	printf("	2: Trama EOT. \n");
	printf("	3: Trama ACK. \n");
	printf("	4: Trama NACK. \n");

	opcion = -1;
	while (opcion < 1 || opcion > 4) {
		scanf("%d", &opcion);
		switch (opcion) {
		case 1:
			tAux->C = 05; //trama ENQ
			break;
		case 2:
			tAux->C = 04; //trama EOT
			break;
		case 3:
			tAux->C = 06; //trama ACK
			break;
		case 4:
			tAux->C = 21; //trama NACK
			break;
		default:
			printf("Opcion no valida. Seleccione una opcion valida \n");
		}
	}
}

void envioTramaControl() {
	TramaControl * tAux = new TramaControl;

	seleccionTrama(tAux);

	/*//Para probar que se ha configurado bien la trama
	 printf("%d", tAux->S);
	 printf("%d", tAux->D);
	 printf("%d", tAux->C);
	 printf("%d", tAux->NT);*/

	EnviarCaracter(PuertoCOM, tAux->S);
	EnviarCaracter(PuertoCOM, tAux->D);
	EnviarCaracter(PuertoCOM, tAux->C);
	EnviarCaracter(PuertoCOM, tAux->NT);

}

void configurarTramaDatos(TramaDatos * tDatos, char cad[], int iniCadena,
		int longitud) {
	int tamRestante = longitud - iniCadena; //lo que falta por mandar. Inicialmente, el tamaño entre el fin de la cadena original - lo que llevamos mandado.
	int finCadena = longitud; //inicialmente, finCadena sera el fin de la cadena Original
	int j = 0;

	tDatos->L = tamRestante; //inicialmente, L sera el tamaño Restante

	if (tamRestante > 254) { //si faltan por mandar mas de 254 caracteres.
		finCadena = 254 + iniCadena; //finCadena sera 254 caracteres por delante de iniCadena
		tDatos->L = 254; // L valdra el maximo, 254 caracteres.
	}

	tDatos->S = 22;
	tDatos->D = 'T';
	tDatos->C = 02;
	tDatos->NT = '0';

	for (int i = iniCadena; i < finCadena; i++) {
		tDatos->Datos[j] = cad[i];
		j++;
	}
	tDatos->Datos[j] = '\0';
	tDatos->BCE = calcularBCE(tDatos);
}

void enviarTramaDatos(TramaDatos * tDatos) {
	EnviarCaracter(PuertoCOM, tDatos->S);
	EnviarCaracter(PuertoCOM, tDatos->D);
	EnviarCaracter(PuertoCOM, tDatos->C);
	EnviarCaracter(PuertoCOM, tDatos->NT);
	EnviarCaracter(PuertoCOM, tDatos->L);
	EnviarCadena(PuertoCOM, tDatos->Datos, tDatos->L);
	EnviarCaracter(PuertoCOM, tDatos->BCE);
}

void enviarMensaje(char cad[], int longitud) {
	TramaDatos * tDatos;
	int numTramas = 0;

	numTramas = longitud / 254;
	if (longitud % 254 != 0) {
		numTramas++;
	}
	for (int i = 0; i < numTramas; i++) {
		tDatos = new TramaDatos();
		configurarTramaDatos(tDatos, cad, i * 254, longitud); //rellenamos la trama de Datos
		enviarTramaDatos(tDatos); //enviamos la Trama de Datos
	}

}

void envioFichero(int &campo, TramaControl * tControl,
		TramaDatos * tDatosRecepcion, variablesRecepcion * vRec,
		funcionF4 *f4) {
	//TODO hacer el envio de los ficheros.
	char cad[255];
	TramaDatos * tDatosEnvio;
	ifstream fent;
	bool fin=false; //bandera para indicar que el emisor a pulsado ESC y cortado la comunicacion

	fent.open("FRC-E.txt");

	if (!fent.is_open()) { //si no se ha podido abrir el flujo de entrada...
		printf(
				"Ha ocurrido un ERROR al abrir el fichero. Asegurese de que el fichero 'FRC-E.txt' existe en el directorio\n");
	} else { //si se ha podido abrir el flujo de entrada..
		printf("Enviando fichero.\n");
		EnviarCaracter(PuertoCOM, '#'); //caracter para indicar comienzo de envio de fichero

		//PROCESO DE ENVIO-RECEPCION SIMULTANEO.
		while (!fent.eof() && !fin) {
			//miramos antes de mandar otra trama si se ha pulsado ESC
			if (kbhit()) {
				char car = getch();
				if(car==27){
					fin=true;
				}
			}
			if (!fin){
				tDatosEnvio = new TramaDatos; //creamos una nueva trama de datos
				//leemos
				fent.read(cad, 254); //cogemos 254 caracteres. Si hay menos, se cogerán menos.
				cad[fent.gcount()] = '\0'; //ponemos el \0 final
				//configuramos la trama de datos
				configurarTramaDatos(tDatosEnvio, cad, 0, fent.gcount());
				//enviamos la trama de Datos solo si se ha leido algo
				if(fent.gcount()>0){
					enviarTramaDatos(tDatosEnvio);
				}
				//recibimos informacion si nos han enviado algo para no parar la recepcion
				recibir(campo, tControl, tDatosRecepcion, vRec, f4);
				delete tDatosEnvio; //eliminamos la referencia a esa trama de datos que habiamos creado
			}
			else{
				printf("El emisor (este PC) ha cortado la comunicacion con ESC. \n");
			}
		}

		EnviarCaracter(PuertoCOM, '@'); //caracter para indicar fin de envio de fichero
		printf("Fichero enviado.\n");

	}
}

void configurarMaestroEsclavo(funcionF4 * f4) {
	int opcion;

	//caracter que indica que estamos comenzando a elegir nosotros
	EnviarCaracter(PuertoCOM, '(');

	printf("Que quieres ser: \n");
	printf("	1. Maestro \n");
	printf("	2. Esclavo \n");

	opcion = -1;
	while (opcion < 1 || opcion > 2) {
		scanf("%d", &opcion);
		switch (opcion) {

		case 1: //SOMOS MAESTRO
			f4->maestro = true;
			EnviarCaracter(PuertoCOM, 'E'); //le decimos que va a ser esclavo
			break;

		case 2: //SOMOS ESCLAVO
			f4->maestro = false;
			EnviarCaracter(PuertoCOM, 'M'); //le decimos que va a ser maestro
			break;
		}
	}
	f4->iniciada=true;
}


char enviar(cadena &cad, int & i, int &campo, TramaControl * tControl,
		TramaDatos * tDatos, variablesRecepcion * vRec, funcionF4 *f4) {

	bool funcion=false;

	char car = getch();

	if (car != 27) {
		switch (car) {
		case '\0': //FUNCION
			car = getch();
			switch (car) {
			case (59): //F1
				cad[i] = '\n';
				printf("%c", '\n');
				i++;
				cad[i] = '\0';
				enviarMensaje(cad, i); //enviamos la cadena en bloques de datos.
				i = 0; //i=-1 y no i=0 porque a continuacion se incrementa abajo.
				break;
			case (60): //F2
				envioTramaControl();
				break;
			case (61): //F3
				envioFichero(campo, tControl, tDatos, vRec, f4);
				break;
			case (62): //F4
				configurarMaestroEsclavo(f4);
			}
			funcion=true;
			break;
		case 13: //INTRO
			cad[i] = '\n';
			printf("%c", '\n');
			break;
		case 8: //RETURN
			if (i != 0) {
				printf("%c", '\b');
				printf("%c", ' ');
				printf("%c", '\b');
				i--;
			}
			break;
		default: //CARACTER
			if (i < 800) { //si no estamos al final de la cadena dejamos escribir caracteres.
				printf("%c", car);
				cad[i] = car;
			}
		}
		if (i < 800 && car != 8 && !funcion) { //si no estamos al final de la cadena o el caracter no
			i++;	//era un retroceso, incrementamos el indice de la cadena.
		}
	}
	return car;

	//retroceso= 8 = '\b' si lo imprimes retrasa el cursor
	//cuando es retroceso ->imp ret, espacio, imp ret

	//intro= 13 => imprimir un '\n' y ponerlo en la cadena

	//f1=> poner '\0'   f1='\0' primero y 59 despues (hacer dos car=getch())
}

void seleccionPuerto(char * puerto) {
	int opcion;

	printf("Selecciona el puerto a utilizar: \n");
	printf("	1. COM1 \n");
	printf("	2. COM2 \n");
	printf("	3. COM3 \n");
	printf("	4. COM4 \n");

	opcion = -1;
	while (opcion < 1 || opcion > 4) {
		scanf("%d", &opcion);
		switch (opcion) {
		case 1:
			puerto[3] = '1';
			break;
		case 2:
			puerto[3] = '2';
			break;
		case 3:
			puerto[3] = '3';
			break;
		case 4:
			puerto[3] = '4';
			break;
		}
	}
}

void seleccionarVelocidad(int &velocidad) {
	int opcion;

	printf("Bits por segundo: \n");
	printf("	1. 1400 \n");
	printf("	2. 4800 \n");
	printf("	3. 9600 \n");
	printf("	4. 19200 \n");

	opcion = -1;
	while (opcion < 1 || opcion > 4) {
		scanf("%d", &opcion);
		switch (opcion) {
		case 1:
			velocidad = 1400;
			break;
		case 2:
			velocidad = 4800;
			break;
		case 3:
			velocidad = 9600;
			break;
		case 4:
			velocidad = 19200;
			break;
		}
	}
}

void seleccionarDatos(char &datos) {
	int opcion;

	printf("Bits de datos: \n");
	printf("	1. 5 \n");
	printf("	2. 6 \n");
	printf("	3. 7 \n");
	printf("	4. 8 \n");

	opcion = -1;
	while (opcion < 1 || opcion > 4) {
		scanf("%d", &opcion);
		switch (opcion) {
		case 1:
			datos = 5;
			break;
		case 2:
			datos = 6;
			break;
		case 3:
			datos = 7;
			break;
		case 4:
			datos = 8;
			break;
		}
	}
}

void seleccionarParidad(char &paridad) {
	int opcion;

	printf("Paridad: \n");
	printf("	1. Sin paridad \n");
	printf("	2. Impar \n");
	printf("	3. Par \n");
	printf("	4. Marca \n");
	printf("	5. Espacio \n");

	opcion = -1;
	while (opcion < 1 || opcion > 5) {
		scanf("%d", &opcion);
		switch (opcion) {
		case 1:
			paridad = 0;
			break;
		case 2:
			paridad = 1;
			break;
		case 3:
			paridad = 2;
			break;
		case 4:
			paridad = 3;
			break;
		case 5:
			paridad = 4;
			break;
		}
	}
}

void seleccionarStop(char &stop) {
	int opcion;

	printf("Bits de parada: \n");
	printf("	1. 1 \n");
	printf("	2. 1,5 \n");
	printf("	3. 2 \n");

	opcion = -1;
	while (opcion < 1 || opcion > 3) {
		scanf("%d", &opcion);
		switch (opcion) {
		case 1:
			stop = 0;
			break;
		case 2:
			stop = 1;
			break;
		case 3:
			stop = 2;
			break;
		}
	}
}

void abrirPersonalizado(char *p) {
	int velocidad = 9600;
	char datos = 8;
	char paridad = 0;
	char stop = 1;

	seleccionarVelocidad(velocidad);
	seleccionarDatos(datos);
	seleccionarParidad(paridad);
	seleccionarStop(stop);

	PuertoCOM = AbrirPuerto(p, velocidad, datos, paridad, stop); //en sala siempre COM1

}

/*
void algoritmoF4(funcionF4 * f4){
	int tipoTrama;
	char direccion;

	tipoTrama=recibirF4(f4,direccion);
	while(tipoTrama!=1){ //mientras no recibida trama ENQ, esperamos;
		tipoTrama=recibirF4(f4,direccion);
	}
	//conexion establecida
	f4->D=direccion;

	//funcion de recibir o enviar hasta que llegue el final
	while(tipoTrama!=2){ //Espera de recibir una trama EOT
		if((f4->maestro && f4->D='R') || (!f4->maestro && f4->D='T')){
			enviarF4(f4);

		}
		if((f4->maestro && f4->D='T') || (!f4->maestro && f4->D='R')){
			recibirF4(f4);
			enviarConfirmacionF4(f4);
		}
	}
}*/

void configurarTramaDatosF4(TramaDatos * tDatos, char cad[], int iniCadena,
		int longitud, funcionF4 * f4) {
	int tamRestante = longitud - iniCadena; //lo que falta por mandar. Inicialmente, el tamaño entre el fin de la cadena original - lo que llevamos mandado.
	int finCadena = longitud; //inicialmente, finCadena sera el fin de la cadena Original
	int j = 0;

	tDatos->L = tamRestante; //inicialmente, L sera el tamaño Restante

	if (tamRestante > 254) { //si faltan por mandar mas de 254 caracteres.
		finCadena = 254 + iniCadena; //finCadena sera 254 caracteres por delante de iniCadena
		tDatos->L = 254; // L valdra el maximo, 254 caracteres.
	}

	tDatos->S = 22;
	tDatos->D = f4->D;
	tDatos->C = 02;
	tDatos->NT = f4->NT;

	if(f4->NT=='0'){
		f4->NT='1';
	}
	else{
		f4->NT='0';
	}

	for (int i = iniCadena; i < finCadena; i++) {
		tDatos->Datos[j] = cad[i];
		j++;
	}
	tDatos->Datos[j] = '\0';
	tDatos->BCE = calcularBCE(tDatos);
}


int procesarTramaDatosF4(TramaDatos * tDatos, int flagFichero, ofstream & fsal, bool & error) {
	unsigned calculo = calcularBCE(tDatos);

	if ((unsigned char)tDatos->BCE != (unsigned char)calculo) {
		error=true;
	} else {
		if (flagFichero == 0)
			mostrarMensaje(tDatos->Datos, tDatos->L);
		if (flagFichero == 1)
			fsal.write(tDatos->Datos, tDatos->L);
	}
	return calculo;
}

void mostrarTramaControlF4(bool recibido,TramaControl * tControl){

	if(!recibido){//la hemos enviado
		switch(tControl->C){
			case 05: //envio de trama ENQ
				cout<<"E "<<tControl->D<<" ENQ "<<tControl->NT<<endl;
				break;
			case 02: //envio de trama STX, aunque nunca se va a dar
				cout<<"E "<<tControl->D<<" STX "<<tControl->NT<<endl;
				break;
			case 21: //envio de trama NACK
				cout<<"E "<<tControl->D<<" NACK "<<tControl->NT<<endl;
				break;
			case 06: //envio de trama ACK
				cout<<"E "<<tControl->D<<" ACK "<<tControl->NT<<endl;
				break;
			case 04: //envio de trama EOT
				cout<<"E "<<tControl->D<<" EOT "<<tControl->NT<<endl;
				break;
		}
	}
	else{//la hemos recibido
		switch(tControl->C){
			case 05: //envio de trama ENQ
				cout<<"R "<<tControl->D<<" ENQ "<<tControl->NT<<endl;
				break;
			case 02: //envio de trama STX, aunque nunca se va a dar
				cout<<"R "<<tControl->D<<" STX "<<tControl->NT<<endl;
				break;
			case 21: //envio de trama NACK
				cout<<"R "<<tControl->D<<" NACK "<<tControl->NT<<endl;
				break;
			case 06: //envio de trama ACK
				cout<<"R "<<tControl->D<<" ACK "<<tControl->NT<<endl;
				break;
			case 04: //envio de trama EOT
				cout<<"R "<<tControl->D<<" EOT "<<tControl->NT<<endl;
				break;
		}
	}
}

void mostrarTramaControl2F4(bool recibido,TramaDatos * tControl){

	if(!recibido){//la hemos enviado
		switch(tControl->C){
			case 05: //envio de trama ENQ
				cout<<"E "<<tControl->D<<" ENQ "<<tControl->NT<<endl;
				break;
			case 02: //envio de trama STX, aunque nunca se va a dar
				cout<<"E "<<tControl->D<<" STX "<<tControl->NT<<endl;
				break;
			case 21: //envio de trama NACK
				cout<<"E "<<tControl->D<<" NACK "<<tControl->NT<<endl;
				break;
			case 06: //envio de trama ACK
				cout<<"E "<<tControl->D<<" ACK "<<tControl->NT<<endl;
				break;
			case 04: //envio de trama EOT
				cout<<"E "<<tControl->D<<" EOT "<<tControl->NT<<endl;
				break;
		}
	}
	else{//la hemos recibido
		switch(tControl->C){
			case 05: //envio de trama ENQ
				cout<<"R "<<tControl->D<<" ENQ "<<tControl->NT<<endl;
				break;
			case 02: //envio de trama STX, aunque nunca se va a dar
				cout<<"R "<<tControl->D<<" STX "<<tControl->NT<<endl;
				break;
			case 21: //envio de trama NACK
				cout<<"R "<<tControl->D<<" NACK "<<tControl->NT<<endl;
				break;
			case 06: //envio de trama ACK
				cout<<"R "<<tControl->D<<" ACK "<<tControl->NT<<endl;
				break;
			case 04: //envio de trama EOT
				cout<<"R "<<tControl->D<<" EOT "<<tControl->NT<<endl;
				break;
		}
	}
}

void mostrarTramaDatosF4(bool recibido, TramaDatos * tDatos, unsigned BCEcalculado){

	if(!recibido){//la hemos enviado
		cout<<"E "<<tDatos->D<<" STX "<<tDatos->NT<<" "<<(int) tDatos->BCE<<endl;
	}
	else{//la hemos recibido
		cout<<"R "<<tDatos->D<<" STX "<<tDatos->NT<<" "<<(int) tDatos->BCE<<" "<< (int)(unsigned char) BCEcalculado<<endl;
	}
}

void enviarTramaF4(funcionF4 * f4, int tipo, TramaDatos * &tDatos){
	tDatos = new TramaDatos;
	TramaControl * tControl= new TramaControl;
	char cad[255];
	bool emisor= (f4->maestro && f4->D=='R') || (!f4->maestro && f4->D=='T'); //es emisor el maestro en seleccion y el esclavo en sondeo.
	if(emisor){ //si somos emisores (se cumple uno de los dos casos anteriores)...
		if (kbhit() ) { //comprobamos si se ha pulsado alguna tecla
			char car = getch();
			if(car=='\0'){//FUNCION
				car = getch();
				if(car==63){ //F5 (para introducir error en esta trama).
					if(F5==false){
						F5=true;
						printf("Se ha pulsado F5... introduciendo ERROR \n");
					}
				}
			}
			if(car==27){ //ESC para cortar el envio del fichero.
				f4->fin=true;
			}
		}
	}
	switch(tipo){
		case 0: //envio de trama ENQ
			tControl->S=22;
			tControl->D=f4->D;
			tControl->C=05;
			tControl->NT='0';
			mostrarTramaControlF4(false, tControl);
			EnviarCaracter(PuertoCOM, tControl->S);
			EnviarCaracter(PuertoCOM, tControl->D);
			EnviarCaracter(PuertoCOM, tControl->C);
			EnviarCaracter(PuertoCOM, tControl->NT);
			break;
		case 1: //envio de trama STX
			if(!f4->fent.eof()){
				f4->fent.read(cad, 254); //cogemos 254 caracteres. Si hay menos, se cogerán menos.
				if(f4->fent.gcount()!=0){
					cad[f4->fent.gcount()] = '\0'; //ponemos el \0 final
					configurarTramaDatosF4(tDatos, cad, 0, f4->fent.gcount(), f4);
					mostrarTramaDatosF4(false, tDatos,0);
					if(F5==true){
						char cError=tDatos->Datos[0];
						tDatos->Datos[0]='Ç';
						enviarTramaDatos(tDatos);
						tDatos->Datos[0]=cError;
						F5=false;
					}
					else{
						enviarTramaDatos(tDatos);
					}
				}
				else{
					f4->fin=true;
				}
			}
			else{
				f4->fin=true;
			}
			break;
		case 2: //envio de trama NACK
			tControl->S=22;
			tControl->D=f4->D;
			tControl->C=21;
			tControl->NT=f4->NT;
			mostrarTramaControlF4(false, tControl);
			EnviarCaracter(PuertoCOM, tControl->S);
			EnviarCaracter(PuertoCOM, tControl->D);
			EnviarCaracter(PuertoCOM, tControl->C);
			EnviarCaracter(PuertoCOM, tControl->NT);
			break;

		case 3: //envio de trama ACK
			tControl->S=22;
			tControl->D=f4->D;
			tControl->C=06;
			tControl->NT=f4->NT;
			mostrarTramaControlF4(false, tControl);
			EnviarCaracter(PuertoCOM, tControl->S);
			EnviarCaracter(PuertoCOM, tControl->D);
			EnviarCaracter(PuertoCOM, tControl->C);
			EnviarCaracter(PuertoCOM, tControl->NT);
			break;

		case 4: //envio de trama EOT
			tControl->S=22;
			tControl->D=f4->D;
			tControl->C=04;
			tControl->NT=f4->NT;
			mostrarTramaControlF4(false, tControl);
			EnviarCaracter(PuertoCOM, tControl->S);
			EnviarCaracter(PuertoCOM, tControl->D);
			EnviarCaracter(PuertoCOM, tControl->C);
			EnviarCaracter(PuertoCOM, tControl->NT);
			break;
	}

}


int recibirTramaF4(funcionF4 * f4, bool & error){
	char car;
	bool completa=false;
	TramaDatos * tDatos= new TramaDatos;
	TramaControl * tControl;
	int campo=1;
	int ContDatos=0;
	bool control=false;
	unsigned BCEcalculado=0;

	error=false;

	while(!completa){
		car = RecibirCaracter(PuertoCOM);
		if (car){
			switch (campo) {
				case 1:
					tDatos->S = car;
					campo++;
					break;
				case 2:
					tDatos->D = car;
					campo++;
					break;
				case 3:
					tDatos->C = car;
					if (car != 02) { //si es trama de control..
						if(car == 05){ //si es de tipo ENQ...
							f4->D=tDatos->D; //cogemos el tipo de conexion que vamos a llevar a cabo
						}
						control=true;
					}
					campo++;
					break;
				case 4:
					tDatos->NT = car;

					if((f4->maestro && f4->D=='T') || (!f4->maestro && f4->D=='R') ){
						f4->NT=car;
					}
					if (control==true) { //es trama de control
						//copiarAtramaControl(tDatos, tControl);
						//mostrarTramaControlF4(true,tControl);
						mostrarTramaControl2F4(true,tDatos);
						completa= true;
					} else { //es trama de datos
						campo++;
					}
					break;
				case 5:
					tDatos->L = (unsigned char) car;
					campo++;
					break;
				case 6:
					tDatos->Datos[ContDatos] = car;
					if (ContDatos < tDatos->L - 1) {
						ContDatos++;
					} else {
						tDatos->Datos[ContDatos + 1] = '\0';
						campo++;
						ContDatos = 0; //para resetear para siguiente cadena.
					}
					break;
				case 7:
					tDatos->BCE = car;
					BCEcalculado=procesarTramaDatosF4(tDatos, 1, f4->fsal, error);
					mostrarTramaDatosF4(true, tDatos, BCEcalculado);
					completa=true;
					break;
			}
		}
	}

	switch(tDatos->C){
	case 05: //envio de trama ENQ
		return 0;
		break;
	case 02: //envio de trama STX
		return 1;
		break;
	case 21: //envio de trama NACK
		return 2;
		break;

	case 06: //envio de trama ACK
		return 3;
		break;
	case 04: //envio de trama EOT
		return 4;
		break;
	}
	return -1;
}

void retransmision(TramaDatos * tDatos){
	mostrarTramaDatosF4(false, tDatos,0);
	enviarTramaDatos(tDatos);
}

void maestro(funcionF4 * f4){
	TramaDatos * tDatos;
	int opcion;
	int estado=-1;
	bool error;
	bool primeraLectura=true;

	f4->fin=false;
	F5=false;

	///////////SELECCION DEL TIPO////////////////
	printf("Tipo comunicacion: \n");
	printf("	1. Seleccion \n");
	printf("	2. Sondeo \n");
	opcion = -1;
	while (opcion < 1 || opcion > 2) {
		scanf("%d", &opcion);
		switch (opcion) {
		case 1:
			f4->D = 'R';
			break;
		case 2:
			f4->D = 'T';
			break;
		}
	}

	//////////ESTABLECIMIENTO///////////
	enviarTramaF4(f4,0,tDatos); //tipo 0 = trama de Establecimiento (ENQ)
	estado=recibirTramaF4(f4,error); //tipo 3 = trama de confirmacion (ACK)
	while(estado!=3){ //esperando trama ACK
		estado= recibirTramaF4(f4,error); //si no hemos recibido aun trama ACK esperamos
	}

	////////TRANSMISION DE DATOS EN SELECCION/////////////
	if(f4->D=='R'){ //si es de seleccion..
		f4->fent.open("FRC-E.txt");
		f4->NT='0';
		enviarTramaF4(f4, 1,tDatos); //tipo 1= trama de Datos (STX), por si fichero vacio
		while(!f4->fin){
			if(!primeraLectura){ //la primera lectura se efectua antes del while, no aquí.
				enviarTramaF4(f4, 1,tDatos); //tipo 1= trama de Datos (STX)
			}
			estado= recibirTramaF4(f4,error); //tipo 3 = trama de confirmacion (ACK)
			while(estado!=3){ //esperando trama ACK
				if(estado==2){ //tipo 2 = trama NACK
					retransmision(tDatos); //enviamos la trama erronea de nuevo
				}
				estado= recibirTramaF4(f4,error); //si no hemos recibido aun trama ACK esperamos
			}
			if(f4->fent.eof()){
				f4->fin=true;
			}
			primeraLectura=false; //sabemos que ya se ha hecho por lo menos una lectura
		}
		///////////CIERRE DE CONEXION EN SELECCION/////////////
		f4->NT='0';
		enviarTramaF4(f4, 4,tDatos); //enviamos una trama EOT
		estado=recibirTramaF4(f4,error); //tipo 3 = trama de confirmacion (ACK)
		while(estado!=3){ //esperando trama ACK
			estado= recibirTramaF4(f4,error); //si no hemos recibido aun trama ACK esperamos
		}
		f4->fent.close();
	}
	else{
		//////////TRANSMISION DE DATOS EN SONDEO///////////////////
		if(f4->D=='T'){ //si es sondeo..
			f4->fsal.open("FRC-R.txt");
			while(!f4->fin){
				estado= recibirTramaF4(f4,error); //tipo 1 = trama de datos (STX)
				while(estado!=1 && estado!=4){ //esperando trama STX o EOT
					estado= recibirTramaF4(f4,error); //si no hemos recibido aun trama STX o EOT esperamos
				}
				if(estado!=4){ //tipo 4= trama de finalizacion (EOT)
					if(!error)//no error en STX
						enviarTramaF4(f4,3,tDatos);  //enviamos trama tipo 3 = trama de confirmacion (ACK)
					else{
						enviarTramaF4(f4,2,tDatos);  //enviamos trama tipo 2 = trama NACK
					}
				}
				else{
					/////////////CIERRE DE CONEXION EN SONDEO//////////////
					printf("Quieres finalizar la conexion: \n");
					printf("	1. Acepto \n");
					printf("	2. No acepto \n");
					opcion = -1;
					while (opcion < 1 || opcion > 2) {
						scanf("%d", &opcion);
						switch (opcion) {
						case 1:
							enviarTramaF4(f4,3,tDatos); // confirmamos con trama ACK
							f4->fin=true;
							break;
						case 2:
							enviarTramaF4(f4,2,tDatos); //denegamos con trama NACK
							break;
						}
					}
				}
			}
			f4->fsal.close();
		}
	}
	f4->iniciada=false;
}

void esclavo(funcionF4 * f4){
	int estado=-1;
	TramaDatos * tDatos;
	bool error=false;
	bool primeraLectura=true;

	//////////ESPERA DE ESTABLECIMIENTO DE CONEXION////////////
	while(estado!=0){ //tipo 0= trama ENQ
		estado=recibirTramaF4(f4,error); //f4 configurado con el tipo de conexion tras recibir trama ENQ
	}
	enviarTramaF4(f4,3,tDatos); //confirmacion con trama ACK

	////////TRANSMISION DE DATOS EN SONDEO/////////////
	if(f4->D=='T'){ //si es de sondeo..
		f4->fent.open("FRC-E.txt");
		f4->NT='0';
		enviarTramaF4(f4, 1,tDatos); //tipo 1= trama de Datos (STX)
		while(!f4->fin){
			if(!primeraLectura){ //la primera lectura se efectua antes del while, no aquí.
				enviarTramaF4(f4, 1,tDatos); //tipo 1= trama de Datos (STX)
			}
			estado= recibirTramaF4(f4,error); //tipo 3 = trama de confirmacion (ACK)
			while(estado!=3){ //esperando trama ACK
				if(estado==2){ //tipo 2 = trama NACK
					retransmision(tDatos); //enviamos la trama erronea de nuevo
				}
				estado= recibirTramaF4(f4,error); //si no hemos recibido aun trama ACK esperamos
			}
			if(f4->fent.eof()){
				f4->fin=true;
			}
			primeraLectura=false; //sabemos que ya se ha hecho por lo menos una lectura
		}
		///////////CIERRE DE CONEXION EN SONDEO/////////////
		f4->NT='0';
		estado=-1;
		while(estado!=3){ //mientras no recibamos trama ACK de confirmacion
			enviarTramaF4(f4, 4,tDatos); //enviamos una trama EOT
			if(f4->NT=='0'){ //cambiamos valor de  NT por si hay que volver a solicitar cierre
				f4->NT='1';
			}
			else{
				f4->NT='0';
			}
			estado=recibirTramaF4(f4,error); //tipo 3 = trama de confirmacion (ACK)
			while(estado!=3 && estado!=2){ //esperando trama ACK o NACK //TOCADO//
				estado= recibirTramaF4(f4,error); //si no hemos recibido aun trama ACK o NACK esperamos
			}
		}
		f4->fent.close();
	}
	else{
		//////////TRANSMISION DE DATOS EN SELECCION///////////////////
		if(f4->D=='R'){ //si es seleccion..
			f4->fsal.open("FRC-R.txt");
			while(!f4->fin){
				estado= recibirTramaF4(f4,error); //tipo 1 = trama de datos (STX) o tipo 4= trama de cierre EOT
				while(estado!=1 && estado!=4){ //esperando trama STX o EOT
					estado= recibirTramaF4(f4,error); //si no hemos recibido aun trama STX o EOT esperamos
				}
				if(estado!=4){ //tipo 4= trama de finalizacion (EOT)
					if(!error)//no error en STX
						enviarTramaF4(f4,3,tDatos);  //enviamos trama tipo 3 = trama de confirmacion (ACK)
					else{
						enviarTramaF4(f4,2,tDatos);  //enviamos trama tipo 2 = trama NACK
					}				}
				else{ //si es trama de finalizacion (EOT)
					/////////////CIERRE DE CONEXION EN SELECCION//////////////
					enviarTramaF4(f4, 3,tDatos);
					f4->fin=true;
				}
			}
			f4->fsal.close();
		}
	}

	f4->iniciada=false;
}


int main() {
	char car = 0;
	cadena cad;
	int i = 0;
	int opcion;
	char puerto[5] = { 'C', 'O', 'M', '1', 0 };
	char * p;
	int campo = 1;
	TramaControl * tControl = new TramaControl;
	TramaDatos * tDatos = new TramaDatos;

	variablesRecepcion * vRec = new variablesRecepcion;

	vRec->ContDatos = 0;
	vRec->control = 0;
	vRec->flagFic = 0;
	vRec->fsal;

	funcionF4 * f4 = new funcionF4;
	f4->iniciada=false;
	f4->NT='0';
	f4->fin=false;

// Parámetros necesarios al llamar a AbrirPuerto:
// - Nombre del puerto a abrir ("COM1", "COM2", "COM3", ...).
// - Velocidad (1200, 1400, 4800, 9600, 19200, 38400, 57600, 115200).
// - Número de bits en cada byte enviado o recibido (4, 5, 6, 7, 8).
// - Paridad (0=sin paridad, 1=impar, 2=par, 3=marca, 4=espacio).
// - Bits de stop (0=1 bit, 1=1.5 bits, 2=2 bits)

	//Eleccion del puerto

	p = &puerto[0];
	seleccionPuerto(p);

	printf("Seleccionar los parámetros de configuración: \n");
	printf(
			"	1.Configuración por defecto (9600, 8, sin paridad, 1 bit parada) \n");
	printf("	2.Configuración personalizada: \n");

	scanf("%d", &opcion);

	if (opcion == 1) {
		p = &puerto[0];
		PuertoCOM = AbrirPuerto(p, 9600, 8, 0, 0); //en sala siempre COM1
	}

	else {
		p = &puerto[0];
		abrirPersonalizado(p);
	}

	if (PuertoCOM == NULL) {
		printf("Error al abrir el puerto\n");
		getch();
		return (1);
	} else
		printf("Puerto abierto correctamente\n");

// Lectura y escritura simultánea de caracteres:
	while (car != 27) {
		recibir(campo, tControl, tDatos, vRec, f4);

		if (kbhit()) {
			car = enviar(cad, i, campo, tControl, tDatos, vRec, f4);
		}

		if(f4->iniciada){
			if(f4->maestro)
				maestro(f4);
			else
				esclavo(f4);

			delete f4;
			f4 = new funcionF4;
			f4->iniciada=false;
			f4->NT='0';
			f4->fin=false;

		}
	}

	// Para cerrar el puerto:
	CerrarPuerto(PuertoCOM);

	return 0;
}
