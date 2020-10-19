#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
typedef struct merge_thread{
    long long int n;
    int *arr;
}merge_thread;
void merge_sort(int*,int);
void merge_sort_fork(int*,int);
void *merge_sort_threaded(void*);
void merge(int*,int,int*,int,int*,int);
void selection_sort(int*,int);
pid_t waitpid(pid_t pid, int *wstatus, int options);
int main()
{
    clock_t start,end;double total_time_naive,total_time_fork,total_time_threaded;
    long long int n,shmid1,shmid2,shmid3,i;
    key_t key1 = IPC_PRIVATE,key2=IPC_PRIVATE,key3=IPC_PRIVATE;
    printf("Enter the value of n : ");
    scanf("%lld",&n);
    int *arr,*arr2,*arr3;
    size_t shm_size = sizeof(int)*(n+5); 
    shmid1=shmget(key1,shm_size,IPC_CREAT | 0666);
    shmid2=shmget(key2,shm_size,IPC_CREAT | 0666);
    shmid3=shmget(key3,shm_size,IPC_CREAT | 0666);

    if(shmid1<0 || shmid2<0 || shmid3<0)
    {
        perror("Error in shmget()");
        exit(EXIT_FAILURE);
    }

    arr=shmat(shmid1,NULL,0);
    arr2=shmat(shmid2,NULL,0);
    arr3=shmat(shmid3,NULL,0);
    if(arr==(int*)-1 || arr2==(int*)-1 ||arr3==(int*)-1)
    {
        perror("Error in shmat()");
        exit(EXIT_FAILURE);
    }

    //printf("\n\x1B[1;33mRandom Array of size %lld :\x1B[1;0m\n",n);
    printf("\n\x1B[1;33mInput Array of size %lld :\x1B[1;0m\n",n);
    srand(time(0));
    for(i=0; i < n; i++)
    {
        scanf("%d",&arr[i]);
        //arr[i]=rand()%100;
        arr2[i]=arr[i];
        arr3[i]=arr[i];
        //printf("%d ",arr[i]);
    }
    printf("\n\n");
    
    start=clock();
    merge_sort(arr,n);
    end=clock()-start;
    total_time_naive=((double)end)/CLOCKS_PER_SEC;
    printf("\x1B[1;33mSorted array (naive):\x1B[1;0m \n");
    for(i=0; i < n; i++)
        printf("%d ",arr[i]);
    printf("\n");
    printf("\t\t\t\t\x1B[1;36mNormal Merge Sort took  :  \x1B[1;31m%f seconds\x1B[1;0m\n\n",total_time_naive);
    shmdt(arr);
    shmctl(shmid1,IPC_RMID,NULL);

    
    start=clock();
    merge_sort_fork(arr2,n);
    end=clock()-start;
    total_time_fork=((double)end)/CLOCKS_PER_SEC;
    printf("\x1B[1;33mSorted array (concurrent): \x1B[1;0m\n");
    for(i=0; i < n; i++)
        printf("%d ",arr2[i]);
    printf("\n");
    printf("\t\t\t\t\x1B[1;36mConcurrent Merge Sort took:\x1B[1;31m%f seconds\x1B[1;0m\n\n",total_time_fork);
    shmdt(arr2);
    shmctl(shmid2,IPC_RMID,NULL);

    pthread_t tid;
    merge_thread th;
    th.n=n;
    th.arr=arr3;
    //exit(0);
    start=clock();
    pthread_create(&tid,NULL,merge_sort_threaded,&th);
    //printf("\nthreaded out\n");
    pthread_join(tid,NULL);
    end=clock()-start;
    total_time_threaded=((double)end)/CLOCKS_PER_SEC;
    printf("\x1B[1;33mSorted array (threaded): \x1B[1;0m\n");
    for(i=0; i < n; i++)
        printf("%d ",th.arr[i]);
    printf("\n\t\t\t\t\x1B[1;36mThreaded  Merge  Sort took:\x1B[1;31m%f seconds\x1B[1;0m\n\n",total_time_threaded);

    printf("\x1B[1;33m\n\n\t\t\t\t\t\tREPORT\x1B[1;0m\n\n\t\t\t\x1B[1;32mNormal Merge sort     :\t\t\t\x1B[1;36m%f secs\n",total_time_naive);
    printf("\t\t\t\x1B[1;32mConcurrent Merge Sort :\t\t\t\x1B[1;36m%f secs\n",total_time_fork);
    printf("\t\t\t\x1B[1;32mThreaded  Merge  Sort :\t\t\t\x1B[1;36m%f secs\n\n",total_time_threaded);
    
}
void selection_sort(int *arr,int n)
{
    int i, j, index, temp;
    for(i=0; i<n-1; i++)
    {
        index=i;
        for(j=i+1; j<n; j++)
        {
            if(arr[j]<arr[index])
            {
                index=j;
            }
        }
        temp=arr[i];
        arr[i]=arr[index];
        arr[index]=temp;
    }
}
void *merge_sort_threaded(void *Args)
{
    merge_thread *args=(merge_thread*) Args;
    int n= args->n;
    int *arr=args->arr;
    if(n<5)
    {
        selection_sort(arr,n);
        return NULL;
    }

    int mid= n/2;
    int *left=arr,*right=arr+mid;

    merge_thread l_thread;
    l_thread.n=mid;
    l_thread.arr=left;
    pthread_t left_tid;
    pthread_create(&left_tid,NULL,merge_sort_threaded,&l_thread);

    merge_thread r_thread;
    r_thread.n=n-mid;
    r_thread.arr=right;
    pthread_t right_tid;
    pthread_create(&right_tid,NULL,merge_sort_threaded,&r_thread);

    pthread_join(left_tid,NULL);
    pthread_join(right_tid,NULL);

    merge(left,mid,right,n-mid,arr,n);
}
void merge_sort_fork(int *arr,int n)
{
    if(n<5)
    {
        selection_sort(arr,n);
        return;
    }
    int mid=n/2; int i;
    pid_t pid1,pid2;int *left=arr,*right=arr+mid;
    pid1=fork();
    if(pid1>=0)
    {
        if(pid1==0)
        {
            
            merge_sort_fork(left,mid);
            exit(EXIT_SUCCESS);
        }
        else
        {
            pid2=fork();
            if(pid2>=0)
            {
                if(pid2==0)
                {
                   
                    merge_sort_fork(right,n-mid);
                    exit(EXIT_SUCCESS);
                }
            }
            else
            {
                perror("Fork failed");
                exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        perror("Fork failed\n");
        exit(EXIT_FAILURE);
    }
    int wstatus;
    waitpid(pid1, &wstatus,0);
    waitpid(pid2, &wstatus,0);
    merge(left,mid,right,n-mid,arr,n);
}
void merge_sort(int *arr,int n)
{
    if(n<5)
    {
        selection_sort(arr,n);
        return;
    }   
    else
    {
        int mid=n/2;
        int *left=(int*)malloc(mid*sizeof(int));
        int *right=(int*)malloc((n-mid)*sizeof(int));
        int i;
        for(i=0;i<mid;i++)
            left[i]=arr[i];
        for(i=mid;i<n;i++)
            right[i-mid]=arr[i];
        merge_sort(left,mid);
        merge_sort(right,n-mid);
        merge(left,mid,right,n-mid,arr,n);
    }
}
void merge(int *left,int len_l,int *right,int len_r,int *arr,int len_arr)
{
    
    int i=0,j=0,k=0;int temp[len_arr];
    while(i<len_l && j<len_r)
    {
        if(left[i]<=right[j])
            temp[k]=left[i++];
        else
            temp[k]=right[j++];
        k++;
    }
    if(i<len_l)
    {
        while(i<len_l)
        {
            temp[k]=left[i++];
            k++;
        }
    }
    else
    {
        while(j<len_r)
        {
            temp[k]=right[j++];
            k++;
        }
    }
    for (i=0;i<len_arr;i++)
        arr[i]=temp[i];
}