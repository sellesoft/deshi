/*
    Implementation of the dining philosophers problem 
    https://en.wikipedia.org/wiki/Dining_philosophers_problem

    Example for using deshi's threading api
*/

#include "deshi.h"

mutex forks[5];

// since we want to keep our interfaces C compatible, we have to pass information to a
// threaded function through a struct 
struct Philosopher{
    str8 name;
    u8 id;
};

// function representing a philosopher
void philosopher(void* mein){
    Philosopher* me = (Philosopher*)mein;
    threader_set_thread_name(me->name);

    switch(me->id){
        
    }

}   

int main(){
    deshi_init();

    for(u32 i = 0; i != (u32)-10; i--){
        Log("", i%5);
    }

    threader_spawn_thread(4);
    Philosopher philosophers[] = { {STR8("Aristotle"),0}, {STR8("Plato"), 1}, {STR8("Socrates"), 2}, {STR8("Epicurus"), 3}, {STR8("Pythagoras"), 4} };
    forI(5) forks[i] = mutex_init();
    threader_add_job({&philosopher,&philosophers[0]});
    threader_add_job({&philosopher,&philosophers[1]});
    threader_add_job({&philosopher,&philosophers[2]});
    threader_add_job({&philosopher,&philosophers[3]});
    philosopher(&philosophers[4]); 
}