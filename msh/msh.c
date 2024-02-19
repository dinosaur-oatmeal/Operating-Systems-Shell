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

// added to support redirection from popen.c
#include <fcntl.h>

// test file arguments with: ./msh test-msh.sh

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 32

// command function declaration
void command(char *token[], char error_message[], char *head_ptr, int token_count, FILE *myFile);

int main(int argc, char *argv[])
{

  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );

  // error message and written message
  char error_message[30] = "An error has occurred\n";
  // write(STDERR_FILENO, error_message, strlen(error_message));

  //printf("Hello");

  // create file pointer
  FILE *myFile;

  // open file if there's an argument for it
  if(argc > 1)
  {
    // can't have more than one input file
    if(argv[2] != NULL)
    {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(EXIT_FAILURE);
    }

    // open file to be read from
    myFile = fopen(argv[1], "r");

    // file can't be opened
    if(myFile == NULL)
    {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(EXIT_FAILURE);
    }
  }

  while( 1 )
  {
    // interactive mode
    if(myFile == NULL)
    {
      // Print out the msh prompt
      printf ("msh> ");

      // Read the command from the command line.  The
      // maximum command that will be read is MAX_COMMAND_SIZE
      // This while command will wait here until the user
      // inputs something.
      while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );
    }

    // batch mode
    else
    {
        size_t count = 0;
        
        // read next line of file
        if(!feof(myFile))
        {
          getline(&command_string, &count, myFile);
        }
    }

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

    command(token, error_message, head_ptr, token_count, myFile);
  }

  // close file pointer if applicable
  if(myFile != NULL)
  {
    fclose(myFile);
  }

  return 0;
}

void command(char *token[], char error_message[], char *head_ptr, int token_count, FILE *myFile)
{
  // handle whitespace by looping through input
  int count_token = 0;
  int count_null = 0;
  for(int count_input = 0; count_input < token_count; count_input++)
  {
    if(token[count_input] == NULL)
    {
      // look for entire commands of whitespace and return to main
      count_null++;
      if(myFile != NULL)
      {
        // batch mode exit to main
        if(count_null == token_count && !feof(myFile))
        {
          return;
        }
      }

      // interactive mode exit to main
      else
      {
        if(count_null == token_count)
        {
          return;
        }
      }
    }

    // move non-NULL parameters to the front of token[] and NULL parameters to the end
    if(token[count_input] != NULL)
    {
      token[count_token] = token[count_input];

      // only overwrite data if values are different
      if(count_input != count_token)
      {
        token[count_input] = NULL;
      }

      count_token++;
    }
  }

  // batch mode exit call at end of file
  if(token[0] == NULL)
  {
    exit(0);
  }

  // compare first token to exit
  if(strcmp(token[0], "exit") == 0)
  {
    // exiting
    if(token[1] == NULL)
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

    // cd does not exist
    else if(chdir(token[1]) != 0)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }

    // execute command
    else
    {
      chdir(token[1]);
    }
  }

  // all processes that require fork()
  else
  {
    // make child's pid and status
    pid_t child_pid = fork();
    int status;

    // child pid error
    if(child_pid == -1)
    {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(EXIT_FAILURE);
    }

    // execute child pid's process
    else if(child_pid == 0)
    {

      // look for redirection
      // command stored in token[]
      for(int i = 1; i < token_count; i++)
      {
        // break at the end of the command
        if(token[i] == NULL)
        {
          break;
        }

        // find ">" in token
        if(strcmp(token[i], ">") == 0)
        {
          // error if multiple file inputs after ">"
          for(int j = i + 2; j < token_count; j++)
          {
            if(token[j] != NULL)
            {
              write(STDERR_FILENO, error_message, strlen(error_message));
              break;
            }
          }

          //error if ">" is first char in input
          if(i == 0)
          {
            write(STDERR_FILENO, error_message, strlen(error_message));
            break;
          }

          int fd = open(token[i + 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

          // error opening file
          if(fd < 0)
          {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(0);
          }

          dup2(fd, 1);
          close(fd);

          // trim off ">" and [filename] in command and break
          token[i] = NULL;
          token[i + 1] = NULL;
          break;
        }
      }

      // see if command exists in directories
      if(execvp(token[0], &token[0]) == -1)
      {
        write(STDERR_FILENO, error_message, strlen(error_message));
      }

      else
      {
        // executes process
        // (execvp automatically searches required directories)
        execvp(token[0], &token[0]);
      }

      // exit child process
      fflush(NULL);
      exit(EXIT_SUCCESS);
    }

    // wait for process
    else if(child_pid > 0)
    {
      wait(NULL);
    }

    // fork failed
    else
    {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(0);
    }

    // wait for child process to finish
    waitpid(child_pid, &status, 0);
  }

  // remove next 5 lines once everything works as intended
  // used for debugging
  /*int token_index  = 0;
  for( token_index = 0; token_index < token_count; token_index ++ ) 
  {
    printf("token[%d] = %s\n", token_index, token[token_index] );  
  }*/

  free( head_ptr );
}