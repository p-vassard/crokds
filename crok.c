#include <PA9.h>

// PAGfx Include
#include <stdio.h>  // nécessaire pour la gestion fichier
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include "fat.h"
#include "gfx/all_gfx.c"
#include "gfx/all_gfx.h"


//Sons
#include "crocrot.h"

//Musiques
#include "intro.h"
#include "kaupunki.h"

#define R2S2 0.707106781

typedef struct
{
	s32 posX;
	s32 posY;
	u16 dir;		//16 direction, dans le sens des aiguilles d'une montre, 0 : le haut, 4: droite, 8 : bas, 12 : gauche.
	int dureeDir;	//Nombre de mouvements entre chaque changement de direction
	int dureeBouge;	//Nombre de frames entre chaque déplacement
	int id;
} pabo;

typedef struct
{
	pabo *ennemi;
	struct listePabos *suivant;
} listePabos;

typedef struct
{
	s32 posX;
	s32 posY;
	int dir;	//0 : haut, 1 : bas, 2 : droite, 3 : gauche
	int id;
	int bouge;	//0 si bouge pas, 1 si bouge, change à chaque frame pour savoir s'il y a rafraichissement de position
	u16 angle;
	int mort;
} crok;

typedef struct
{
	s32 posX;
	s32 posY;
	int actif;
} stylet;

typedef struct
{
	s32 posX;
	s32 posY;
	int id;
	int type;
	int visible;
} nourriture;

typedef struct
{
	s32 posX;
	s32 posY;
	int id;
	int type;
} etoile;

void ajoutePabo(int numSprite, listePabos *liste, crok *joueur);
pabo* creePabo(int numSprite, crok *joueur);
void bougePabo(listePabos *premier);
void get_pointeur(stylet *pointeur, crok *joueur);
void bougeCrokStylet(stylet *pointeur, crok *joueur);
int bougeCrokPad(crok *joueur, stylet *pointeur);
void creeMiam(nourriture *miam);	//Recréé les 5 instance de nourriture
int checkCollisionMiam(crok *joueur, nourriture *miam);
int checkCollisionPabo(crok *joueur, listePabos *premier);
void supprimePabos(listePabos *courant);
void gereEtoiles();
void ecranTitre();
void jeu();
void gameOver(int score);

etoile etoiles[45];

int main(void){

	PA_Init();		//Initialisation de la librairie principale
	PA_InitVBL();	//Initialisation de la VBL
	PA_InitSound();	//Inistalisation du son
	PA_InitRand();	//Initialisation du générateur de nombre aléatoires
	fatInitDefault();
	PA_PlayMod(intro);	//Musique ^^
	while(1) {
		ecranTitre();
		jeu();
	}
	return 0;
	
return 0;
}

/*
	Boucle contenant la partie jouable du jeu
*/
void jeu() {

	PA_StopMod();
	PA_PlayMod(kaupunki);	//Musique ^^
	PA_ResetBgSys();
	PA_ResetSpriteSys();
	
	//Affichage du fond
	PA_Init16bitBg(1,3);
	PA_Load16bitBitmap(1,fondCadre_Bitmap);
	PA_InitText(0,1);
	PA_InitText(1,1);
	//PA_SetTextTileCol(1, 7); 	//0 à 9: blanc, rouge, vert, bleu foncé, violet, bleu clair, jaune, gris, gris foncé, noir
	
	//Chargement des palettes
	PA_LoadSpritePal(0, // Screen
					0, // Palette number
					(void*)pabo_Pal);	// Palette name	
	PA_LoadSpritePal(0, // Screen
					1, // Palette number
					(void*)crok_Pal);	// Palette name	
	PA_LoadSpritePal(0, // Screen
					2, // Palette number
					(void*)fraise_Pal);	// Palette name	
	PA_LoadSpritePal(0, // Screen
					3, // Palette number
					(void*)pomme_Pal);	// Palette name	
	PA_LoadSpritePal(0, // Screen
					4, // Palette number
					(void*)banane_Pal);	// Palette name	
	PA_LoadSpritePal(0, // Screen
					5, // Palette number
					(void*)raisin_Pal);	// Palette name	
	PA_LoadSpritePal(0, // Screen
					6, // Palette number
					(void*)gateau_Pal);	// Palette name
	PA_LoadSpritePal(0, // Screen
					7, // Palette number
					(void*)etoiles_Pal);// Palette name
					
	PA_LoadSpritePal(1, // Screen
					0, // Palette number
					(void*)fraise_Pal);	// Palette name	
	PA_LoadSpritePal(1, // Screen
					1, // Palette number
					(void*)pomme_Pal);	// Palette name	
	PA_LoadSpritePal(1, // Screen
					2, // Palette number
					(void*)banane_Pal);	// Palette name	
	PA_LoadSpritePal(1, // Screen
					3, // Palette number
					(void*)raisin_Pal);	// Palette name	
	PA_LoadSpritePal(1, // Screen
					4, // Palette number
					(void*)gateau_Pal);	// Palette name
	void* miamSprites[5] = {(void*)fraise_Sprite, (void*)pomme_Sprite, (void*)banane_Sprite, (void*)raisin_Sprite, (void*)gateau_Sprite};
	char* listeMiam[] = {"Fraise", "Pomme", "Banane", "Raisin", "Gateau"};
	//PA_CreateSprite(1, 0,(void*)fraise_Pal, OBJ_SIZE_16X16,1, 1, 50, 100);
		
	
	//Création du Crok		
	crok joueur;
	joueur.posX = ((255<<8)-8)/2;
	joueur.posY = ((191<<8)-8)/2;
	joueur.id = 0;
	joueur.mort = 0;
	PA_CreateSprite(0, joueur.id,(void*)crok_Sprite, OBJ_SIZE_16X16,1, 1, (joueur.posX>>8)-8, (joueur.posY>>8)-8);

	//Création des 5 miams
	nourriture miam[5];
	creeMiam(miam);
	
	//Création des étoiles
	int i;
	int numSprite = 7;	//Numéro de sprite
	
	for (i = 0 ; i < 30 ; i++) {
		etoiles[i].posX = PA_RandMinMax(0, 255)<<8;
		etoiles[i].posY = PA_RandMinMax(0, 191)<<8;
		etoiles[i].id = numSprite++;
		etoiles[i].type = (int)i/15;
		PA_CreateSprite(0, etoiles[i].id,(void*)etoiles_Sprite, OBJ_SIZE_8X8,1, 7, (etoiles[i].posX>>8), (etoiles[i].posY>>8));
		PA_SetSpriteAnim(0, etoiles[i].id, (int)i/15);
		PA_SetSpritePrio(0, etoiles[i].id, 3);
	}
	
	//Création du premier pabo
	listePabos listeEnnemis;
	listeEnnemis.ennemi = creePabo(numSprite++, &joueur);
	
	//Variables
	
	int mange = 0; //Nombre de miam mangés
	int score = 0;
	int scoreAffiche = -1;
	int niveau = 1;
	int niveauTermine = 0;
	int dernierMange = -1;
	int nombreCombo = 0;
	int scoreCombo = 0;
	int aPerdu = 0;
	
	//Stylet
	stylet pointeur;
	pointeur.posX = 0;
	pointeur.posY = 0;
	pointeur.actif = 0;
	

	PA_OutputText(1, 7, 10, "Combo actuelle :");
	PA_OutputText(1, 7, 12, "Nombre de combo : 0");
	
	while(aPerdu == 0)
	{
		if (joueur.mort ==0) {
			//Mouvments du Crok
			get_pointeur(&pointeur, &joueur);	//Récupération des données du pointeur
			
			if (bougeCrokPad(&joueur, &pointeur) == 1)		//Mouvements au pad
				pointeur.actif = 0;
			
			if (pointeur.actif == 1)			//Si pas de mouvement au pad, on tente celui au stylet
				bougeCrokStylet(&pointeur, &joueur);
			
			PA_SetSpriteXY(0, joueur.id, (joueur.posX>>8)-8, (joueur.posY>>8)-8);
			
			//Gestion des étoiles
			gereEtoiles();
			
			//Mouvements des pabos
			bougePabo(&listeEnnemis);
			
			//Collision avec nourriture
			i = 0;
			for(i = 0 ; i < 5 ; i++) 
				if (miam[i].visible == 1) {
					int aMange = checkCollisionMiam(&joueur, &miam[i]);
					if (aMange != -1) {
						if (dernierMange == miam[i].type) {
							scoreCombo += ++nombreCombo;	//En combo => +1, +2, +3 ...
							score += scoreCombo;
							
							PA_OutputText(1, 7, 12, "                            ");
							PA_OutputText(1, 7, 12, "Nombre de combo : %d", nombreCombo);
						}
						else {
							if (score != 0)
								PA_DeleteSprite(1, 0);
							PA_CreateSprite(1, 0,(void*)miamSprites[miam[i].type], OBJ_SIZE_16X16,1, miam[i].type, 20, 88);
							PA_OutputText(1, 7, 10, "                            ");
							PA_OutputText(1, 7, 10, "Combo actuelle : %s", listeMiam[miam[i].type]);
							
							PA_OutputText(1, 7, 12, "                            ");
							PA_OutputText(1, 7, 12, "Nombre de combo : 1");
							
							dernierMange = miam[i].type;
							nombreCombo = 1;
							scoreCombo = 0;
							score ++;
						}
						mange ++;
					}
				}
			
			//Tout mangé
			if (mange == 5) {
				niveau++;
				score+=5*niveauTermine++;
				ajoutePabo(numSprite++, &listeEnnemis, &joueur);
				creeMiam(miam);
				mange = 0;
			}
			
			//Reset L
			if (Pad.Newpress.L || Pad.Newpress.R) {
				ajoutePabo(numSprite++, &listeEnnemis, &joueur);
				creeMiam(miam);
				niveau++;
				mange = 0;
			}
			
			if (scoreAffiche != score)
				PA_OutputText(1, 2, 2, "Score : %d  ", ++scoreAffiche);
			PA_OutputText(1, 2, 4, "Niveau : %d  ", niveau);
			
			//Collision avec Pabo
			if (checkCollisionPabo(&joueur, &listeEnnemis) != -1) {
				aPerdu = 1;
				gameOver(score);
				supprimePabos(&listeEnnemis);
			}
		}
		PA_WaitForVBL();
	}
	PA_ResetBgSys();
	return;
}

/*
	Vide la mémoire des pabos
 */
void supprimePabos(listePabos *courant) {

	if (courant->suivant != NULL) {
		supprimePabos((listePabos*)courant->suivant);
		courant->suivant = NULL;
	}
	
	return;
}

/*
	Ecran titre, animation à faire ...
*/
void ecranTitre() {

	
	//Chargement du score
	int score = 42;
	
	//bool bfatok = fatInitDefault();
	bool bfatok = true;
	if (bfatok) {
		FILE *fichier = NULL;
		fichier = fopen("/crok.sav", "r"); // On ouvre le fichier en mode lecture
	 

		// Si l'ouverture a fonctionné
		if (fichier != NULL)
		{
		   // On prend les valeurs des HighScores
		   fscanf(fichier, "%d", &score); // On scan le fichier
		   fclose(fichier); // On ferme le fichier
		}
	}
	
	PA_InitText(0, 0);  // Initialisation du système de texte

	
	PA_OutputText(0, 4, 12, "Appuyez pour commencer !");
	PA_OutputText(0, 6, 20, "Meilleur score : %d", score);
	PA_OutputText(0, 1, 23, "2008 - Créé par Pierre Vassard");
	
	PA_Init16bitBg(1,2);
	PA_Load16bitBitmap(1,titre_Bitmap);
	
	
	//Dédicace =)
	//PA_InitText(1, 0);  // Initialisation du système de texte
	//PA_SetTextTileCol(1, 1);
	//PA_OutputText(1, 1, 22, "Edition spéciale ! Dédicace au");
	//PA_OutputText(1, 5, 23, "mec plus geek que moi =)");
	
	int attente = 0;
	do {
		//Scène d'intro
		attente++;
		PA_WaitForVBL();
	} while(!Stylus.Held || attente < 25);
	PA_WaitForVBL();	//Pour relever le stylet
	PA_OutputText(0, 10, 12, "           ");
}

/*
	Ecran Game Over, animation à faire
*/
void gameOver(int score) {

	//Musique
	PA_PlayMod(intro);	//Musique ^^
	
	//Entrée du nom
	PA_ResetBgSys();
	PA_ResetSpriteSys();
	
	
	PA_Init16bitBg(0,3);
	PA_Load16bitBitmap(0, fondTexteGameOver_Bitmap);
	PA_Init16bitBg(1,3);
	PA_Load16bitBitmap(1, fondGameOver_Bitmap);

	
	PA_InitText(1, 0);  // Initialisation du système de texte

	//HiScore sauvegardé ...
	int highScore = 0;
	FILE *fichier = NULL;
	fichier = fopen("/crok.sav", "r"); // On ouvre le fichier en mode lecture
 

	
	// Si l'ouverture a fonctionné
	
	if (fichier != NULL)
	{
	   // On prend les valeurs des HighScores
	   fscanf(fichier, "%d", &highScore); // On scan le fichier
	   fclose(fichier); // On ferme le fichier
	}
	
	PA_InitText(0,0);
	PA_InitText(1,0);
	PA_OutputText(1, 10, 8, "Score : %d", score);
	

	
	//Sauvegarde du score
	if (score > highScore) {
		FILE* fichier = NULL;
		char texteAffiche[32] = ""; // La chaine de caractère qui contiendra les variables
 
		fichier = fopen("/crok.sav", "w+"); // On ouvre le fichier en mode lecture + écriture et on crée le fichier s'il n'existe pas
 
	   rewind(fichier); // On s'assure d'être au début du fichier texte en retournant au début
	   sprintf(texteAffiche, "%d", score); // Enregistrement des HighScores dans la variable texteAffiche
	   fwrite(texteAffiche,log10(score)+1, 1, fichier); // On écrit la chaine de caractère texteAffiche dans le fichier score.sav se trouvant sur la racine du support (carte SD, etc.)

	   fclose(fichier); // On ferme le fichier
	   PA_SetTextTileCol(1, 1);
	   PA_OutputText(1, 8, 10, "Nouveau record !");
	}
	else
		PA_OutputText(1, 8, 10, "High Score : %d", highScore);
	
	//Attente d'appui sur l'écran
	while(!Stylus.Held)
		PA_WaitForVBL();
		
	return;
}

/*
	Gère les sorties d'écran des étoiles
*/
void gereEtoiles() {
	int i;
	for (i = 0 ; i < 30 ; i++) {
		if (etoiles[i].posX>>8 == -1) {
			etoiles[i].posX = 255<<8;
			etoiles[i].posY = PA_RandMinMax(0, 191)<<8;
		}
		if (etoiles[i].posX>>8 == 256) {
			etoiles[i].posX = 0<<8;
			etoiles[i].posY = PA_RandMinMax(0, 191)<<8;
		}
		if (etoiles[i].posY>>8 == -1) {
			etoiles[i].posY = 191<<8;
			etoiles[i].posX = PA_RandMinMax(0, 255)<<8;
		}
		if (etoiles[i].posY>>8 == 192) {
			etoiles[i].posY = 0<<8;
			etoiles[i].posX = PA_RandMinMax(0, 255)<<8;
		}
		
		PA_SetSpriteXY(0, etoiles[i].id, (etoiles[i].posX>>8), (etoiles[i].posY>>8));
	}


}

/*
	Bouge le Crok avec le pad
	Params :	*joueur : le Crok
	Retourne :	1 si on a appuyé, 0 sinon
*/
int bougeCrokPad(crok *joueur, stylet *pointeur) {
	int retour;
	s32 x=0;
	s32 y=0;
	if (Pad.Held.Up || Pad.Held.Right || Pad.Held.Down || Pad.Held.Left) {
		//Test des 8 directions
		
		//haut
		if (Pad.Held.Up)
			y = -1<<8;
		//Droite
		if (Pad.Held.Right)
			x = 1<<8;
		//Bas
		if (Pad.Held.Down)
			y = 1<<8;
		//gauche
		if (Pad.Held.Left)
			x = -1<<8;
			
		//Diagonales
		
		if (x != 0 && y != 0) {
			x = (x>>8) * PA_Cos(64);
			y = (y>>8) * PA_Sin(64);
		}
		
		//Animations ...
		//Haut
		if (Pad.Held.Up && !Pad.Held.Right && !Pad.Held.Left && !Pad.Held.Down
			&& joueur->dir != 0) {
			PA_StartSpriteAnim(0, joueur->id, 6, 6, 6);
			joueur->dir = 0;
		}
		//Gauche
		else if (((Pad.Held.Up && !Pad.Held.Right && Pad.Held.Left && !Pad.Held.Down)
		|| (!Pad.Held.Up && !Pad.Held.Right && Pad.Held.Left && !Pad.Held.Down))
		&& joueur->dir != 3) {
			PA_StartSpriteAnim(0, joueur->id, 4, 5, 6);
			joueur->dir = 3;
		}
		//Droite
		else if (((Pad.Held.Up && Pad.Held.Right && !Pad.Held.Left && !Pad.Held.Down)
		|| (!Pad.Held.Up && Pad.Held.Right && !Pad.Held.Left && !Pad.Held.Down))
		&& joueur->dir != 2) {
			PA_StartSpriteAnim(0, joueur->id, 2, 3, 6);
			joueur->dir = 2;
		}
		//Bas
		else if (((!Pad.Held.Up && !Pad.Held.Right && !Pad.Held.Left && Pad.Held.Down)
		|| (!Pad.Held.Up && Pad.Held.Right && !Pad.Held.Left && Pad.Held.Down)
		|| (!Pad.Held.Up && !Pad.Held.Right && Pad.Held.Left && Pad.Held.Down))
		&& joueur->dir != 1) {
			PA_StartSpriteAnim(0, joueur->id, 0, 1, 6);
			joueur->dir = 1;
		}

			
		s32 posX = joueur->posX + x;
		s32 posY = joueur->posY + y;
		
		//Etoiles
		int i;
		for (i = 0 ; i < 30 ; i++) {
			etoiles[i].posX += -x / (etoiles[i].type+1);
			etoiles[i].posY += -y / (etoiles[i].type+1);
			PA_SetSpriteXY(0, etoiles[i].id, (etoiles[i].posX>>8), (etoiles[i].posY>>8));
		}
	
		joueur->posX = posX;
		joueur->posY = posY;
		
		//Sorties d'écran
		if ((joueur->posX>>8) == 5)
			joueur->posX = 6<<8;
		if ((joueur->posX>>8) == 251)
			joueur->posX = (250)<<8;	
		if ((joueur->posY>>8) == 5)
			joueur->posY = 6<<8;
		if ((joueur->posY>>8) == 187)
			joueur->posY = 186<<8;	
		
		joueur->bouge = 1;
		retour = 1;
	}
	else {
		
		retour = 0;
	
		if (pointeur->actif == 0) {
			//Animation
			joueur->bouge = 0;
			
			PA_StopSpriteAnim(0, joueur->id);
			switch(joueur->dir) {
				case 0 : PA_SetSpriteAnim(0, joueur->id, 6);
						break;
				case 1 : PA_SetSpriteAnim(0, joueur->id, 0);
						break;
				case 2 : PA_SetSpriteAnim(0, joueur->id, 2);
						break;
				case 3 : PA_SetSpriteAnim(0, joueur->id, 4);
						break;
				default : break;
			}
			joueur->dir = -1;
		}
	}
	return retour;
}

/*
	Bouge le Crok avec les données du stylet
	Params :	*pointeur : le stylet
			*joueur : le Crok
*/
void bougeCrokStylet(stylet *pointeur, crok *joueur) {
	//Capture de l'angle entre le joueur et le stylet
	u16 angle = PA_GetAngle(joueur->posX>>8, joueur->posY>>8, pointeur->posX>>8, pointeur->posY>>8);
	//Calcul de la position X et Y à ajouter pour faire avancer le joueur
	joueur->posX += PA_Cos(angle);
	joueur->posY -= PA_Sin(angle);
	
	//Etoiles
	int i;
	for (i = 0 ; i < 30 ; i++) {
		etoiles[i].posX += -PA_Cos(angle) / (etoiles[i].type+1);
		etoiles[i].posY += PA_Sin(angle) / (etoiles[i].type+1);
		PA_SetSpriteXY(0, etoiles[i].id, (etoiles[i].posX>>8), (etoiles[i].posY>>8));
	}
	
	//Animation
	//Haut
	if ((angle > 64 && angle < 192)
	&&	(joueur->angle <=64 || joueur->angle >=192))
		PA_StartSpriteAnim(0, joueur->id, 6, 6, 6);	
	//Gauche
	if ((angle >=192 &&  angle < 320)
	&&	(joueur->angle <192 || joueur->angle >=320))
		PA_StartSpriteAnim(0, joueur->id, 4, 5, 6);	
	//Bas
	if ((angle >=320 &&  angle < 448)
	&&	(joueur->angle <320 || joueur->angle >=448))
		PA_StartSpriteAnim(0, joueur->id, 0, 1, 6);	
	//Droite
	if ((angle >=448 || angle <=64)
	&&	(joueur->angle <448 && joueur->angle >64))
		PA_StartSpriteAnim(0, joueur->id, 2, 3, 6);	
	
	joueur->angle = angle;
	
	if ((joueur->posX>>8) == 5)
		joueur->posX = 6<<8;
	if ((joueur->posX>>8) == 251)
		joueur->posX = (250)<<8;	
	if ((joueur->posY>>8) == 5)
		joueur->posY = 6<<8;
	if ((joueur->posY>>8) == 187)
		joueur->posY = 186<<8;	
	
	//Si le Crok a atteind son but ...
	
	if (joueur->posX>>8 == pointeur->posX>>8 && joueur->posY>>8 == pointeur->posY>>8) {
		pointeur->actif = 0;
		PA_StopSpriteAnim(0, joueur->id);
		//Haut
		if (angle > 64 && angle < 192)
			PA_SetSpriteAnim(0, joueur->id, 6);	
		//Gauche
		if (angle >=192 &&  angle < 320)
			PA_SetSpriteAnim(0, joueur->id, 4);	
		//Bas
		if (angle >=320 &&  angle < 448)
			PA_SetSpriteAnim(0, joueur->id, 0);	
		//Droite
		if (angle >=448 || angle <=64)
			PA_SetSpriteAnim(0, joueur->id, 2);	
		joueur->bouge = 0;
	}
	
	return;
}

/*
	Récupère la position du stylet décalé de 8 bits
	Params :	*pointeur : le stylet
			*joueur : le Crok
*/
void get_pointeur(stylet *pointeur, crok *joueur) {
	if(Stylus.Held && joueur->posX>>8 != Stylus.X && joueur->posY>>8 != Stylus.Y) {
		pointeur->posX = Stylus.X<<8;
		pointeur->posY = Stylus.Y<<8;
		pointeur->actif = 1;
	}
	return;
}


/*
	Ajoute un pabo à la liste des pabo
	Params :	numSprite : numéro de la sprite du pabo
			*liste : liste des pabos actuelle
*/
void ajoutePabo(int numSprite, listePabos *liste, crok *joueur) {

	pabo *ennemi = creePabo(numSprite, joueur);
	
	listePabos *nouveau = (listePabos*)malloc(sizeof(listePabos));
	nouveau->ennemi = ennemi;
	nouveau->suivant = NULL;
	
	listePabos *courant = liste;
	//Ajoute en fin de liste
	while (courant->suivant != NULL) {
		courant = (listePabos*)courant->suivant;
	}

	courant->suivant = (struct listePabos*)nouveau;
	return;
}

/*
	Créé un Pabo
	Params :	numSprite : numéro de la sprite du pabo
	Retourne :	un pabo paramétré aléatoirement
*/
pabo* creePabo(int numSprite, crok *joueur) {
	int posX;
	int posY;
	//La distance entre le pabo qui apparait et le joueur doit être supérieure à 50px
	do {
		posX = PA_RandMinMax(0, 255);
		posY = PA_RandMinMax(0, 191);
	} while(PA_Distance(joueur->posX>>8, joueur->posY>>8, posX, posY) < (50)*(50));
	
	int dir = PA_RandMinMax(0, 15);
	int id = numSprite;

	pabo *ennemi = malloc(sizeof(*ennemi));
	ennemi->posX = posX<<8;
	ennemi->posY = posY<<8;
	ennemi->dir = dir*32;
	ennemi->id = id;
	ennemi->dureeDir = 0;
	ennemi->dureeBouge = 0;
	PA_CreateSprite(0, id,(void*)pabo_Sprite, OBJ_SIZE_16X16,1, 0, (ennemi->posX>>8)-8, (ennemi->posY>>8)-8);
		
	//Animation
	//Haut
	if (ennemi->dir > 64 && ennemi->dir < 192)
		PA_StartSpriteAnim(0, ennemi->id, 6, 6, 6);	
	//Gauche
	if (ennemi->dir >=192 &&  ennemi->dir < 320)
		PA_StartSpriteAnim(0, ennemi->id, 4, 5, 6);	
	//Bas
	if (ennemi->dir >=320 &&  ennemi->dir < 448)
		PA_StartSpriteAnim(0, ennemi->id, 0, 1, 6);	
	//Droite
	if (ennemi->dir >=448 || ennemi->dir <=64)
		PA_StartSpriteAnim(0, ennemi->id, 2, 3, 6);	
				
	return ennemi;
}

/*
	Bouge un pabo
	params :	*ennemi : le pabo en question
*/
void bougePabo(listePabos *premier) {
	//Ajout d'une direction aléatoire
	listePabos *courant = premier;
	pabo *ennemi;
	while (courant != NULL) {
		ennemi = courant->ennemi;
		int bougeMax = 3;
		int ancienne_dir = ennemi->dir;
		if (++ennemi->dureeBouge != bougeMax) {
			if (++ennemi->dureeDir == 20) {
				ennemi->dir += (PA_RandMinMax(-1, 1)*32);
				ennemi->dir %= 512;
				ennemi->dureeDir=0;
			}
			
			//Animation
			//Haut
			if ((ennemi->dir > 64 && ennemi->dir < 192)
			&&	(ancienne_dir <=64 || ancienne_dir >=192))
				PA_StartSpriteAnim(0, ennemi->id, 6, 6, 6);	
			//Gauche
			if ((ennemi->dir >=192 &&  ennemi->dir < 320)
			&&	(ancienne_dir <192 || ancienne_dir >=320))
				PA_StartSpriteAnim(0, ennemi->id, 4, 5, 6);	
			//Bas
			if ((ennemi->dir >=320 &&  ennemi->dir < 448)
			&&	(ancienne_dir <320 || ancienne_dir >=448))
				PA_StartSpriteAnim(0, ennemi->id, 0, 1, 6);	
			//Droite
			if ((ennemi->dir >=448 || ennemi->dir <=64)
			&&	(ancienne_dir <448 && ancienne_dir >64))
				PA_StartSpriteAnim(0, ennemi->id, 2, 3, 6);	
				
			//Trigo !!
			float ajouteX = PA_Cos(ennemi->dir);
			float ajouteY = -PA_Sin(ennemi->dir);

			//Ajout des modifications
			ennemi->posX += ajouteX;
			ennemi->posY += ajouteY;
			
			//Warp écran
			if ((ennemi->posX>>8) < 0-8)
				ennemi->posX = (255<<8)+8;
			else if ((ennemi->posX>>8) > 255+8)
				ennemi->posX = 0-8;
				
			if ((ennemi->posY>>8) < 0-8)
				ennemi->posY = (191<<8)+8;
			else if ((ennemi->posY>>8) > 191+8)
				ennemi->posY = 0-8;
			
			PA_SetSpriteXY(0, ennemi->id, (ennemi->posX>>8)-8, (ennemi->posY>>8)-8);
		}
		if (ennemi->dureeBouge==bougeMax)
			ennemi->dureeBouge = 0;
			
		courant = (listePabos*)courant->suivant;
	}
}

/*
.	Génère aléatoirement les 5 instances de nourriture
*/
void creeMiam(nourriture *miam) {
	int i;
	void* miamSprites[5] = {(void*)fraise_Sprite, (void*)pomme_Sprite, (void*)banane_Sprite, (void*)raisin_Sprite, (void*)gateau_Sprite};
	
	for (i = 0 ; i < 5 ; i++) {
		miam[i].posX = PA_RandMinMax(0+8, 255-8)<<8;
		miam[i].posY = PA_RandMinMax(0+8, 191-8)<<8;
		miam[i].id = i+1;
		miam[i].type = PA_RandMinMax(0,4);
		miam[i].visible = 1;
		PA_CreateSprite(0, miam[i].id, (void*)miamSprites[miam[i].type], OBJ_SIZE_16X16,1, 2+miam[i].type, (miam[i].posX>>8)-8, (miam[i].posY>>8)-8);
		PA_SetSpritePrio(0, miam[i].id, 2);
	}
}

/*
	Gère la collision entre le joueur et les instances de nourriture
*/
int checkCollisionMiam(crok *joueur, nourriture *miam) {
	int touche = -1;
	//Distance inférieure à 10px
	if (PA_Distance(joueur->posX>>8, joueur->posY>>8, miam->posX>>8, miam->posY>>8) < (10)*(10)) {
			PA_DeleteSprite(0,miam->id);
			miam->visible = 0;
			touche = miam->id;
		}

	return touche;
}

/*
	Gère la collision entre le joueur et les ennemis
*/
int checkCollisionPabo(crok *joueur, listePabos *premier) {

	int retour = -1;
	listePabos *courant = premier;
	while (courant != NULL) {
		//Distance de 12px
		if (PA_Distance(joueur->posX>>8, joueur->posY>>8, courant->ennemi->posX>>8, courant->ennemi->posY>>8) < 12*12) {
			//S'il y a collision ...
			if (joueur->mort == 0) {
				PA_StopSpriteAnim(0, joueur->id);
				PA_SetSpriteAnim(0, joueur->id, 7);
				joueur->mort = 1;
				int sequence = 0;
				PA_StartSpriteAnim(0, courant->ennemi->id, 7, 8, 3);
				PA_PlaySimpleSound(7, crocrot);
				//Attente de la fin du son
				while(sequence++ < 100)
					PA_WaitForVBL();
			}
			retour = courant->ennemi->id;
		}
		
		courant = (listePabos*)courant->suivant;
	}
	//Retourne l'id du Pabo
	return retour;
}
