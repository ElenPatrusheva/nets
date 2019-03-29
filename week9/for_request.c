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
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define HOST_NAME 20
#define FILE_NUMBER 10
#define FILE_NAME 20
#define NET_SIZE 200
#define IP 16
#define PORT 6
#define WORD 25

struct sockaddr_in root, this_host;
struct Node{
    int numb_of_files;
    struct sockaddr_in addr;
    char name[FILE_NAME];
    char files[FILE_NUMBER][FILE_NAME];
};

pthread_t tid[THREADS];
char root_name[HOST_NAME];
int tid_is_free[THREADS];
int numb_of_nodes;
int numb_of_files;
char this_name[HOST_NAME];
char my_files[FILE_NUMBER][FILE_NAME];
struct Node * nodes[NET_SIZE]; //incorrect infa 100

int node_ind(struct Node * new_node){
    for (int i = 0; i < numb_of_nodes; i++){
        if ((nodes[i]->addr.sin_addr.s_addr == new_node->addr.sin_addr.s_addr && new_node->addr.sin_port == nodes[i]->addr.sin_port) && strcmp(nodes[i]->name, new_node->name)==0){
            //printf("Parse node: this node is already known\n");
            return i;
        }
    }
    //printf("Parse node: this node is new\n");
    return -1;
}

int is_me(struct Node  * new_node){
    return new_node->addr.sin_addr.s_addr == this_host.sin_addr.s_addr && new_node->addr.sin_port == this_host.sin_port;
}    

void add_node(char * node_info){
    char delim[] = ":";
    char *ptr = strtok(node_info, delim);
    char tmp[BUFFER];
    strcpy(tmp, node_info);
    char name[HOST_NAME], ip[IP], port[PORT];
    if(ptr == NULL){
        //printf("Parse node: incorrect format: %s\n", tmp);
        return;
    }
    strcpy(name, ptr);
    ptr = strtok(NULL, delim);
    if(ptr == NULL){
        //printf("Parse node: incorrect format: %s\n", tmp);
        return;
    }
    strcpy(ip, ptr);
    ptr = strtok(NULL, delim);
    if(ptr == NULL){
        //printf("Parse node: incorrect format: %s\n", tmp);
        return;
    }
    
    strcpy(port, ptr);
    ptr = strtok(NULL, delim);
    if (ptr != NULL){
        //printf("Parse node: incorrect format: smth excess\n");
        return;
    }
    struct Node * new_node = (struct Node *) malloc(sizeof(struct Node));
    strcpy(new_node->name, name);
    new_node->addr.sin_family = AF_INET;
    new_node->addr.sin_addr.s_addr = inet_addr(ip);
    new_node->addr.sin_port = htons(strtoul(port, NULL, 10));
    new_node->numb_of_files = 0;
    if (is_me(new_node)){
        //printf("I got myself\n");
        return;
    }
    int index = node_ind(new_node);
    if (index  == -1){ 
        new_node->numb_of_files = 0;
        nodes[numb_of_nodes] = new_node;
        numb_of_nodes ++;
        return;
    }   
    nodes[index] = new_node;
}

void add_client(char * client_info){
    char delim[] = ":";

    char tmp[BUFFER];
    strcpy(tmp, client_info);


    char *ptr = strtok(client_info, delim);
    char name[HOST_NAME], ip[IP], port[PORT];
    if(ptr == NULL){
        //printf("Parse node: incorrect format: %s\n", tmp ) ;
        return;
    }
    strcpy(name, ptr);
    //printf("Client name: %s\n", name);
    ptr = strtok(NULL, delim);
    if(ptr == NULL){
        //printf("Parse node: incorrect format: %s\n", tmp);
        return;
    }
    strcpy(ip, ptr);
    //printf("Client ip: %s\n", ip);
    ptr = strtok(NULL, delim);
    if(ptr == NULL){
        //printf("Parse node: no_files: %s\n",tmp);
        return;
    }
    strcpy(port, ptr);
    //printf("Client port: %s\n", port);
    char node_info[BUFFER];
    ptr = strtok(NULL, delim);
    int no_files = 0;
    if (ptr == NULL){
        no_files = 1;
        //printf("No_files:\n");
    }
    else{
        //printf("Files :%s\n", ptr);
    }
    struct Node * new_node = (struct Node *) malloc(sizeof(struct Node));
    strcpy(new_node->name, name);
    new_node->addr.sin_family = AF_INET;
    new_node->addr.sin_addr.s_addr = inet_addr(ip);
    new_node->addr.sin_port = htons(strtoul(port, NULL, 10));
    char new_delim[] = ",";
    new_node->numb_of_files = 0;
    
    char str[BUFFER];
    if(! no_files){
        strcpy(str, ptr);
        char * new_ptr = strtok(str, new_delim);
        //printf("Flag: %s\n", new_ptr);
        while (new_ptr != NULL){
            strcpy(new_node->files[new_node->numb_of_files], new_ptr);
            new_node->numb_of_files++;
            new_ptr = strtok(NULL, new_delim);
        }
    }
    int index = node_ind(new_node);
    if (index == -1){ //no such node
        //new_node->numb_of_files = 0;
        nodes[numb_of_nodes] = new_node;
        numb_of_nodes ++;
        return;
    }
    nodes[index] = new_node;
}

void sync_s(int sockfd){
    char client_info[BUFFER];
    if(recv(sockfd, client_info, sizeof(client_info), 0) == -1){
        //printf("Sync_s recv client info: failed\n");
    }
    else{
        //printf("Node info: %s \n", client_info);
    }
    usleep(100);
    add_client(client_info);
    int ind;
    if(recv(sockfd, &ind, sizeof(int), 0) == -1){
        //printf("Sync_s recv client info: failed\n");
    }
    else{
        //printf("Nodes number is: %d\n", ind);
    }
    //printf("Number recieved: %d\n", ind);
    char node_info[BUFFER];
    for (int i = 0; i < ind; i++){
        usleep(100);
        recv(sockfd, node_info, sizeof(node_info), 0);
        add_node(node_info);
    }
}

int have_file(char * file_name){
    for (int i = 0; i < numb_of_files; i++){
       if (strcmp(my_files[i], file_name) == 0){
           return 1;
       }
    }
    return 0;
}

int get_numb_of_words(char * file_name){
    FILE *fp = fopen(file_name, "r");
    if (fp == NULL){
        printf("There is no file: %s\n", file_name);
        return -1;
    }
    int words = 0;
    char buffer[15];
    while(fscanf(fp, "%s", buffer) != EOF){
       words++;
    } 
    return words;
}

void recv_file_s(int sockfd){
    char file_name[FILE_NAME];
    recv(sockfd, file_name, sizeof(file_name), 0);
    if(!have_file(file_name)){
        printf("Have no file, naimed: %s\n", file_name);
        close(sockfd);
        return;
    }
    int numb_of_words = get_numb_of_words(file_name);
    if (send(sockfd, &numb_of_words, sizeof(int), 0) == -1){
        printf("Sending numb_of_words: failed\n");
        close(sockfd);
        return;
    }
    else{
        printf("Send numb of words: %d", numb_of_words);
    }
    char word[WORD];
    for (int i = 0; i < numb_of_words; i++){
        if (send(sockfd, word, BUFFER, 0) == -1){
            printf("Word %s sending: failed\n", word);
            close(sockfd);
            return;
        }
    }
}

void * clientThread(void * _sockfd){
    int sockfd = *((int *)_sockfd);
    printf ("%d",sockfd);
    int command;
    if(recv(sockfd, &command, sizeof(command), 0) == -1){
        //printf("Recv command from client:fail\n");
        close(sockfd);
    }
    else{ 
        //printf("Recv command from client: success\n");  
    }
    if (command == 1){
        sync_s(sockfd);
    }
    else if(command == 0){
        recv_file_s(sockfd);
    }
    close(sockfd);
    /*
    printf("From client: %s\n", buffer);
    char reply[] = "ok";
    send(sockfd, reply, sizeof(reply), 0);
    close(sockfd);*/
    pthread_t thread = pthread_self();
    for (int i = 0; i < THREADS; i++){
        if (thread == tid[i]){
            tid_is_free[i] = 1;
            close(thread);
        }
    }
    return NULL;

}

void * server(void * i){
    //printf("you are in server\n");
    int sockfd, connfd;
    struct sockaddr_in server, client;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        //printf("Socket creation: failed\n");
        exit(0);
    }
    else{
        //printf("Socket creation: success\n");
    }
    //server = (struct sockaddr_in) malloc(sizeof(struct * sockaddr_in)); 
    /*server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_port = htons(SERVER_PORT);
    */
    if (bind(sockfd, (struct sockaddr *) &this_host, sizeof(this_host)) != 0){
        //printf("Bind: fail\n");
        exit(0);
    }
    else{
        //printf("Bind: success\n");
    }

    pthread_t tid[20];
    int ind = 0;
    while(1){
        if (listen(sockfd, 3) != 0){
            //printf("Listen: failed\n");
            exit(0);
        }
        else{
            //printf("Listen: success\n");
        }
        socklen_t len;   
        connfd = accept(sockfd, (struct sockaddr *) &client, &len);
        if (connfd < 0){
            //printf("Accept: failed\n");
        }
        else{
            //printf("Accept: success\n");
        }
        if (pthread_create(&tid[ind], NULL, clientThread, &connfd) != 0){
            //printf("Thread creation: fail\n");
        }
        //join
        ind ++;   
    }
    close(sockfd);
}

void set_my_files(){
    const char * pattern = "./*.txt";     
    glob_t pglob;
    glob(pattern, GLOB_ERR, NULL, &pglob);
    char file_name[FILE_NAME];
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
    set_my_files();
    return 0;
}

void get_my_info(char * info){ 
    sprintf(info, "%s:%s:%u:", this_name, inet_ntoa(this_host.sin_addr), ntohs(this_host.sin_port));  
    for(int i = 0; i < numb_of_files; i++){
        strcat(info, my_files[i]);
        if(i != numb_of_files - 1){
            strcat(info, ",");
        }
    }
    //printf("node info: %s\n", info);
}

void get_info(char * info, struct Node * node){
    sprintf(info, "%s:%s:%u:", node->name, inet_ntoa(node->addr.sin_addr), ntohs(node->addr.sin_port));
}

int ping(struct sockaddr_in server_to_call){
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
    if (send(connfd, &message, BUFFER, 0) == -1){
        //printf("Send 1: fail\n");
        close(connfd);
        return 1;
    }
    else{
        //printf("Send 1: success\n");
    }
    char my_info[BUFFER];
    get_my_info(my_info);
    //printf("%s\n\n\n", my_info);
    sleep(1);
    if (send(connfd, my_info, sizeof(my_info), 0) == -1){
        //printf("Send node info: fail\n");
        close(connfd);
        return 1;
    }
    else{
        //printf("Send node info: success\n");
    }
    sleep(1);
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
        if (nodes[i]->addr.sin_addr.s_addr == server_to_call.sin_addr.s_addr &&
                nodes[i]->addr.sin_port == server_to_call.sin_port){
            continue;
        }
        get_info(node_info, nodes[i]);
        sleep(1);   
        if (send(connfd, node_info, BUFFER, 0) == -1){
            //printf("Node %d info sending: fail\n", i);
            close(connfd);
            return 1;
        }
        else{
            //printf("Node %d info sending: success\n", i);
        }
    }
    close(connfd);
    return 0;
}

int connect_to_network(){
    return ping(root);
}

int request_from(struct sockaddr_in server_to_call,char *file_name){
    int connfd;
    connfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connfd < 0){
        //printf("Request: sock creation: fail\n");
        close(connfd);
        return 0;
    }
    else{
        //printf("Request: sock creation: success\n");
    }
    if (connect(connfd, (struct sockaddr *)&server_to_call, sizeof(server_to_call)) != 0){
        //printf("Request: connect: fail\n");
        close(connfd);
        return 0;
    }
    else {
        //printf("Ping: connect: success\n");
    }
    int message = 0;
    if (send(connfd, &message, sizeof(int), 0) == -1){
        //printf("Request: send command: failed\n");
        close(connfd);
        return 0;
    }
    else{
        //printf("Request: send command: success\n");
    }
    sleep(1);
    if (send(connfd, file_name, sizeof(file_name), 0) == -1){
        printf("Request: send file name %s:faile\n", file_name);
        close(connfd);
        return 0;
    }
    else{
        printf("Request: send file name %s: success\n", file_name);
    }
    int numb_of_words = 0;
    if (recv(connfd, &numb_of_words, sizeof(numb_of_words), 0) == -1){
        printf("Request: recv numb of words: failed\n");
        close(connfd);
        return 0;
    }
    else{
        printf("Request: recv numb of words: %d\n", numb_of_words);
    }
    usleep(100);
    char word[WORD];
    FILE *fp = NULL;
    fp = fopen(file_name, "a");
    for (int i  = 0; i < numb_of_words; i++){
        recv(connfd,(char *) word, BUFFER, 0);
        fprintf(fp, word);
        fprintf(fp, " "); 
        printf("%s\n", word);
        usleep(100);
    } 
    fclose(fp);
    return 1;
} 

int may_have_file(struct Node *node, char * file_name){
    for (int i = 0; i < node->numb_of_files; i++){
       if (strcmp(node->files[i], file_name) == 0){
           return 1;
       }
    }
    return 0;
}
void request(char * file_name){
    if (have_file(file_name)){
        printf("There is file %s already\n", file_name);
        return;
    }
    for (int i = 0; i < numb_of_nodes; i++){
        if(may_have_file(nodes[i], file_name)){
            if (request_from(nodes[i]->addr, file_name) == 1){
                strcpy(my_files[numb_of_files], file_name);
                numb_of_files++;
                printf("Transfering file %s from host %s\n", file_name, nodes[i]->name);
                return;
            }
        } 
    }
    printf("There is no file %s\n", file_name);
}
    
void client(){
    char command[BUFFER];
    while (1){
        printf("Enter the command:\n");
        scanf("%s", command);
        if (strcmp(command, "exit") == 0){
            exit(0);
        }
        if (strcmp(command, "0") == 0){
            printf("Please enter the file name\n");
            char file_name[FILE_NAME];
            scanf("%s", file_name);
            request(file_name);
        }
        else{
            printf("Incorrect command: %s\n", command);
        }
    }
}

int is_root(){
    return ((root.sin_addr.s_addr == this_host.sin_addr.s_addr) && (root.sin_port == this_host.sin_port));
}

void * sync_network(void * args){
    while(1){
        sleep(3);
        for (int i = 0; i < numb_of_nodes; i++){
            ping(nodes[i]->addr);
        }
    }
}

int main(int argc, char **argv){
    if (argc != 4){
        printf("Incorrect number of argumets\n");
        exit(0);
    }
    strcpy(root_name, argv[1]);
    root.sin_family = AF_INET;
    root.sin_addr.s_addr = inet_addr(argv[2]);
    root.sin_port = htons(strtoul(argv[3], (char **)NULL, 10));//not sure
    init();
    
    if (!is_root()){
        if (connect_to_network() != 0){
            printf("Can't connect to the network\n");
            exit(0);
        }
        else{
            printf("Not root. Connected to network\n");
        }
        struct Node * root_node = (struct Node *) malloc(sizeof(struct Node *));
        strcpy(root_node->name, root_name);
        root_node->addr = root;
        numb_of_nodes = 1;
        nodes[numb_of_nodes - 1] = root_node;
    }
    else {
        numb_of_nodes = 0;
        printf("Yeah, I am a root\n");
    }

    pthread_t server_thread;
    pthread_t ping_thread;
    if (pthread_create(&server_thread, NULL, server, NULL) != 0){
        //printf("server_thread creation: fail\n");
    }
    else{
        //printf("server thread creation: success\n");
    }
    if (pthread_create(&ping_thread, NULL, sync_network, NULL) != 0){
        //printf("Synchronization thread creation: fail\n");
    }
    else{
        //printf("Synchronization thread creation: success\n");
    }
    client();
    close(ping_thread);
    close(server_thread);
}
