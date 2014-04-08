/*
    Led_Matrix - Raspberry Pi powered scrolling LED matrix display
    Journal lumineux défilant à base de Raspberry Pi
	Copyright (C) 2014 François Mocq, F1GYT
    
    See https://github.com/framboise314/led_matrix

    Ce programme est un logiciel libre : vous pouvez le redistribuer
	ou le modifier suivant les termes de la GNU General Public License
	telle que publiée par la Free Software Foundation, soit la version 3
	de la Licence, soit (à votre gré) toute version ultérieure.

    Ce programme est distribué dans l’espoir qu’il sera utile, mais
	SANS AUCUNE GARANTIE : sans même la garantie implicite de COMMERCIALISABILITÉ
	ni d’ADÉQUATION À UN OBJECTIF PARTICULIER. Consultez la GNU General Public License
	pour plus de détails.

    Vous devriez avoir reçu une copie de la GNU General Public License avec
    ce programme ; si ce n’est pas le cas, consultez : <http://www.gnu.org/licenses/>.
	
	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
*
* Compiler avec : gcc prog_7219_v1.c -I/usr/local/include -L/usr/local/lib -lwiringPi
*
*******************************************************************************************
*
*  Le Raspberry Pi communique avec le MAX7219 en utilisant 3 signaux : DATA, CLK, and LOAD (CS).
*  Les signaux correspondants à ceux du bus SPI sur le GPIO P1 sont utilisés ici, mais sont gérés
*  par le programme lui-même. La donnée D15 est envoyée en premier.
*                   ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___
*    DATA _________|D15|D14|D13|D12|D11|D10|D09|D08|D07|D06|D05|D04|D03|D02|D01|D00|______
*         ________    __    __    __    __    __    __    __    __    __    __    ________
*    CLK          |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|
*         ______                                                                      __
*    LOAD       |____________________________________________________________________|  |__
*
*  Cette version gère 2 afficheurs matrice 8x8
*  Le signal de verrouillage n'est donc envoyé qu'après 32 bits (2 x 16) de données
*  Utilisation d'un tampon pour le message
*  ajout d'une initialisation des matrices entre chaque message
*  + affichage de l'heure
*  + mise à l heure
*  
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <time.h>
#include <string.h>
#include "font.h"

// Définition des pins du GPIO utilisées pour la commande du MAX7219
// Les numéros sont ceux de wiringPi
// 10 correspond à CSO  GPIO 24 sur P1
// 12 correspond à MOSI GPIO 19 sur P1
// 14 correspond à SCLK GPIO 23 sur P1

#define MAX7219_CS0		10
#define MAX7219_DIN		12
#define MAX7219_CLK		14

// Définition des registres du MAX7219

#define MAX7219_REG_NOOP 		0x00
#define MAX7219_REG_DECODE 		0x09
#define MAX7219_REG_INTENSITY 	0x0A
#define MAX7219_REG_SCAN_LIMIT 	0x0B
#define MAX7219_REG_SHUTDOWN 	0x0C
#define MAX7219_REG_TEST 		0x0F

// Définition de la luminosité de la matrice

#define MAX7219_INTENSITY 		0x13

/************************************/
/*    Définition des macros         */
/************************************/

// Fonction pour tester la valeur d'un bit
// utilisation : CHECK_BIT(temp, 3)
// Source : http://stackoverflow.com/questions/523724/c-c-check-if-one-bit-is-set-in-i-e-int-variable
// et http://www.bien-programmer.fr/bits.htm
//****************************************

#define CHECK_BIT(var, pos) ((var) & (1<<(pos)))

/************************************/
/*    Définition des variables      */
/************************************/
  // char message[] ;	// Contient le message en ASCII
  // char message[] = "AFPA Le Creusot * Bonjour, nous sommes le jeudi 3 avril 2014 *  ";	// Contient le message en ASCII
  //char message[] = "framboise314.fr : le Raspberry Pi a la sauce francaise ";	// Contient le message en ASCII
  unsigned char caractere;				// Utilisé pour lire le message
  unsigned char registre;				// Utilisé pour l'écriture dans le MAX7219
  unsigned char octet;					// Utilisé pour transmettre les données au MAX7219
  int MAX7219_NOMBRE;					// Nombre de matrices utilisées
  int longueur_message;	                // Longueur du message en octets - Attention au 0 final
  int pointeur_message;					// pointeur utilisé pour parcourir le message
  int longueur_buffer_message;			// Longueur totale du buffer contenant les octets à afficher
  int longueur_buffer_affichage;		// Longueur du buffer d'affichage = <nombre_de_matrices_a_LED> * 8
  int colonne;							// colonne de colonne d'affichage de la matrice
  int nombre_de_matrices_a_LED;			// Nombre de matrices dans l'afficheur
  int i;								// Utilisé pour compter dans les boucles
  int pointeur_buffer;					// Pointeur utilisé dans la gestion du buffer tournant
  int compteur;							// Utilisé dans les boucles
  char heure_affichage[40];				// Buffer contenant l'heure à afficher
  int pointeur_buffer_heure;			// Utilisé pour transférer le graphique de l'heure



/************************************/
/*    Définition des fonctions      */
/************************************/
void MAX7219_Clear(void);
int MAX7219_Setup(int nombre);
int MAX7219_Write(unsigned char reg, unsigned char data);

// Effacement des LED de la matrice
//*********************************

void MAX7219_Clear(void)
{
  unsigned char i;
  for (i=0; i<8; i++)
    MAX7219_Write(i, 0x00);
}

void MAX7219_Latch(void)
{
  // Passer CS0 à 1 et attendre 1µs
  digitalWrite (MAX7219_CS0, HIGH) ; delayMicroseconds (1);

  // Passer CS0 à 0 et attendre 1µs
  digitalWrite (MAX7219_CS0, LOW) ; delayMicroseconds (1);

}

// Initialisation du MAX7219
//**************************

int MAX7219_Setup(int nombre)
{
  unsigned char i;	
  for (i=0; i<(nombre+1); i++)
    {
	  // Pas de limitation de scan, balayer les 8 lignes
      MAX7219_Write(MAX7219_REG_SCAN_LIMIT, 7);
	}
  // Transférer les données dans les registres des MAX7219
  MAX7219_Latch();

  for (i=0; i<(nombre+1); i++)
    {
      // Utilisation d'une matrice 8x8 => mode no decode 
      MAX7219_Write(MAX7219_REG_DECODE, 0x00);
	}
  // Transférer les données dans les registres des MAX7219
  MAX7219_Latch();

  for (i=0; i<(nombre+1); i++)
    {
      // Mode d'opération normal : no shutdown
      MAX7219_Write(MAX7219_REG_SHUTDOWN, 0x01);
	}
  // Transférer les données dans les registres des MAX7219
  MAX7219_Latch();

  for (i=0; i<(nombre+1); i++)
    {
      // Mode d'opération normal : no test mode
      MAX7219_Write(MAX7219_REG_TEST, 0x00);
	}
  // Transférer les données dans les registres des MAX7219
  MAX7219_Latch();

  for (i=0; i<(nombre+1); i++)
    {
      // Réglage de la luminosité des matrices (valeur comprise entre 0x00 et 0x0F)
      MAX7219_Write(MAX7219_REG_INTENSITY, MAX7219_INTENSITY);
	}
  // Transférer les données dans les registres des MAX7219
  MAX7219_Latch();

  for (i=0; i<(nombre+1); i++)
    {
      // Effacement des LED de la matrice
      MAX7219_Clear();
	}
  // Transférer les données dans les registres des MAX7219
  MAX7219_Latch();

  }




// Ecriture dans le MAX7219
//**************************

int MAX7219_Write(unsigned char reg, unsigned char data)
{

  // Définition de la variable utilisée pour le colonne de bits
  char i=16;

  // Combiner les deux octets en un mot de 16 bits
  unsigned short mot = (reg << 8) | data;
  
  // Passer CS0 à 0 pour activer le MAX7219 et attendre 1µs
  digitalWrite (MAX7219_CS0, LOW) ; delayMicroseconds (1);


  // On peut commencer à envoyer les données
  // la variable data contient 16 bits (0 à 15) 
  // qu'il faut envoyer un par un sur DIN

  do
  {
  // Passer CLK à 0 et attendre 1µs
  digitalWrite (MAX7219_CLK, LOW) ; delayMicroseconds (1);

  // Positionner DIN à la valeur du bit concerné
    digitalWrite (MAX7219_DIN, CHECK_BIT(mot, i-1));
    
    // Attendre 1µs
    delayMicroseconds (1);

    // Passer CLK à 1 et attendre 1µs
    digitalWrite (MAX7219_CLK, HIGH) ; delayMicroseconds (1);

    // tester le bit suivant jusqu'au bit 0
    i--;
  }
  while (i!=0);

   // Attendre 1µs
  delayMicroseconds (1);

}


/***********************************************************************************
**************  Programme principal ************************************************
************************************************************************************
*/

int main(int argc, char *argv[])
{
  // Initialiser wiringPi
  if (wiringPiSetup () < 0)
  {
    fprintf (stderr, "setup failed\n") ;
    exit (1) ;
  }

do
{
 // char message[] = "AFPA Le Creusot * Bonjour, nous sommes le jeudi 3 avril 2014 *  ";	// Contient le message en ASCII
char message[] = "framboise314.fr ""\x01"" Journal lumineux d""\x82""filant " "\x85" " base de Raspberry Pi * framboise314.fr * Raspberry Pi powered scrolling LED matrix display * ";
/******************************************/
/*    Récupérer l'heure sous forme HH:MM  */
/******************************************/ 
   /* lire l'heure courante */
   time_t now = time (NULL);

   /* la convertir en heure locale */
   struct tm tm_now = *localtime (&now);

   /* Creer une chaine HH:MM*/
   char s_now[sizeof "HH:MM"];

   strftime (s_now, sizeof s_now, "%H:%M", &tm_now);

   /* afficher le resultat : */
   printf ("Il est actuellement %s \n", s_now);

   // Ajouter l'heure au message
   printf(" Le message est : %s \n",message);
   printf(" La longueur du message avant ajout de l'heure est : %d \n",sizeof(message)-1);
   strcat(message, s_now);
   strcat(message, "       ");
   printf(" Le message est : %s \n",message);
   longueur_message = sizeof(message)-1+12;
   printf(" La longueur du message est : %d \n",longueur_message);
   
   
/***************************************/
/*    Définition du buffer de message  */
/***************************************/
  // Récupération du nombre de MAX7219 utilisés
  if(argc != 2)
  {
    printf("Pour lancer le programme utilisez : sudo <nom_du_programme> <nombre_de_matrices_a_LED> \n");
    exit(1);
  }

  MAX7219_NOMBRE = atoi(argv[1]);
  if(MAX7219_NOMBRE == 0)
  {
    printf("Vous devez entrer un chiffre supérieur à 0\n");
    exit(1);
  }
   
   // Le nombre d'octets à réserver est égal à [<nombre_de_matrices_a_LED> x 8] + 
   // [<nombre_de_caractères_du_message> x 8]
  longueur_buffer_message = (MAX7219_NOMBRE * 8) + (longueur_message * 8);
  unsigned char buffer_message [longueur_buffer_message]; 
  
  printf ("Réservation de %d octets pour le buffer de message\n", longueur_buffer_message);
  
/**************************************************/
/*    Remplissage du buffer graphique de message  */
/**************************************************/
  // On commence par remplir la zone vide de <nombre_de_matrices_a_LED> * 8 octets
  for (i=0; i < (MAX7219_NOMBRE * 8); i ++)
  {
     buffer_message [i] = 0;
  }

  // Ensuite on convertit le code ASCII de chaque caractère en octets pour l'affichage
  //printf("Après init de la zone vide, le pointeur vaut : %d\n", i); 
  pointeur_message = 0;
  do
  {
     caractere = message [pointeur_message];
	 if (caractere > 128)
	 {
	 printf("Caractère %d \n",caractere);
	 }
	 // traiter les caractères accentués
	// if (caractere > 128)
	 //{
	 //printf("Caractère > 128 ! \n");
	 //}
	 
	 // Récupération des 8 octets du caractère
	 for (colonne = 1; colonne < 9; colonne ++)
	 {
	     octet = cp437_font [caractere] [ colonne - 1];
		 buffer_message [i] = octet;
		//printf("Dans le buffer : Octet %d contient %x \n",i, octet);
	     i++; // colonne suivante du buffer d'affichage
	 }
	 // Passer au caractère suivant
	 //printf("Après le caractère %d le buffer vaut %d \n", pointeur_message, i);
	 pointeur_message ++;
	 
  }
  while (pointeur_message < (longueur_message -1));
  
 
  printf("Initialisation des ports GPIO \n");
  // Initialiser les ports GPIO utilisés en sortie
  pinMode (MAX7219_CS0, OUTPUT) ;
  pinMode (MAX7219_DIN, OUTPUT) ;
  pinMode (MAX7219_CLK, OUTPUT) ;
  
  
  
  printf("Initialisation du max7219 \n");
  // Initialiser le MAX7219
  MAX7219_Setup(MAX7219_NOMBRE);
  printf("Ecriture des données : %s \n", message);
  printf("Longueur message : %d \n", sizeof(message));
  printf("Longueur réelle buffer : %d \n", sizeof(buffer_message));
  printf("Longueur utilsée : %d \n", longueur_buffer_message);
  
  // initialisation du pointeur
  pointeur_buffer = 0;
  do
   {
        // Initialiser le MAX7219
  MAX7219_Setup(MAX7219_NOMBRE);

	  // Afficher le début du message
      for (colonne =1; colonne <9; colonne ++)
       {
         // Lire un octet et l'envoyer à l'afficheur
	     compteur = 1;
	     do
	      {
	         octet = buffer_message [((compteur-1)*8) + pointeur_buffer + colonne-1];
	         //printf("Lettre 1 Octet %d %x \n", colonne, octet);
	         MAX7219_Write(colonne, octet);
	         compteur++;
	      }
	      while (compteur < MAX7219_NOMBRE+1);
	  MAX7219_Latch();
        }
	pointeur_buffer ++;
	delay(25);
	}
	while (pointeur_buffer < longueur_buffer_message -((MAX7219_NOMBRE+1) * 8) );
		    if (pointeur_buffer > longueur_buffer_message)
//		    if (pointeur_buffer + (MAX7219_NOMBRE * 8) > longueur_buffer_message)
	    {
	      pointeur_buffer = 0;
          // réinitialiser les afficheurs pour éviter des plantages (cause indéterminée)
	      MAX7219_Setup(MAX7219_NOMBRE);
	    }

	
}
while(1);
}  


