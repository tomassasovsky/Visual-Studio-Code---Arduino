int Variable_01 = 5;	//Dim Variable_01
int Variable_02 = 7;	//Dim Variable_02 
int Resultado;			//Dim Resultado

void setup(){
 Serial.begin(9600); 
}
void loop (){
    //Leer Variable_01, Variable_02;
    if (Variable_01 > 0){ //Si la Variable 1 es mayor a 0
        if (Variable_01 > Variable_02 && Variable_02 != 0 && (Variable_01 != 5 || Variable_02 != 1000)){ //Si la Variable 1 es mayor que la 2 y la variable 2 no es 0
            Resultado = Variable_01/Variable_02;
        }
        else if (Variable_01 == 5 || Variable_02 == 1000){ //Si la Variable 1 es equivalente a 5 o la Variable 2 equivalente a 1000
            Resultado = (Variable_01^Variable_02)^(1/3);
        }
        Serial.println(Resultado); //Imprimir resultado
    }
}

/*Pseudocódigo:

Procedimiento Ejemplo()
Dim Variable_01 quesea Entero
Dim Variable_01 quesea Entero
Dim Resultado quesea Entero
Leer Variable_01
	 Variable_02
Si Variable_01 > Variable_02 Y Variable_02 <> 0 Y (Variable_01 <> 5 O Variable_02 <> 1000) Entonces
Resultado = Variable_01/Variable_02
Siotro
Resultado = (Variable_01^Variable_02)^(1/3)
Fin Si
Imprimir Resultado
Fin Procedimiento
*/