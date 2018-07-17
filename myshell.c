// Brandon Piper
//
// myshell.c  
//    Basic shell that handles standard UNIX commands
//    file redirection, and piping.  

#include <stdio.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/stat.h>
#include <errno.h> 
#include <fcntl.h>

extern char **getline(); 

/*____________________________________________________________________
  This routine handles the file re-direction for <.
  It feeds the file on the right as input into the
  command on the left.
*/
void
in_cmd( char** args ){
  int pid, i;
  char* filename;
  int pos;
  
  for( pos = 0; strcmp(args[pos], "<") != 0  ; pos++ ){}
  args[pos] = args[pos + 1];
  args[pos + 1] = NULL;
  
  pid = fork();
  if( pid < 0 ){
    fprintf( stderr, "error: fork failed\n");
    exit(1);
  }
  else if( pid > 0 ){
    wait( 0 );
  }
  else{
    int fd;
    
    // open the file to read from.
    fd = open( filename, O_CREAT|O_WRONLY );
    
    close( 0 );   // close stdin.
    dup( fd );    // replace stin with the file created.
    close( fd );  // remove original fd from table.
    
    if( execvp( args[0], args ) == -1 )
      fprintf( stderr, "error: execvp failed\n" );
  }
}

/*____________________________________________________________________
  This routine handles the file re-direction for >.
  The output from the left command is outputted to
  the file on the right.
*/
void
out_cmd( char** args ){
  int pid, i;
  char* filename;
  char* flags[8];
  int pos;
  
  for( pos = 0; strcmp(args[pos], ">") != 0  ; pos++ ){}
  args[pos ] = NULL;
  filename = args[ pos + 1 ]; 
  
  pid = fork();
  if( pid < 0 ){
    fprintf( stderr, "error: fork failed\n");
    exit(1);
  }
  else if( pid > 0 ){
    wait( 0 );
  }
  else{
    int fd;
    
    // create and open the file to write to.
    fd = open( filename, O_CREAT|O_WRONLY, S_IRWXU );

    close( 1 );   // close stdout.
    dup( fd );    // replace stdout with fd.
    close( fd );  // remove original fd from table.
    
    if( execvp( args[0], args ) == -1 )
      fprintf( stderr, "error: execvp failed\n" );
  }
}

/*____________________________________________________________________
  This routine handles piping two separate commands.
  The output of the command on the left of the pipe
  symbol becomes the input to the command on the right.
*/
void 
pipe_cmd( char** args ){
  
  int pos;
  // locate the pipe symbol.j
  for( pos = 0; strcmp(args[pos], "|") != 0  ; pos++ ){} 
  args[pos] = NULL;
  
  int pid = fork();
  if( pid < 0 ){
    fprintf( stderr, "error: fork failed\n");
    exit(1);
  }
  else if( pid > 0 ){
    wait(0);
  }
  else{
    int fd[2];            // declare the file descriptor 
    pipe(fd);             // array and create the pipe
    
    int pid2 = fork();
    if( pid2 < 0 ){
      fprintf( stderr, "error: fork failed\n" );
      exit(1);
    }
    else if( pid2 > 0 ){
      close(1);           // close stdout
      dup( fd[1] );       // replace with fd[1]
      close( fd[1] );     // close the pipe ends
      close( fd[0] ); 
      
      if( execvp( args[0], args ) == -1 )
	fprintf( stderr, "error: execvp failed\n");
    }
    else{
      close(0);           // close stdin
      dup( fd[0] );       // replace with fd[0]
      close( fd[1] );     // close the pipe ends
      close( fd[0] ); 
      
      // pass in a sub array of args starting 
      // at the point where the | symbol was found.
      // use that as the starting point for the
      // call to exec.
      if( execvp( args[pos + 1], &args[pos +1] ) == -1 )
	fprintf(stderr, "error: execvp failed\n");
    }
  }
}

/*____________________________________________________________________
  This routine determines what type of multiple
  argument command was entered and call the 
  appropriate routine to handle it.
*/
void 
mult_cmd( char** args ){

  int i;
  pid_t pid;
  int in_flag = 0, out_flag = 0, pipe_flag = 0;

  // search for <,>, or |
  for( i = 0; args[i] != NULL; i++ ){
    if( strcmp( args[i], "<") == 0 ){
      in_flag = 1;
    }
    else if( strcmp( args[i], ">") == 0 ){
      out_flag = 1;
    }
    else if( strcmp( args[i], "|") == 0 ){
      pipe_flag = 1;
    }
  }
  
  // determine which command was entered.
  // and call appropriate routine: <,>,|.
  if( in_flag )
    in_cmd( args );
  else if( out_flag )
    out_cmd( args );
  else if( pipe_flag )
    pipe_cmd( args );
  
  // if not <,>,| then execute normal command.
  else{
    
    pid = fork();
    if( pid < 0 ){
      fprintf( stderr, "error: fork failed\n");
      exit(1);
    }
    else if( pid > 0 ){
      // Parent code. 
      wait( 0 );
    }
    else{
      // Child executes command 
      if( execvp( args[0], args ) == -1 )
	fprintf( stderr, "error: execvp failed\n");
    }
  }
}

/*____________________________________________________________________
  This routine handles any shell command
  that contains only the executable file.
*/
void 
sngl_cmd( char** args ){
  pid_t pid;
  
  pid = fork();
  if( pid < 0 ){
    fprintf( stderr, "error: fork failed\n");
    exit(1);
  }
  else if( pid > 0 ){
    // Parent code
    wait( 0 );
  }
  else{
    // Child code
    if( execvp( args[0], args ) == -1 )
      fprintf( stderr, "error: execvp failed\n");
  }
}

/*____________________________________________________________________
  This routine determines whether the command 
  single argument or mulitiple argument and calls
  the appropriate routine.
*/
void
process_cmd( char** args ){
  
  if( args[1] != NULL ){
    // multiple argument command
    mult_cmd( args );
  }
  else{
    // single argument command
    sngl_cmd( args );
  }
}

/*____________________________________________________________________
  MAIN - reads input as commands to the shell 
  by using the getline() routine from the lex.l
  file provided. If "exit" then program exits.
*/
main(){
  char **args; 
  
  while(1){
    printf("myshell> ");
    args = getline(); 
    if( args[0] != NULL ){
      if( strcmp( args[0], "exit" ) == 0 )      
	exit(0);
      else
	process_cmd( args );
    }
  }
}
