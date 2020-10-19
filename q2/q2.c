#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<semaphore.h>
#include<time.h>
#include<unistd.h>
void * pharma_func(void*);
void * vax_func(void*);
void * student_func(void*);
int antibody_test(int,int);
typedef struct Pharma{
    int pharma_index;
    float pharma_probab;
    int no_batches;
    int batch_capacity;
    pthread_mutex_t pharma_mutex;
    pthread_t pharma_thread;
    pthread_cond_t vax_cond;
}Pharma;

typedef struct Vac_zones{
    int vax_index;
    int slot_capacity;
    int slots;
    int occupied;
    int refill;
    float batch_probab;
    pthread_mutex_t vax_mutex;
    pthread_t vax_thread;
    pthread_cond_t stud_cond;
}Vax;

typedef struct Students{
    int student_index;
    pthread_t student_thread;
    int round_no;
    int status;
}Students;

void pharma_ready(Pharma*);
void vax_ready(Vax*);
void students_waiting(int);
void students_vaccinating(int,int);

int no_pharma, no_vax, no_students;
Pharma pharma[500];
Vax vax[500];
Students students[500];
Students studentsR2[500];
Students studentsR3[500];
int r2=0,r3=0;

int main()
{
    int i;
    srand(time(0));
    printf("Enter no of pharma companies :");
    scanf("%d",&no_pharma);
    printf("Enter no of Vaccination Zones :");
    scanf("%d",&no_vax);
    printf("Enter no of Students registered :");
    scanf("%d",&no_students);
    printf("Enter respective probability of success for %d companies :",no_pharma);
    for(i=0;i<no_pharma;i++)
    {
        scanf("%f",&(pharma[i].pharma_probab));
    }

    for(i=0;i<no_pharma;i++)
    {
        pharma[i].pharma_index=i+1;
        pthread_mutex_init(&(pharma[i].pharma_mutex),NULL);
    }
    for(i=0;i<no_vax;i++)
    {
        vax[i].vax_index=i+1;
        vax[i].refill=0;
        pthread_mutex_init(&(vax[i].vax_mutex),NULL);
    }
    for(i=0;i<no_students;i++)
    {
        students[i].student_index=i+1;
        students[i].round_no=0;
        students[i].status=0;
    }

    printf("\n\n\t\t\t\t\t\t\t\t\x1B[1;31mBeginning simulation:\n");
    for(i=0;i<no_pharma;i++)
        pthread_create(&(pharma[i].pharma_thread), NULL,pharma_func,&pharma[i]);    
    for(i=0;i<no_vax;i++)
        pthread_create(&(vax[i].vax_thread), NULL,vax_func,&vax[i]);
    for(i=0;i<no_students;i++)
        pthread_create(&(students[i].student_thread), NULL,student_func,&students[i]);
    for(i=0;i<no_students;i++)
    {
        pthread_join(students[i].student_thread,0);
    }
    printf("\n\n\t\t\t\t\t\t\x1B[1;32mSimulation ov'r ! \n\n");

    for(i=0;i < no_pharma;i++)
    {
        pthread_mutex_destroy(&(pharma[i].pharma_mutex));
    }
    for(i=0;i < no_vax;i++)
    {
        pthread_mutex_destroy(&(vax[i].vax_mutex));
    }
    //printf("Cookies left in the jar: %d\n", var);

    return 0;

}

void *pharma_func(void *args)
{
    Pharma *pharma = (Pharma*)args;
    while(1)
    {
        int batches= rand()%5+1;
        printf("\x1B[1;32mPharmaceutical Company %d is preparing %d batches of vaccines which have success probability %f\n",pharma->pharma_index,batches,pharma->pharma_probab);
        int sleeep=rand()%4+2;
        sleep(sleeep);
        pthread_mutex_lock(&(pharma->pharma_mutex));
        pharma->no_batches=batches;
        pharma->batch_capacity=rand()%11+10;
        printf("\t\x1B[1;32mPharmaceutical Company %d has prepared %d batches of vaccines which have success probability %f.Waiting for all the vaccines to be used to resume production..\n",pharma->pharma_index,batches,pharma->pharma_probab);
        pharma_ready(pharma);
    }
}

void pharma_ready(Pharma *pharma)
{
    while(1)
    {
        if(pharma->no_batches==0)
            break;
        else
        {
            pthread_cond_wait(&(pharma->vax_cond),&(pharma->pharma_mutex));
        }
    }
    printf("\t\t\t\t\x1B[1;31mAll the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now...\n",pharma->pharma_index);
    pthread_mutex_unlock(&(pharma->pharma_mutex));
}

void * vax_func(void *args)
{
    Vax *vax = (Vax*)args;
    while(1)
    {
        int flag = 0,i;
        for(i=0; i < no_pharma; i++)
        {
            pthread_mutex_lock(&(pharma[i].pharma_mutex));
            if(pharma[i].no_batches>0)
            {
                flag=1;
                vax->slot_capacity=pharma[i].batch_capacity;
                pharma[i].no_batches--;
                if(vax->refill==0)
                {
                    printf("\x1B[1;33mPharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %f\n",i+1,vax->vax_index,pharma[i].pharma_probab);
                    vax->batch_probab=pharma[i].pharma_probab;
                    vax->refill++;
                }
                else
                {
                    printf("\x1B[1;33mPharmaceutical Company %d has delivered vaccines to Vaccination zone %d,which has success probability %f resuming vaccinations now..\n",i+1,vax->vax_index,pharma[i].pharma_probab);
                    vax->batch_probab=pharma[i].pharma_probab;
                }
                pthread_cond_signal(&(pharma[i].vax_cond));
                pthread_mutex_unlock(&(pharma[i].pharma_mutex));
                break;
            }
            pthread_cond_signal(&(pharma[i].vax_cond));
            pthread_mutex_unlock(&(pharma[i].pharma_mutex));
        }
        while(flag)
        {
            pthread_mutex_lock(&(vax->vax_mutex));
            if(vax->slot_capacity == 0)
            {
                printf("\t\t\t\t\t\t\t\t\x1B[1;36mVax zone %d ran out of vaccines , waiting for next batch of vaccines to resume vaccination ..\n",vax->vax_index);
                pthread_mutex_unlock(&(vax->vax_mutex));
                break;
            }
            vax->slots=rand()%8+1;
            vax->occupied=0;
            if(vax->slots > vax->slot_capacity)
                vax->slots= vax->slot_capacity;
            
            vax->slot_capacity = vax->slot_capacity - vax->slots;
            printf("\t\t\t\t\t\t\x1B[1;35mVax Zone %d ready to vaccinate with %d slots\n",vax->vax_index, vax->slots);
            vax_ready(vax);
        }
    }
}

void vax_ready(Vax *vax)
{
    printf("\x1B[1;37mVaccination zone %d entering Vaccination Phase\n",vax->vax_index);
    while(1)
    {
        if(vax->slots == vax->occupied)
        {
            printf("\t\t\t\t\t\t\x1B[1;33mAll slots filled for vax zone %d, waiting for vacancy ...\n",vax->vax_index);
            break;
        }
        else
        {
            pthread_cond_wait(&(vax->stud_cond), &(vax->vax_mutex));
        }
    }
    pthread_mutex_unlock(&(vax->vax_mutex));
}

void *student_func(void *args)
{
    Students *stud = (Students*)args;
    if(stud->status==1)
        return NULL;
    int t= rand()%10;
    sleep(t);
    stud->round_no++;
    printf("\t\t\t\t\t\t\x1B[1;36mStudent %d has arrived for his vaccination (round no :%d )\n",stud->student_index,stud->round_no);
    //stud->round_no++;
    //printf("(Round no : %d)\n",stud->round_no);
    students_waiting(stud->student_index);
}

void students_waiting(int index)
{
    int flag = 0,i;
    while(!flag)
    {
        for(i=0; i < no_vax;i++)
        {
            pthread_mutex_lock(&(vax[i].vax_mutex));
            if((vax[i].slots - vax[i].occupied) > 0)
            {
                vax[i].occupied++;
                flag=1;
                printf("\t\t\x1B[1;31mStudent %d waiting to be allocated a slot  on a Vaccination zone\n",index);
                students_vaccinating(i, index);
                break;
            }

            pthread_cond_signal(&(vax[i].stud_cond));
            pthread_mutex_unlock(&(vax[i].vax_mutex));
        }
    }
}
void students_vaccinating(int i , int index)
{
    printf("\t\t\x1B[1;33mSTUDENT %d Assigned a slot at Vaccination Zone %d, waiting to be vaccinated ..\n",index, i+1);
    sleep(1);
    printf("\t\t\t\t\x1B[1;35mSTUDENT %d VACCINATED AT VAX_zone %d which had success probability %f\n",index, i+1,vax[i].batch_probab);
    int at=antibody_test(index,i+1);
    if(at==1)
    {
        students[index-1].status=1;
        pthread_cond_signal(&(vax[i].stud_cond));
        pthread_mutex_unlock(&(vax[i].vax_mutex));
    }
    else
    {
        pthread_join(students[index-1].student_thread,0);
        pthread_create(&(students[index-1].student_thread), NULL,student_func,&students[index-1]);
        //student_func(&students[index-1]);
        //pthread_cond_signal(&(vax[i].stud_cond));
        pthread_mutex_unlock(&(vax[i].vax_mutex));
    }
}
int antibody_test(int index,int zone_index)
{
    printf("\t\t\t\t\t\x1B[1;37mStudent %d going for antibody test :\n",index);
    int i = index-1;
    float r = (rand()/(double)RAND_MAX);
    //float r= 1.0;
    //printf("\n\nvalue of r :%f , zone no : %d zone batch probab:%f\n\n",r,zone_index,vax[zone_index].batch_probab);
    if(students[i].round_no==3)
    {
        if(r>vax[zone_index].batch_probab)
            printf("\t\t\t\t\t\x1B[1;32mStudent %d Tested Positive for antibodies\n",index);
        else
        {
            printf("\t\t\t\t\t\x1B[1;31mStudent %d Tested Negative for antibodies\n",index);
            printf("\x1B[1;32m3 trials of Vaccination complete, Student %d sent home for online semester,Take care!\n",index);
        }
        return 1;
    }
    if(r>vax[zone_index].batch_probab)
    {
        printf("\t\t\t\t\t\x1B[1;32mStudent %d Tested Positive for antibodies\n",index);
        return 1;
    }
    else
    {
        printf("\t\t\t\t\t\x1B[1;31mStudent %d tested negative for antibodies,will try again .. \n",index);
        return 0;
    }
    
}
