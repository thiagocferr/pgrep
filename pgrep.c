#define _GNU_SOURCE 

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <pthread.h>
#include <dirent.h>
#include <regex.h>

pthread_mutex_t print_mutex;
pthread_mutex_t running_threads_mutex;

pthread_cond_t running_threads_cond;

int max_thread_num; // Constant during program
int running_threads = 1; // To indicate how many call to the file_grep func are running
                         // (Starts with 1 representing the inital thread that runs main())
regex_t g_pattern; // REGEX pattern to search

// Find occurences of the global pattern in a file
void *file_grep(void *thread_arg)
{    
    char *path = (char *) thread_arg;
    
    // Variable inicialization
    FILE *file = fopen(path, "r");
    int total_line_num = 0;
    ssize_t line_lenght = 0;
    int c;
    
    // Check number of lines in FILE
    while ((c = fgetc(file)) != -1)
    {
        if (c == '\n')
            total_line_num++;
    }
    
    // Array representing lines where the patter occurs
    int regex_lines[total_line_num];
    rewind(file);

    int curr_line = 0;
    int i = 0;
    
    // Searching for patterns in each line and storing lines that have pattern
    while (line_lenght != -1)
    {       
        char *line;
        size_t n = 0;

        line_lenght = getline(&line, &n, file);

        int regex_exec = regexec(&g_pattern, line, 0, 0, 0);        
        free(line);
         
        
        // We just need to know if line has a match
        if (regex_exec != 0)
        {
            curr_line++;
            continue; 
        }
                   
                
        regex_lines[i] = curr_line;
        curr_line++;
        i++;         
    }
    
    // Printing in order
    pthread_mutex_lock(&print_mutex);      
    for (int j = 0; j < i; j++)
        printf("%s: %d\n", path, regex_lines[j]);    
    pthread_mutex_unlock(&print_mutex);  
    
    pthread_mutex_lock(&running_threads_mutex);   
    
    running_threads--;
    // Maybe put a if test here? (to check if number of thread is greater than 1 or less than maximum)
    pthread_cond_signal(&running_threads_cond);
    
    pthread_mutex_unlock(&running_threads_mutex);
    
    fclose(file);
    free(path);
    pthread_exit(NULL);
    
}

// Store in new_path the concatenation of a string old_path with a '/' and an append, given its size
void concat_path (char *new_path, char const *old_path, char *append, int append_real_size)
{
    
    char shorter_append[append_real_size + 1];

    strncpy(shorter_append, append, append_real_size);
    shorter_append[append_real_size] = '\0';
    
    strcpy(new_path, old_path);
    strcat(new_path, "/");
    strcat(new_path, shorter_append);
    
}

// Search recursevily into a folder and finds archives to analise
int rec_search (char const *dir_name)
{    
    DIR *direc = opendir(dir_name);
    
    if (!direc)
    {
        printf("Error: couldn't open directory %s\n", dir_name);
        closedir(direc);
        return 0;
    }
        
    struct dirent *curr_file;
    
    // Until end of folder is reached
    while (curr_file = readdir(direc))
    {        
        // Ignore the '.' and '..' folders
        if (strcmp(curr_file->d_name, ".") == 0 || strcmp(curr_file->d_name, "..") == 0)
            continue;

        struct stat buf;
        
        // Formats path string to current directory
        int i = 0;
        while (curr_file->d_name[i] != '\0')
            i++;            
        char format_file_name[strlen(dir_name) + i + 5];         
        concat_path(format_file_name, dir_name, curr_file->d_name, i);
        
        if (stat(format_file_name, &buf) == -1)
        {
            printf("Error: Couldn't check statistics of file %s\n", curr_file->d_name);
            continue;
        }        
        
        // Checks if file is a regular one
        if (S_ISREG(buf.st_mode))
        {            
            // Need to dinamically allocate path string to the file_grep function. It is 
            // then responsible form dealocating it when finished
            char *allocated_file_path = malloc(strlen(format_file_name) + 2);
            strcpy(allocated_file_path, format_file_name);
            
            pthread_t file_thread;            

            // Var running_threads is added when thread is created and subtracted when is destroyed
            // Since the recursive search is linear, when the main thread return to the main() function,
            // we need that all threads end before we proceed to eliminate glocal mutex and conditional
            // variables
            pthread_mutex_lock(&running_threads_mutex);
                
            // Waiting for thread number to be below the maximum (as in the program argumment)
            while (running_threads >= max_thread_num)
                pthread_cond_wait(&running_threads_cond, &running_threads_mutex);     
            
            int r = pthread_create(&file_thread, NULL, file_grep, (void *) allocated_file_path);
            if (!r)
                running_threads++;    
            
            pthread_mutex_unlock(&running_threads_mutex);
        }
        else if (S_ISDIR(buf.st_mode))
            rec_search(format_file_name);

        else
        {
            printf("Error: File %s\n is not a directory nor a regular file", curr_file->d_name);
            continue; 
        }
    }
    
    closedir(direc);
    return 1;
}

int main(int argc, char const *argv[])
{
    if (argc < 4)
    {
        printf("Error: insufficient number of arguments\n");
        return 0;
    }
    
    int max_thread_arg = atoi(argv[1]);    
    if (max_thread_arg < 2)
    {
        printf("Error: minimum number of threads is 2\n");
        return 0;
    }
    const char *string_pattern = argv[2];
    char const *path = argv[3];
    
    int pat_res = regcomp(&g_pattern, string_pattern, 0);
    if (pat_res != 0)
    {
        printf("Error: couldn't recognize patter %s\n", string_pattern);
        return 0;
    }
    
    max_thread_num = max_thread_arg;

    // Inicializing pthreads variables
    pthread_mutex_init(&print_mutex, NULL);
    pthread_mutex_init(&running_threads_mutex, NULL);
    pthread_cond_init(&running_threads_cond, NULL);
    
    int search_val = rec_search(path);

    // Preventing pthread variables to be destoyed before termination of all adjacent threads
    pthread_mutex_lock(&running_threads_mutex);
    while (running_threads > 1)
        pthread_cond_wait(&running_threads_cond, &running_threads_mutex);            
    pthread_mutex_unlock(&running_threads_mutex);
        
    // Destroying pthreads variables (after no more file threads are executing)
    pthread_mutex_destroy(&print_mutex);
    pthread_mutex_destroy(&running_threads_mutex);   
    pthread_cond_destroy(&running_threads_cond);

    running_threads--;
    regfree(&g_pattern);
    
    return 1;
}
