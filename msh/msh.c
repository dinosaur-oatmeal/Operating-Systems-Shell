// Name: Will Maberry
//
// The MIT License (MIT)
// 
// Copyright (c) 2024 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <dirent.h>

#include <fcntl.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 32

int main(int argc, char * argv[])
{

  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandi line.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something.
    while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *argument_pointer;                                         
                                                           
    char *working_string  = strdup( command_string );                

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    
    char *head_ptr = working_string;
    
    // Tokenize the input with whitespace used as the delimiter
    while ( ( (argument_pointer = strsep(&working_string, WHITESPACE ) ) != NULL) &&
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_pointer, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your shell functionality

    // handle whitespace by looping through input
    if(token[1] == NULL)
    {
      int count_token = 1;
      for(int count_input = 1; count_input < token_count; count_input++)
      {
        //move non-NULL parameters to front of token[]
        if(token[count_input] != NULL)
        {
          token[count_token] = token[count_input];
          token[count_input] = NULL;
          count_token++;
        }
      }
    }

    char error_message[30] = "An error has occurred\n";
    //write(STDERR_FILENO, error_message, strlen(error_message));

    // compare first token to exit
    if(strcmp(token[0], "exit") == 0)
    {
      //printf("Success!");

      // error exiting
      if(token[1] != NULL)
      {
        write(STDERR_FILENO, error_message, strlen(error_message));
      }

      // exit
      else
      {
        exit(0);
      }
    }

    // cd
    if(strcmp(token[0], "cd") == 0)
    {
        // incorrect input for cd
        if(token[1] == NULL || token[2] != NULL)
        {
          write(STDERR_FILENO, error_message, strlen(error_message));
        }

        // call chdir
        else
        {
          // cd does not exist
          if(chdir(token[1]) != 0)
          {
            write(STDERR_FILENO, error_message, strlen(error_message));
          }

          // execute command
          else
          {
              chdir(token[1]);
          }
        }
    }

    // all processes that require fork()
    else
    {
      //look for redirection
        for(int i = 1; i < argc; i++)
        {
          if(strcmp(token[i], ">") == 0)
          {
            int fd = open(token[i + 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

            if(fd < 0)
            {
              write(STDERR_FILENO, error_message, strlen(error_message));
              exit(0);
            }

            dup2(fd, 1);
            close(fd);

            //trim off '>' and [filename] in command
            token[i] = NULL;
            token[i + 1] = NULL;
          }
        }
      // make child's pid and status
      pid_t child_pid = fork();
      int status;

      // child pid error
      if( child_pid == -1)
      {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(EXIT_FAILURE);
      }

      // execute child pid's process
      if(child_pid == 0)
      {

        

        // executes process and automatically searches correct directories
        execvp(token[0], &token[0]);

        fflush(NULL);
        exit(EXIT_SUCCESS);
      }

      // wait for child process to finish
      waitpid(child_pid, &status, 0);

      // child terminated by signal
      if(WIFSIGNALED(status))
      {
        write(STDERR_FILENO, error_message, strlen(error_message));
      }
    }

    // remove next 5 lines at the end
    int token_index  = 0;
    for( token_index = 0; token_index < token_count; token_index ++ ) 
    {
      printf("token[%d] = %s\n", token_index, token[token_index] );  
    }

    free( head_ptr );
  }

  return 0; 
}