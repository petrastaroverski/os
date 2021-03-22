/*Ostvariti program koji generira proste brojeve te neke od njih stavlja u m
eđuspremnik i koristi (ispisuje).
Proste brojeve generirati korištenjem biblioteke GMP (upute ispod).
Od dobivenog prostog broja analizirati grupe od uzastopnih 10 bitova. 
Ako je bilo koja takva grupa bitova zapravo prost broj, onda ga proslijediti dalje 
(odabrati). U protivnom tražiti idući prosti. 
Ako se niti nakon 10 brojeva nije pronašao onaj s prostim u nekih 10 bitova, 
onda zadnji broj (10.) odabrati za dalje.
Odabrane brojeve stavljati u ograničeni međuspremnik kojeg cirkularno popunjavati.
U jednoj sekundi generirati najviše tri broja (pogledati skicu rješenja ispod).
Svake sekunde uzeti prvi broj iz međuspremnika (cirkularno) i ispisati ga.
Nakon desetog ispisa završiti program.
Koristiti Makefile i cilj pokreni za pokretanje programa 
(za automatsku provjeru koda u repozitoriju). */

#include <stdio.h>
#include <time.h>

#include "slucajni_prosti_broj.h"

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
}

uint64_t uzmi_iz_ms () //Kad je prazan ne uzimaju se vrijednosti već vraća 0.
{
    if (brojac > 0){
        uint16_t broj = MS[izlaz];
        MS[izlaz] = 0; //radi ispisa označiti da je opet slobodno
        izlaz = (izlaz+1) % 10;
        brojac--;
        return broj;
    }
    else
        return 0;
}

int prost (uint16_t x)
{
    uint16_t i;
    for(i = 2; i < x/2; i++){
        if(x % i == 0){
            return 1;
        }
    }
    return 0;
}
/*int mpz_probab_prime_p (mpz_t a, int reps);

Funkcija koja ispituje primarnost broja a. Ako je vraćena vrijednost 0
broj nije prost, 1 označava da je vjerojatno prost, a 2 da je
sigurno prost. Preporučene vrijednosti 'reps'-a je od 5 do 10.*/

struct gmp_pomocno p;
uint64_t generiraj_dobar_broj()
{
    int i = 0, j;
    int odabrano = 0;
    int broj = 0;
    uint64_t maska = 0x03ff, x;
    while ((odabrano == 0) && (i < 10))
    {
        broj = daj_novi_slucajan_prosti_broj(&p);
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

int main(int argc, char *argv[])
{

	//struct gmp_pomocno p;

	inicijaliziraj_generator (&p, 0);

    //glavni program

    uint16_t broj;
    int broj_ispisa = 0,  broj_odabranih = 0;

    time_t t;
    time(&t);


    while (broj_ispisa < 10){
        //ne generirati više od 3 broja u sekundi
        if (broj_odabranih < 3){
            broj = generiraj_dobar_broj();
            stavi_broj_u_ms(broj);
        }
        //uzimanje i ispis u jednoj sekundi
        if (time(NULL) != t){
            broj = uzmi_iz_ms();
            printf ("%" PRIx64 "\n", broj);
            broj_ispisa++;
            time(&t);
            broj_odabranih = 0;
        }
    }

    obrisi_generator (&p);
	
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
