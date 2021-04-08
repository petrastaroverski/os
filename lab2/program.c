/*Promijeniti program prve vježbe tako da se generiranje brojeva obavlja 
u posebnim dretvama (radna_dretva), a podatke iz međuspremnika uzimaju "neradne" 
dretve (neradna_dretva). Pristup međuspremniku MS zaštititi međusobnim 
isključivanjem korištenjem Lamportova algoritma.
Razlog korištenja međusobnog isključivanja u ovom zadatku jest isključivo radi 
sprečavanja paralelnog pristupa međuspremniku. */
/*Početna dretva treba stvoriti nekoliko dretvi radna_dretva (5-10) i 
nekoliko dretvi neradna_dretva (3-5). Početna dretva potom spava 20 sekundi. 
Nakon toga postavlja globalnu varijablu kraj u KRAJ_RADA te čeka kraj svih ostalih dretvi.*/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "slucajni_prosti_broj.h"

#define KRAJ_RADA 1

//int kraj_rada = 1;
int kraj = 0;
int N1 = 5; //radne dretve
int N2 = 4; //neradne dretve
//ne koristimo više od 15 dodatnih dretvi
int ulaz1[15]; //n=15
int broj1[15];

uint64_t MS[10]; //n=10
uint64_t ulaz = 0;
uint64_t izlaz = 0;
int brojac = 0;


void stavi_broj_u_ms (uint64_t broj) //Kad je međuspremnik pun novi se brojevi ne stavljaju u njega.
{
    if (brojac < 10){
        MS[ulaz] = broj;
        ulaz = (ulaz + 1) % 10;
        brojac++;
    }
    else if (brojac > 10)
    {
        //prepisujemo neprocitanu poruku
        brojac--;
        izlaz = (izlaz + 1) % 10;
    }
}

uint64_t uzmi_iz_ms () //Kad je prazan ne uzimaju se vrijednosti već vraća 0.
{
    uint64_t broj = MS[izlaz];
    if (brojac > 0){
        MS[izlaz] = 0; //radi ispisa označiti da je opet slobodno
        izlaz = (izlaz+1) % 10;
        brojac--;
        return broj;
    }
    //else
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

//Lampartov algoritam medusobnog iskljucivanja
void udi_u_KO(int i)
{
    int j;

    int N = N1+N2; //ukupan broj dretvi
    ulaz1[i] = 1; //uzimanje broja dretve

    broj1[i] = 0;
    for (j = 1; j < N; j++)
    {
        if(broj1[j] > broj1[i])
            broj1[i] = broj1[j];
    }
    broj1[i]++;
    //uzimanje broja je završeno
    ulaz1[i] = 0;

    //cekanje dretve s manjim brojem
    for ( j = 0; j < N; j++)
    {
        //ako dretva ceka broj, pricekamo da zavrsi
        while(ulaz1[j] == 1)
        ;
        //cekamo kao j ima prednost
        while( ulaz1[j] != 0 && (broj1[j] < broj1[i] || (broj1[j] == broj1[i] && (j < i))))
        ;
    }
}

void izadi_iz_KO(int i) 
{
    broj1[i] = 0;
}


void *radna_dretva (void *id)
{
    int *d = id;
    struct gmp_pomocno lokalni_G;
    uint64_t x;

    inicijaliziraj_generator (&lokalni_G, *d+1); //(*d)+1

    while (kraj != KRAJ_RADA)
    {
        x = generiraj_dobar_broj(&lokalni_G);
        //taj G koristiti i u daj_noiv_slucajan_prosti_broj (&G);

        udi_u_KO(*d);

        stavi_broj_u_ms(x);
        printf("Radna dretva %d: stavio broj u MS %" PRIx64 "\n", *d, x);

        izadi_iz_KO(*d);
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
        udi_u_KO(*d);

        y = uzmi_iz_ms();
        printf("Neradna dretva %d: Uzeo iz MS %" PRIx64 "\n", *d, y);

        izadi_iz_KO(*d);
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
        pthread_join(t[i], NULL);

    
    obrisi_generator (&p_main);
	
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