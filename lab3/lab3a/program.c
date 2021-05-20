/*Promijeniti program druge vježbe tako da se dretve sinkroniziraju semaforima
 (u Lab3a) i monitorima (u Lab3b).
 Lab3a
Koristiti tri semafora za pristup međuspremniku: jedan za KO, drugi za provjeru ima li 
slobodnih mjesta u međuspremniku te treći za provjeru ima li poruka u međuspremniku. 
Dretve koje ne mogu staviti/uzeti poruke će se na tim semaforima privremeno blokirati. 
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

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

//semafori
sem_t KO;
sem_t PUNI;
sem_t PRAZNI;

void stavi_broj_u_ms (uint64_t broj) //Kad je međuspremnik pun novi se brojevi ne stavljaju u njega.
{
    if (brojac < 10){
        MS[ulaz] = broj;
        ulaz = (ulaz + 1) % 10;
        brojac++;
    }
}



uint64_t uzmi_iz_ms () //Kad je prazan ne uzimaju se vrijednosti već vraća 0.
{
    //uint64_t broj = MS[izlaz];
        MS[izlaz] = 0; //radi ispisa označiti da je opet slobodno
        izlaz = (izlaz+1) % 10;
        brojac--;
        return izlaz;
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

    inicijaliziraj_generator (&lokalni_G, *d + 1); //(*d)
    time(&s);

    while (kraj != KRAJ_RADA)
    {
        if(time(NULL) != s)
        {
            x = generiraj_dobar_broj(&lokalni_G);
            sem_wait(&PRAZNI); //početno jednak veličini MS
            if ((kraj == KRAJ_RADA)) break;
		    sem_wait(&KO);     //početno 1

            stavi_broj_u_ms(x);
            printf("Radna dretva %d: stavio broj u MS %" PRIx64 "\n", *d, x);

            sem_post(&KO);
		    sem_post(&PUNI);

            proslo = 1;
            time(&s);
        }
        
        else if (proslo < 3)
        {
            x = generiraj_dobar_broj(&lokalni_G);
            sem_wait(&KO);

            stavi_broj_u_ms(x);
            printf("Radna dretva %d: stavio broj u MS %" PRIx64 "\n", *d, x);
            sem_post(&KO);
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

        sem_wait(&PUNI); //početno 0
        if ((kraj == KRAJ_RADA)) break;
		sem_wait(&KO);

        y = uzmi_iz_ms();
        printf("Neradna dretva %d: Uzeo iz MS %" PRIx64 "\n", *d, y);

        sem_post(&KO);
		sem_post(&PRAZNI);
        
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

    //semafori
    sem_init(&KO, 0, 1);
    sem_init(&PUNI, 0, 0);
    sem_init(&PRAZNI, 0, 10); //ovaj 10 je  N u MS

    int i;
    for( i = 0; i < 15; i++)
    {
        ulaz1[i] = 0;
        broj1[i] = 0;
    }


    //stvaramo 5 radnih i 4 neradne dretve
    for( i = 0; i < N1; i++)
    {
        id_dretve[i] = i;
        if(pthread_create (&t[i], NULL, radna_dretva, &id_dretve[i]))
        {
            printf("greška-nema nove radne dretve\n");
            exit(1);
        }
    }

    for( i = 0; i < N2; i++)
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

    for(i = 0; i < (N1 + N2); i++)
        sem_post(&PRAZNI);

    for(i = 0; i < (N1 + N2); i++)
        sem_post(&PUNI);

    
    obrisi_generator (&p_main);

    sem_destroy(&KO);
    sem_destroy(&PUNI);
    sem_destroy(&PRAZNI);
	
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
