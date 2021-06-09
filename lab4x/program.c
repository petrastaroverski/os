#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "slucajni_prosti_broj.h"

#define dim_mem 50

int KRAJ_RADA = 1;

//int kraj_rada = 1;
int kraj = 0;
//promijeniti broj dretvi na X, Y
int N1 = 4; //radne dretve
int N2 = 3; //neradne dretve
//ne koristimo više od 15 dodatnih dretvi
int ulaz1[15]; //n=15
int broj1[15];
int *string;
int size=dim_mem;

uint64_t MS[10]; //n=10
uint64_t ulaz = 0;
uint64_t izlaz = 0;
int brojac = 0;

//monitori
pthread_mutex_t m;
pthread_cond_t red_puni;
pthread_cond_t red_prazni;
pthread_cond_t nema_mem;

int br_punih;
int br_praznih;

void stavi_broj_u_ms (uint64_t broj) //Kad je međuspremnik pun novi se brojevi ne stavljaju u njega.
{
        MS[ulaz] = broj;
        ulaz = (ulaz + 1) % 10;
    
}

uint64_t uzmi_iz_ms () //Kad je prazan ne uzimaju se vrijednosti već vraća 0.
{
   	uint64_t broj = MS[izlaz];
	MS[izlaz] = 0;
	izlaz = (izlaz + 1) % 10;
		
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

void ispis()
{
 int i;
 printf("memorija: ");
 for(i=0; i < dim_mem; i++){
   if(string[i] != 0)  
      printf("%d", string[i]);
    else 
      printf("-");   
 }
 
 printf("\n");

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

int zauzmi (int p, int id)
{
    int n, prvi = 50, drugi = 51, i = 0; //n = i , v= p, i=br

    n = 0;
    while ((n + p) < size)
    {
        while ((n < size) && (string[n] != 0))
            n++;
        if((n + p) >= size)
            break;
        while(i < p)
        {
            if(string[n + i] != 0)
                break;
            i++;
        }
        n = n + i;
        if(i == p)
        {
            while ((n < size) && (string[n] == 0))
            {
                n++;
                i++;
            }
            i--;
            if(i < drugi)
            {
                drugi = i;
                prvi = n - i -1;
            }
        }
        
    }

    if (prvi == 50)
        return -1;
    n = prvi;
    i = 0;
    while (i < p)
    {
        string[n] = id;
        n++;
        i++;
    }
    return prvi;
    
}

int oslobodi(int m, int id)
{
  int i = m;
  while((i < size) && (string[i] == id))
  {
    string[i] = 0;
    i++;
  }
  return 0;
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
            time(&s);
        }
        
        else if (proslo < 3)
        {
            x = generiraj_dobar_broj(&lokalni_G);
            pthread_mutex_lock(&m);

			while( br_praznih == 0 ){
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
        printf("Neradna dretva %d: Uzeo iz MS %" PRIx64 "\n", *d, y);

        br_praznih++;
        br_punih--;

		pthread_cond_signal(&red_prazni);

		int x;
        do{
			x = zauzmi( y % 20, *d);
			if( x == -1 ){
				if (size == dim_mem) {
    				size = 2 * dim_mem;
    				string = realloc (string, 2 * dim_mem * sizeof (int));
    				int i;
    				for (i = dim_mem; i < 2 * dim_mem; ++i)
        			string[i] = -1;
				}
				pthread_cond_wait(&nema_mem, &m);
			}
		}while( x == -1);

        ispis();

		pthread_mutex_unlock(&m);

		sleep(y % 5);

		pthread_mutex_lock(&m);
		printf("%d potrosio broj: %" PRIx64 "\n", *d, y);
		oslobodi(x, *d);
        ispis();
	
		pthread_cond_broadcast(&nema_mem);
		pthread_mutex_unlock(&m);
        
    }
    return NULL;
}



int main(int argc, char *argv[])
{

    struct gmp_pomocno p_main;
	inicijaliziraj_generator (&p_main, 1);
    int i;
    string = (int*) malloc (dim_mem * sizeof (int));
    
    for (i =0; i < dim_mem; i++)
        string[i] = 0;
  

    ulaz = 0;
    izlaz = 0;
    brojac = 0;
    
    pthread_t t[15];
    int id_dretve[15];



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