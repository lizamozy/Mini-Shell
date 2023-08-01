#include <msh.h>
#include <msh_parse.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

struct msh_sequence{
	//points to the froht of the seqence
	struct msh_pipeline* head;
	//tail will point to the last pipeline with a null next pointer
	struct msh_pipeline* tail;

};

struct msh_pipeline{
	struct msh_command* commands[MSH_MAXCMNDS];
	char* input;
	struct msh_pipeline* next;
};

struct msh_command{
	char* args[MSH_MAXARGS+2];
	struct msh_command* next;
	//char* program;
	char* data;
	msh_free_data_fn_t free_data;
};

void
msh_execute(struct msh_pipeline *p)
{
	
	//check if its the first command takes nothing in but its ooutput goes directly to a middle command
	//or one of the middle commands (weave in)
	//if its the last command (puts this to standard out)
	int pipefd[2];
	struct msh_command* c = p->commands[0];
	pid_t pid;
	int prevfile = STDIN_FILENO;
		//check if its cd
		if(strcmp(c, "cd")==0){
			env("PATH");
			//gives you oath of the envornoment variables
			struct dir* getenv("HOME");
		}
		//check if you exit
		if(strcmp(c, "exit")==0){
			exit(EXIT_SUCCESS);
		}
		// if(strcmp(c, "~")==0){
		
		// }
	while(c->next != NULL){ 
		
		//case for when its the last command
		pid = fork();
		if(c->next==NULL){

			if (pid == -1) {
				perror_exit("Forking process");
			}
			//if we are in the child process
			if(pid == 0){
				
				close(STDIN_FILENO);
				if(dup2(prevfile, STDIN_FILENO) == -1){
					perror_exit("parent dup stdout");
				}
				//want to send result to out so we dont close out 
				execvp(c->args[0], c->args);
			}
		}

		if(pipe(pipefd)== -1){
			perror_exit("Opening pipe");
		} 
		//saving the parents file descriptor 
		if (pid == -1) {
			perror_exit("Forking process");
		}
		//if we are in the child process
		if(pid == 0){
			//creating a pipe that goes to the next cmmand
			close(STDIN_FILENO);
			if(dup2(prevfile, STDIN_FILENO) == -1){
				perror_exit("parent dup stdout");
			}
			close(STDOUT_FILENO);
			//dup2 standards in an
			if (dup2(pipefd[1], STDOUT_FILENO) == -1){
				perror_exit("child dup stdin");
			}

			execvp(c->args[0], c->args);
			//if we are at last command we will exec and then close everything 
		}
		//wait until the child process is over 
		//waitpid(pid);
		//saving the previous parents file descriptor 
		prevfile = pipefd[0];
		c = c->next;
	}
	return;
}

void
msh_init(void)
{
	//setting up signal handler for 
		//control -D (quits the program)
		//check for memory leaks and that nothing crashes
	return;
}
