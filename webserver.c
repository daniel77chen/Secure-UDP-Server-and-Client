// NAME: Daniel Chen,    Winston Lau
// EMAIL: kim77chi@gmail.com, winstonlau99@gmail.com
// ID: 605006027, 504934155

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h> 
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h> 
#include <strings.h>
#include <ctype.h> 
#include <dirent.h> 
#include <time.h> 
#define BUF_SIZE 1024
#define MYPORT 5861 
#define BACKLOG 5
#define MAX_GET_LENGTH 8192
#define MAX_FILEPATH_LENGTH 2048
#define VERSION_LENGTH 9
extern int errno; 

int exit_code;
int socket_fd; 
int new_fd;
socklen_t sin_size;
struct sockaddr_in my_addr;
struct sockaddr_in their_addr;
char tmp_buf[BUF_SIZE];  //just for holding contents from the socket
char msg_buf[MAX_GET_LENGTH];
char method[4];
char url[MAX_FILEPATH_LENGTH];
//char path[MAX_FILEPATH_LENGTH];
char version[VERSION_LENGTH];
char extension[5]; 
char content_type[15];
struct map * header_list;
struct data * data_list; 

int gatherInfo(void);

void pError (char * message, int exit_code) {
    fprintf(stderr, "%s\n", message);
    exit(exit_code);
}
void bzero_data() {
    bzero(tmp_buf, BUF_SIZE);
    bzero(msg_buf, MAX_GET_LENGTH);
    //bzero(path, MAX_FILEPATH_LENGTH);
    bzero(method, 3);
    bzero(url, MAX_FILEPATH_LENGTH);
    bzero(version,9);
    bzero(extension, 5);
}
void serverError(char * message) {
    fprintf(stderr, "%s\n", message);
    if(close(new_fd)== -1)
        pError("error with closing accepted fd", 1);
    new_fd = -1;
}

void create_socket_and_listen() {  //socket, bind, listen
    if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        pError("Could not create socket", 1);
    }
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYPORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    memset(my_addr.sin_zero,'\0',sizeof(my_addr.sin_zero));

    if(bind(socket_fd,(struct sockaddr*) &my_addr, sizeof(struct sockaddr)) == -1) {
        pError("Could not bind to socket", 1);
    }
    if (listen(socket_fd, BACKLOG) == -1) {
        pError("Could not set socket to listen for others", 1);
    }
}

int check_valid_file() {
    DIR *current; 
    struct dirent *entry; 
    current = opendir("."); 
    int is_valid = 0; 
    errno = 0; 
    if (current) {
        while((entry = readdir(current)) != NULL) {
            if (strcasecmp(url, entry->d_name) == 0) {
                is_valid = 1; 
                break;
            }
        }
        if (errno != 0) {
            pError("Error while reading from Directory!", 1); 
        }
        closedir(current); 
    } else {
        pError("Could not open current Directory!", 1);
    }
    return is_valid; 
}

int respond() {
    if (check_valid_file()) { // checks to see if url is a valid file
        FILE* file = fopen(url, "r"); 
        if (file) {
            struct stat file_info;
            if( stat(url, &file_info) != 0) {
                close(new_fd);
                pError("Could not obtain info on the file", 1);
            }
            time_t seconds = time(NULL);
            struct tm * current_time = gmtime(&seconds);
            char date[256];
            strftime(date, sizeof(date), "%a, %d %b %Y %T %Z", current_time);
            char headers [MAX_GET_LENGTH];
            bzero(headers, MAX_GET_LENGTH);
            sprintf(headers + strlen(headers), "%s 200 OK\n", version);
            sprintf(headers + strlen(headers), "Connection: close\n");
            sprintf(headers + strlen(headers), "Content-Type: %s\n", content_type);
            sprintf(headers + strlen(headers), "Date: %s\n", date);
            current_time = gmtime(&(file_info.st_mtime));
            strftime(date, sizeof(date), "%a, %d %b %Y %T %Z", current_time);
            sprintf(headers + strlen(headers), "Last-Modified: %s\n", date);
            sprintf(headers + strlen(headers), "Content-Length: %li\n\n", (long) file_info.st_size);
            write(new_fd, headers, strlen(headers));
            int bytes_read = 0;
            bzero(tmp_buf, BUF_SIZE);
            while(1) {
                bytes_read = fread(tmp_buf, sizeof(char), BUF_SIZE, file);
                if (bytes_read == 0)
                    break;
                if(bytes_read == -1) {
                    close(new_fd);
                    pError("error reading from found file", 1); 
                }
                write(new_fd, tmp_buf, bytes_read *sizeof(char));
                bzero(tmp_buf, BUF_SIZE);
            }
        } else {
            close(new_fd);
            pError("Could not open the found file", 1); 
        }
    } else {
        // 404 Error! 
        time_t seconds = time(NULL);
        struct tm * current_time = gmtime(&seconds);
        char date[256];
        strftime(date, sizeof(date), "%a, %d %b %Y %T %Z", current_time);
        char headers [MAX_GET_LENGTH];
        bzero(headers, MAX_GET_LENGTH);
        sprintf(headers + strlen(headers), "%s 404 NOT FOUND\n", version);
        sprintf(headers + strlen(headers), "Connection: close\n");
        sprintf(headers + strlen(headers), "Date: %s\n\n", date);
        write(new_fd, headers, strlen(headers));
        char error_message[50] = "<h1>404 Error: The page could not be found</h1>";
        write(new_fd, error_message, strlen(error_message));
    }
    return 0;
}

int main (/*int argc, char ** argv*/) {
    //path[0] = '.';
    bzero_data();
    create_socket_and_listen();
    sin_size = sizeof(struct sockaddr_in);
    while(1) {
        if((new_fd = accept(socket_fd, (struct sockaddr *) &their_addr, &sin_size)) == -1) {
            serverError("Could not accept the other socket");
            close(new_fd);
            continue;
        }
        printf("Connection made to: %s\n", inet_ntoa(their_addr.sin_addr));
        if(read(new_fd, msg_buf, MAX_GET_LENGTH) < 0) {
            close(new_fd);
            pError("Error while reading", 1);
        }
        printf("%s\n", msg_buf);       
        if (gatherInfo() == -1) {
            close(new_fd);
            continue;
        }
        respond();
        close(new_fd);
    }
}

int gatherInfo() {  //return -1 if serverError
    ////// REQUEST LINE
    //int msg_size = strlen(msg_buf);
    int i, file_size = 0; 

    //// METHOD
    strncpy(method, msg_buf, 3);
    method[3] = '\0';
    if(strcmp(method, "GET") != 0 || msg_buf[3] != ' ') {
        serverError("can only accept GET requests");
        return -1;
    }

    //// FILENAME       //copy the filepath byte by byte 
    for (i = 5; i < MAX_FILEPATH_LENGTH; i++, file_size++) {    
          
        if(!strncmp(&msg_buf[i], "%20", 3)) {    //convert space representation in filepath to literal string space
            strncat(url, " ", 1);
            i += 2; //skip to the 0 of %20 so that next interation will be the following char
            continue;   //not sure if + should be a space since some URL's do that 
        }
        printf("%c", msg_buf[i]);
        strncat(url, &msg_buf[i], 1);
    }
    url[file_size] = '\0';
    //strcat(path,url);
    
    //// VERSION 
    if(strncmp(&msg_buf[i], "HTTP/1.0", 8) == 0) 
        strcpy(version, "HTTP/1.0");
    else if(strncmp(&msg_buf[i], "HTTP/1.1", 8) == 0) 
        strcpy(version, "HTTP/1.1");
    else {
        serverError("Incorrect HTTP version");
        return -1;
    }

    ////EXTENSION
    int reached_period = 0, extension_length = 0;
    int path_length = strlen(url); 
    for(i=0; i < path_length; i++) {
        if(url[i] == '.') {
            if(reached_period) {
                bzero(extension, 5);
                extension_length = 0;                
            }
            reached_period = 1;
            continue;
        }
        if(reached_period)
            extension[extension_length++] = url[i];
    }
    extension[extension_length] = '\0';

    if(strcasecmp(extension, "jpeg") && strcasecmp(extension, "jpg") && 
        strcasecmp(extension, "html") && strcasecmp(extension, "htm") &&
        strcasecmp(extension, "gif") && strcasecmp(extension, "png") &&
        strcasecmp(extension, "txt")) {
            serverError("URL has invalid extension");
            return -1;
    }
    if(strcmp(extension, "html") == 0 || strcmp(extension, "htm") == 0) {
        strcpy(content_type, "text/html");
    } else if (strcmp(extension, "jpeg") == 0 || strcmp(extension, "jpg") == 0) {
        strcpy(content_type, "image/jpeg");
    } else if (strcmp(extension, "gif") == 0) {
        strcpy(content_type, "image/gif");
    } else if (strcmp(extension, "png") == 0) {
        strcpy(content_type, "image/png");
    } else if (strcmp(extension, "txt") == 0) {
        strcpy(content_type, "text/plain");
    }
    return 0;
} 


