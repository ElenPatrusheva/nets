#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <glob.h>

#define BUFFER 1024
#define THREADS 20
#define HOST_NAME 20
#define FILE_NUMBER 10
#define NET_SIZE 200
#define IP 16
#define PORT 6
#define MAX_CON 5

struct sockaddr_in root, this_host;
struct Node{
    int numb_of_files;
    struct sockaddr_in addr;
    char name[BUFFER];
    char files[FILE_NUMBER][BUFFER];
};

struct fd_client{
    int socket;
    struct sockaddr_in addr;
};
char root_name[HOST_NAME];
int tid_is_free[THREADS];
int numb_of_nodes;
int numb_of_files;
char this_name[HOST_NAME];
char my_files[FILE_NUMBER][BUFFER];
struct Node * kdb[NET_SIZE]; //incorrect infa 100
pthread_mutex_t thread_mutex;
pthread_mutex_t kdb_mutex;
pthread_mutex_t cdb_mutex;
pthread_mutex_t bldb_mutex;
struct sockaddr_in  cdb[NET_SIZE];////!!!!
struct sockaddr_in  bldb[NET_SIZE];
pthread_t tid[THREADS];
int n_cdb[NET_SIZE];
int b_n;
int c_n;
char my_info[BUFFER];

void get_my_info(char * info){ 
    sprintf(info, "%s:%s:%u:", this_name, inet_ntoa(this_host.sin_addr), ntohs(this_host.sin_port));  
    for(int i = 0; i < numb_of_files; i++){
        strcat(info, my_files[i]);
        if(i != numb_of_files - 1){
            strcat(info, ",");
        }
    }
}

int ping(struct sockaddr_in server_to_call, char * my_info){
    //printf("I want to ping server. Ip: %s. Port: %u\n", inet_ntoa(server_to_call.sin_addr), ntohs(server_to_call.sin_port)); 
    int connfd;
    //struct sockaddr_in client;
    connfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connfd < 0){
        //printf("Ping: sock creation: fail\n");
        return 1;
    }
    else{
        //printf("Ping: sock creation: success\n");
    }
    if (connect(connfd, (struct sockaddr *)&server_to_call, sizeof(server_to_call)) != 0){
        //printf("Ping: connect: fail\n");
        return 2;
    }
    else {
        //printf("Ping: connect: success\n");
    }
    int message = 1;
    if (send(connfd, &message, sizeof(message), 0) == -1){
        //printf("Send 1: fail\n");
        close(connfd);
        return 1;
    }
    else{
        //printf("Send 1: success\n");
    }
    //printf("%s\n\n\n", my_info);
    //printf("MY info is: %s\n", my_info);
    if (send(connfd, my_info, BUFFER, 0) == -1){
        //printf("Send node info: fail\n");
        close(connfd);
        return 1;
    }
    else{
        //printf("Send node info: success\n");
    }
    /*
    if (send(connfd, &numb_of_nodes, sizeof(int), 0) == -1){
        //printf("Send number of nodes: fail\n");
        close(connfd);
        return 1;
    }
    else{
        //printf("Send number of nodes: success\n");
    }
    int n_nodes = numb_of_nodes;
    char node_info[BUFFER];
    for (int i = 0; i < n_nodes; i++){
        
        get_info(node_info, kdb[i]);
        printf("Node number %d info is:%s\n", i, node_info);
        if (nodes[i]->addr.sin_addr.s_addr == server_to_call.sin_addr.s_addr &&
                nodes[i]->addr.sin_port == server_to_call.sin_port){
            continue;
        }
        if (send(connfd, node_info, BUFFER, 0) == -1){
            //printf("Node %d info sending: fail\n", i);
            close(connfd);
            return 1;
        }
        else{
            //printf("Node %d info sending: success\n", i);
        }
    }*/
    close(connfd);
    return 0;
}

void set_my_files(){
    const char * pattern = "./*.txt";     
    glob_t pglob;
    glob(pattern, GLOB_ERR, NULL, &pglob);
    char file_name[BUFFER];
    numb_of_files = 0;
    for (int i = 0; i < pglob.gl_pathc; i++){
        strcpy(file_name, pglob.gl_pathv[i]);
        memmove(file_name, file_name + 2, strlen(file_name));
        strcpy(my_files[numb_of_files], file_name);
        numb_of_files ++;
    }
}

int init(){    
    char name [HOST_NAME];
    printf("Enter this host name: ");
    scanf("%s", name);
    strcpy(this_name, name);
    char ip[IP];
    printf("Enter this ip: ");
    scanf("%s", ip);
    this_host.sin_family = AF_INET;
    this_host.sin_addr.s_addr = inet_addr(ip);
    printf("Enter this port: ");
    unsigned long port;
    scanf("%lu", &port);
    this_host.sin_port = htons(port);
    numb_of_nodes = 0;
    set_my_files();
    return 0;
}

int main(int argc, char **argv){
    if (argc != 3){
        printf("Incorrect number of argumets\n");
        exit(0);
    }
    root.sin_family = AF_INET;
    root.sin_addr.s_addr = inet_addr(argv[1]);
    root.sin_port = htons(strtoul(argv[2], (char **)NULL, 10));//not sure
    init();
    get_my_info(my_info); 
    while(1){
        ping(root, my_info);
    }
}
