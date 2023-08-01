#include <msh_parse.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sunit.h>

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

void msh_pipeline_free(struct msh_pipeline *p)
{
	int i,j;
	for(i=0; i < MSH_MAXCMNDS; i++){
		if(p->commands[i] == NULL) {
			break;
		}
		for(j=0; j < MSH_MAXARGS; j++){
			if(p->commands[i]->args[j]==NULL){
				break;
			}
			free(p->commands[i]->args[j]);
		}
		free(p->commands[i]);
	}
	if(p->input != NULL){
		free(p->input);
	}
	free(p);
}

void msh_sequence_free(struct msh_sequence *s)
{
	//need to create a temp pipeline that you pass in to pipelinefree
	struct msh_pipeline* temp_pipe;
	temp_pipe = s->head;
	while(temp_pipe != NULL){
		struct msh_pipeline* passed_in = temp_pipe->next;
		msh_pipeline_free(temp_pipe);
		temp_pipe = passed_in;
	}
	free(s);
}

struct msh_sequence * msh_sequence_alloc(void)
{
	struct msh_sequence* sequence = calloc(1, sizeof(struct msh_sequence));
	if (sequence == NULL){
		return NULL;
	}
	sequence->head = NULL;
	sequence->tail = NULL;

	return sequence;
}

char * msh_pipeline_input(struct msh_pipeline *p)
{
	return p->input;
}

msh_err_t msh_sequence_parse(char *str, struct msh_sequence *seq)
{
    char *token, *token2, *token3, *ptr, *ptr2, *ptr3;
	char* tempstr = strdup(str);
	int pipe_counter = 0;
	//here we get commands

	struct msh_pipeline* prev_pipe = NULL;
    for (token = strtok_r(tempstr, ";", &ptr); token != NULL; token = strtok_r(ptr, ";", &ptr)){
		//allocate space for a newpipe
		//checks for so many pipes
		if(pipe_counter>= 16){
			free(tempstr);
			return -20;
		}
		struct msh_pipeline* pipe = calloc(1, sizeof(struct msh_pipeline));
		if(seq->head == NULL && seq->tail == NULL){
			//opreations for new pipe
			pipe->next = NULL;
			pipe->input = strdup(token);
			seq->head = pipe;
			seq->tail = pipe;	
		}else{
			//move tail pointer to 
			seq->tail->next = pipe;
			seq->tail = pipe;
			seq->tail->input = strdup(token);
		}
		pipe_counter++;
		int command_counter=0;
		struct msh_command* prev_command = NULL;
		//here we get subcommands for a pipe
        for (token2 = strtok_r(token, "|", &ptr2); token2 != NULL; token2 = strtok_r(ptr2, "|", &ptr2),command_counter++) {
			if(command_counter >= MSH_MAXCMNDS-1){
				//if we are over, free that pipe and throw error message
				free(tempstr);
				return MSH_ERR_TOO_MANY_CMDS;
			}
			//else if: not over number of commands, we need to allocate a command
			struct msh_command* command = calloc(1, sizeof(struct msh_command));
			//command next would be null
			command->next=NULL;
			pipe->commands[command_counter] = command;
			if(prev_command != NULL){
				prev_command->next=command;
			}
			prev_command=command;
			//need to keep track of the previours 
			int arg_counter=0;
			//increment counter for the command counter
			for(token3 = strtok_r(token2, " ", &ptr3); token3 != NULL; token3 = strtok_r(ptr3, " ", &ptr3), arg_counter++){
				//need to check if we are over the number of args + the program so 17
				if(arg_counter == MSH_MAXARGS+2){
					//for loop through maxargs plus one and when you find a null one you break out of the for loop to free them
					free(tempstr);
					int i;
					//free the args
					for(i=0; i<MSH_MAXARGS+2;i++){
						if(command->args[i]==NULL){
							break;
						}
						free(command->args[i]);
					}
					//need to free all of the args
					return MSH_ERR_TOO_MANY_ARGS;
				}
				//we are putting each arg into the args array
				command->args[arg_counter] = strdup(token3);
			}
			if(arg_counter==0){
				free(tempstr);

				msh_pipeline_free(pipe);
				if(prev_pipe !=NULL){
					prev_pipe->next=NULL;
				}
				else {
					seq->head = NULL;
					seq->tail = NULL;
				}
				return MSH_ERR_PIPE_MISSING_CMD;
			}
		}
		prev_pipe = pipe;
    }
	//need to free all of the strings that we string duped
	free(tempstr);
	return 0;
}

//dequeues the first pipeline in the sequence.
struct msh_pipeline * msh_sequence_pipeline(struct msh_sequence *s)
{
	//check if the head is null; if head it null return null
	if(s->head==NULL){
		return NULL;
	}
	//create a temp pointer to the head
	struct msh_pipeline* temp_pipe;
	temp_pipe = s->head;
	//make the head point to the next pipeline 
	s->head = s->head->next;

	return temp_pipe;
}

//queries a specific command in the pipeline.
struct msh_command *msh_pipeline_command(struct msh_pipeline *p, size_t nth)
{
	return p->commands[nth];
}

//determines whether or not a pipeline should return in the backroudn or not
//1 is yes, 0 otherwise 
int msh_pipeline_background(struct msh_pipeline *p)
{
	(void)p;

	return 0;
}

//tells us if the command `c` is the final command in the pipeline, or not.
//0 if it is, 1 otherwise
int msh_command_final(struct msh_command *c)
{
	//if next command is null, true. 
	if(c->next==NULL){
		return 1;
	}
	return 0;
}

void msh_command_file_outputs(struct msh_command *c, char **stdout, char **stderr)
{
	(void)c;
	(void)stdout;
	(void)stderr;
	// if(stdout == NULL && stderr == NULL){
	// }
	// stdout = c->args;
	// stderr = c->program;
}

char * msh_command_program(struct msh_command *c)
{
	return c->args[0];
}
//this function is wrong
//retrieves the arguments for the command as a`NULL`-terminated array of strings (see the `execv` argument list as an example).
char ** msh_command_args(struct msh_command *c)
{
	return c->args;
}

void msh_command_putdata(struct msh_command *c, void *data, msh_free_data_fn_t fn)
{
	(void)fn;
	(void)c;
	(void)data;
}

void *msh_command_getdata(struct msh_command *c)
{
	(void)c;
	return NULL;
}
