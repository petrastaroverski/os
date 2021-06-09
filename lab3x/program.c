/*Promijeniti program druge vježbe tako da se dretve sinkroniziraju semaforima
 (u Lab3a) i monitorima (u Lab3b).
 Lab3b
Koristiti dva reda uvjeta za zaustavljanje dretvi kad u njemu nema mjesta, odnosno, 
kada nema novih poruka. Stavljanje i uzimanje brojeva iz međuspremnika raditi unutar 
monitora, dok samo generiranje brojeva mora biti izvan. 
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "slucajni_prosti_broj.h"

#define KRAJ_RADA 1

//int kraj_rada = 1;
int kraj = 0;
//promijeniti broj dretvi na X, Y
int N1 = 4; //radne dretve
int N2 = 3; //neradne dretve
//ne koristimo više od 15 dodatnih dretvi
int ulaz1[15]; //n=15
int broj1[15];

uint64_t MS[10]; //n=10
uint64_t ulaz = 0;
uint64_t izlaz = 0;
int brojac = 0;

//monitori
pthread_mutex_t m;
pthread_cond_t red_puni;
pthread_cond_t red_prazni;

int br_punih;
int br_praznih;

int uzeo = 0;

void stavi_broj_u_ms (uint64_t broj) //Kad je međuspremnik pun novi se brojevi ne stavljaju u njega.
{
    MS[ulaz] = broj;
    ulaz = ( (ulaz + 1) % 10);
}



uint64_t uzmi_iz_ms () //Kad je prazan ne uzimaju se vrijednosti već vraća 0.
{
    uint64_t broj = MS[izlaz];

    if(uzeo == 0)
        return broj;
    else if ( uzeo == 1 )
    {
        izlaz = ((izlaz + 1) % 10);
        return broj;
    }

    return broj;
}

int prost (uint64_t x)
{
    uint64_t i;
    for(i = 2; i < x/2; i++){
        if(x % i == 0){
            return 1;
        }
    }
    return 0;
}


uint64_t generiraj_dobar_broj(struct gmp_pomocno *p)
{
    int i = 0, j;
    int odabrano = 0;
    uint64_t broj = 0;
    uint64_t maska = 0x03ff, x;
    while ((odabrano == 0) && (i < 10))
    {
        broj = daj_novi_slucajan_prosti_broj(p);
        for (j = 0; j < 64-10; j++)
        {
            x = (broj >> j) & maska;
            if (prost (x))
            {   
                //odabrano == 0;
                break;
            }
        }
        i++;
    }
    return broj;
}

void *radna_dretva (void *id) 
{
    int *d = id;
    struct gmp_pomocno lokalni_G;
    uint64_t x;
    int proslo = 0;
    time_t s;

    inicijaliziraj_generator (&lokalni_G, *d + 1); //(*d)+1
    time(&s);

    while (kraj != KRAJ_RADA)
    {
        if(time(NULL) != s)
        {
            x = generiraj_dobar_broj(&lokalni_G);
            
            pthread_mutex_lock(&m);
            while (br_praznih == 0)
                pthread_cond_wait(&red_prazni, &m);
            if(kraj == KRAJ_RADA)
            {
                pthread_mutex_unlock (&m);
                break;
            }

            stavi_broj_u_ms(x);
            printf("Radna dretva %d: stavio broj u MS %" PRIx64 "\n", *d, x);

            br_punih++;
            br_praznih--;

		    pthread_cond_signal(&red_puni);
		    pthread_mutex_unlock(&m);

            proslo = 1;
            s = time(NULL);
        }
        
        else if (proslo < 3)
        {
            x = generiraj_dobar_broj(&lokalni_G);
            pthread_mutex_lock(&m);

            while( br_praznih == 0){
                pthread_cond_wait(&red_prazni, &m);
            }
            if( kraj == KRAJ_RADA ){ 
                pthread_mutex_unlock (&m);
                break;
            }


            stavi_broj_u_ms(x);
            printf("Radna dretva %d: stavio broj u MS %" PRIx64 "\n", *d, x);

            br_punih++;
            br_praznih--;
            
            pthread_cond_signal(&red_puni);
            pthread_mutex_unlock (&m);
            proslo++;
        } 
    }

    obrisi_generator(&lokalni_G);
    return NULL;
}

void *neradna_dretva (void *id)
{
    int *d = id;
    uint64_t y;

	while(kraj != KRAJ_RADA)
    {
        sleep(3);
        pthread_mutex_lock(&m);
        while (br_punih == 0)
			pthread_cond_wait(&red_puni, &m);
        if(kraj == KRAJ_RADA)
        {
            pthread_mutex_unlock(&m);
            break;
        }

        y = uzmi_iz_ms();
        uzeo = 1 - uzeo;
        printf("Neradna dretva %d: Uzeo iz MS %" PRIx64 "\n", *d, y);

        br_praznih++;
        br_punih--;

		pthread_cond_signal(&red_prazni);
		pthread_mutex_unlock(&m);
        
    }
    return NULL;
}


int main(int argc, char *argv[])
{

    struct gmp_pomocno p_main;
	inicijaliziraj_generator (&p_main, 1);
  

    ulaz = 0;
    izlaz = 0;
    brojac = 0;
    
    pthread_t t[15];
    int id_dretve[15];

    /*int i;
    for( i = 0; i < 15; i++)
    {
        ulaz1[i] = 0;
        broj1[i] = 0;
    }*/

    //monitori
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&red_puni, NULL);
    pthread_cond_init(&red_prazni, NULL);

    br_punih = 0;
    br_praznih = 10; //10 ko MS


    //stvaramo 5 radnih i 4 neradne dretve
    for( int i = 0; i < N1; i++)
    {
        id_dretve[i] = i;
        if(pthread_create (&t[i], NULL, radna_dretva, &id_dretve[i]))
        {
            printf("greška-nema nove radne dretve\n");
            exit(1);
        }
    }

    for(int i = 0; i < N2; i++)
    {
        id_dretve[N1+i] = N1 + i;
        if(pthread_create(&t[N1 + i], NULL, neradna_dretva, &id_dretve[N1 + i]))
        {
            printf("greška-nema nove neradne dretve\n");
            exit(1);
        }
    }
    sleep(20);
    
    kraj = KRAJ_RADA;


    pthread_mutex_lock(&m);
/*Postavljamo br_punih i br_praznih na vrijednost vecu od 0 kako bi dretve zadovoljile 
uvijet prolaska, tj. da nemamo beskonacno cekanje. Kako je postavljeno kraj = KRAJ_RADA,
dretve koje su ostale aktivne nece nista promijeniti osim izaci iz do while petlje te 
tako zavrsiti svoj rad.*/
    br_punih = 1;
    br_praznih = 1;
    pthread_cond_broadcast(&red_puni);
    pthread_cond_broadcast(&red_prazni);
    pthread_mutex_unlock(&m);


    for(int i = 0; i < (N1 + N2); i++)
        pthread_join(t[i], NULL);


    obrisi_generator (&p_main);

    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&red_puni);
    pthread_cond_destroy(&red_prazni);
	
    return 0;
}

/*
  prevođenje:
  - ručno: gcc program.c slucajni_prosti_broj.c -lgmp -lm -o program
  - preko Makefile-a: make
  pokretanje:
  - ./program
  - ili: make pokreni
  nepotrebne datoteke (.o, .d, program) NE stavljati u repozitorij
  - obrisati ih ručno ili s make obrisi
*/

