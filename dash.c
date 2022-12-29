#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/wait.h>
#include<fcntl.h>
#define DEF_PATH  "/bin"
#define MSG "An error has occured\n"

char *standardPath = NULL;

void errorMsg()
{
	printf(MSG);
}

void changePath(char* path){
	if(path == NULL)
	{
		errorMsg();
		exit(0);
	}
	// changing the path variable
	if(standardPath != NULL)
	{
		free(standardPath);
		standardPath = NULL;
	}
	standardPath = strdup(path);
}


void executePath(char *finalPath,char **cmdArray)
{	
	pid_t pId,waitId;
	pId = fork();
	int status;
	//child process is created and now we are checking if there is any redirection
	int redirectionIndex = 0; // stores the index of the redirection file name
	int count = 0; // checking if there is no more than on redirection symbol
	if(pId == 0)
	{	
		int index = 0;
		//checking if there is an error in redirection symbol
		
		while(cmdArray[index] != NULL)
		{
		
			if(strstr(cmdArray[index],">>"))
			{
				errorMsg();
				return;
		
			}
			// If the redirection symbol is correct 
			if(strcmp(cmdArray[index],">") == 0)
			{
				redirectionIndex = index+1;
				count++;
				//checking if there is only one index which contains the redirection file name
				//if there is more than one or none it should show an error
				
				if(cmdArray[index+1] == NULL || cmdArray[index+2] !=NULL)
				{
					errorMsg();
					return;
				}
				
			}
			index++;
			
		}

		//checking if there is only one redirection in one command
		// if there are more than one then it should show an error
		if(count>1)
		{
			errorMsg();
			return;
		}
		// if there is only one redirection then the output has to be seen in the entered external file
		else if(count == 1) 
		{
			char *outputPath = strdup(cmdArray[redirectionIndex]);
			close(STDOUT_FILENO);
			open(outputPath, O_CREAT | O_WRONLY | O_TRUNC,S_IRWXU);
			cmdArray[redirectionIndex-1] = NULL; //resetting the indices
			cmdArray[redirectionIndex] = NULL;
		}
		//If execv fails
		if(execv(finalPath,cmdArray)==-1)
		{
			errorMsg();
			return;
		}
	}
	// if pId is less than zero which is neither a child nor a parent process
	else if(pId < 0)
	{
		errorMsg();
		exit(0);
	}
	else
	
	{
	// letting the parent wait until the child dies	
		do
		{
			waitId = waitpid(pId,&status,WUNTRACED);	

		}while(!WIFEXITED(status) && !WIFSIGNALED(status)); 
		waitId++;
		
	}
}

void validateCommand(const char **cmdArray) 
{
	//checking if a command is present in the array
	if(cmdArray[0] == NULL)
	{
		return;
	}
	//checking for the exit command
	if(strcmp(cmdArray[0],"exit") == 0)
	{
		//checking if there is any command after the exit command
		if(cmdArray[1] != NULL)
		{
			errorMsg();
			return;
		}
		exit(0);
	}
	//checking for the change directory - "cd" command
	if(strcmp(cmdArray[0],"cd") == 0)
	{
		if(cmdArray[2]!=NULL || chdir(cmdArray[1]) == -1)
		{
			errorMsg();
		}
		return;
	}
	//checking for the "path" command
	if(strcmp(cmdArray[0],"path")==0)
	{
		char *pathToBeSent = NULL;
		//checking if there is a path next to path command
		int lengthOfPath = 0;
		//Finding the total length to be allocated to the pathToBeSent
		if(cmdArray[1] == NULL)
		{
			return;
		}
		int index = 1;
		int count = 0;
		for(index = 1;cmdArray[index]!=NULL;index++)
		{		
			//Counting one length extra to allocate space after each word
			lengthOfPath = lengthOfPath + (strlen(cmdArray[index])+1);	
		}
		//Allocating the required size of memory 
		
		count = index;
		pathToBeSent = malloc(lengthOfPath * sizeof(char *));

		//If allocation of memory is failed
		if(pathToBeSent == NULL)
		{
			return;
		}
		//Concatinating the elements of the array to pathToBeSent
		int i = 1;
		while(cmdArray[i]!=NULL)
		{
			strcat(pathToBeSent,cmdArray[i]);
			strcat(pathToBeSent," ");
			i++;
		}

		if(pathToBeSent == NULL)
		{
			return;
		}
		changePath(pathToBeSent);
		return;
	}

	int flag1 = -1;
	char *finalPath = NULL; 
	char *execPath = strdup(standardPath);
	const char spaceDelimiter[] = " \t\r\n\v\f";
	char *space = strtok(execPath," ");
	int flag = 0;
	// removing the white spaces
	while(space!=NULL)
	{
		finalPath = (char *)malloc((sizeof(space) + sizeof(cmdArray[0])+2)*sizeof(char*));
		//making the final path
		strcpy(finalPath,space);
		strcat(finalPath,"/");
		strcat(finalPath,cmdArray[0]);
		//cheching if the final path can be executed
		flag1 = access(finalPath,X_OK);
		if(flag1 == 0)
		{	
			
			executePath(finalPath,(char **)cmdArray);
			break;
		}
		space = strtok(NULL," ");
	}
	//if the final path cannot be executed print error
	if(flag1 == -1)
	{
		errorMsg();
	}	
}

void filterCommand(char *commandExec)
{
	
	// If the command which is passed is NULL, then error
	if(commandExec == NULL)
	{
		errorMsg();
		return;
	} 
	
	const char *cmdArray[1024];
	int index = 0;
	const char spaceDelimiter[] = " \f\n\r\t\v";
	char *strTokenized = strdup(commandExec);
	char *strToken = strdup(commandExec);
	//checking if there is no command on the left side of '&' which will lead to an error
	if(strstr(strToken,"&") != NULL)
	{
		//if the first word is '&', return error
		if(strcmp(strtok(strToken,spaceDelimiter),"&")==0)
		{
			errorMsg();
			return;
		}
	}

	const char ampersandDelimiter[] = "&";
	const char redirectDelimiter[] = ">";
	char *saveAmpersand = NULL;
	char *ampersandToken = NULL;
	char *space = NULL;
	char *spaceToken = NULL;
	char *redirectionToken = NULL;
	char *redirection = NULL;
	//Tokenizing the words with '&' as delimiter
	char *ampersand = NULL;
	ampersand = strtok_r(strTokenized,"&",&saveAmpersand);
	while(ampersand!=NULL)
	{	
		//Tokenizing the words with " " as delimiter
		space = strtok(ampersand,spaceDelimiter);
		while(space!=NULL)
		{	
			//checking the presence of redirection token
			if(strstr(space,">")!=NULL)
			{	
				//if the token is redirection symbol without spaces before or after it
				if(strcmp(space,">") != 0)
				{
					redirection = strtok(space,redirectDelimiter);
					cmdArray[index] = strdup(redirection);
					index++;
					cmdArray[index] = strdup(">");
					index++;
					redirection = strtok(NULL,redirectDelimiter);
					cmdArray[index] = strdup(redirection);
					index++;
					break;
				}
			}
			//If there is redirection with spaces before and after it or any other command
		
			cmdArray[index] = strdup(space);
			index++;
			space = strtok(NULL,spaceDelimiter);
		}
	
		//Making the last Index null 
		cmdArray[index] = NULL;
		validateCommand(cmdArray);
		index =0;
		ampersand = strtok_r(NULL,"&",&saveAmpersand);
	}
}


void batchMode(char *command)
{
	//Checking if the commad is not null
	if(command == NULL)
	{
		errorMsg();
		return;
	}

	//file open
	FILE *fileRedirect = fopen(command,"r");

	// If the opening of file failed
	if(fileRedirect == NULL)
	{
		errorMsg();
		return;
	}
	size_t sizeOfCommand = 0;
	char *commandLine = NULL;
	//Reading file line by line
	while(1)
	{
		sizeOfCommand = getline(&commandLine,&sizeOfCommand,fileRedirect);

		//Reading of lines in the file stops when EOF is reached
		if(sizeOfCommand == EOF)
		{
			break;
		}
		//Sending the commands for execution
		filterCommand(commandLine);		
	}
	fclose(fileRedirect);
}


int main(int argc, char** argv)
{	
	//changing the standard path varible to /bin
	changePath(DEF_PATH);
	//If there is only one Argument, start interactive mode
	if(argc == 1)
	{
		char *commandExec = NULL; // to store the command
	        size_t commandExecSize = 0; // size of command size
		printf("dash> ");
		getline(&commandExec, &commandExecSize,stdin);
		while(1)
		{
			filterCommand(commandExec);
			printf("dash> ");
			getline(&commandExec,&commandExecSize,stdin);
		}
	}
	//If there is more that one argument, start Batch Mode
	else if(argc == 2)
	{
		batchMode(argv[argc-1]);
	}

	//more than two arguments, error
	else
	{
		errorMsg();
		exit(1);
	}
	return 0;	
}
