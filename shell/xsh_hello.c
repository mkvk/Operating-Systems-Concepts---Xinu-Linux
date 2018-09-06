/* xsh_hello.c - xsh_hello */

#include <xinu.h>
#include <string.h>
#include <stdio.h>

/*----------------------------------------------------------------------------------------------------------
 * xsh_hello - display a personalised Xinu welcome message to the user by taking his/her  name as parameter
 *----------------------------------------------------------------------------------------------------------
 */

shellcmd xsh_hello(int nargs, char * args[]) {

	/* Output info for '--help' argument */
	if (strncmp(args[1], "--help", 7) == 0) {    
		printf("Usage: %s <string>\n", args[0]);
		printf("Description: Displays a greeting message to the user when the name is entered as a single word after hello command\n");
		printf("Options:  --help  Display this help and exit\n");
		return 0;
	}

	/* Check the argument count and display the error message accordingly for arguments that are either  more or less than requred. */
	else if (nargs != 2) {
		if(nargs > 2)
			fprintf(stderr, "%s: too many arguments", args[0]);
		else
			fprintf(stderr, "%s: too few arguments", args[0]);
		fprintf(stderr, "\nTry '%s --help' for more information\n", args[0]);
		return 1;
	}

	/* Print the message welcoming user */
	printf("Hello %s, Welcome to the world of Xinu!!\n", args[1]);
	return 0;
}
