#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define check(A, B, ...) if (!(A)) { printf(B "\n", ##__VA_ARGS__); goto error; }

#define MAX_STRING 64

#define CREATE_FLAG "create"
#define ADD_FLAG "add"
#define REMOVE_FLAG "remove"
#define CHANGE_FLAG "change"


typedef struct AccountEvent 
{
	double Amount;
	char Description[MAX_STRING];
	time_t EventTime;
	size_t ID;
} AccountEvent;

int AccountCreate(char *filename)
{
	check(filename != NULL, "filename can't be null.");

	// first try reading it to see if it exists
	FILE *file = fopen(filename, "r");
	check(file == NULL, "The file \"%s\" already exists.", filename);

	file = fopen(filename, "w");
	check(file != NULL, "Could not create file \"%s\". Try running with admin permissions.", filename);
	fclose(file);

	return 0;

error:
	return 1;
}

int testAccountWrite()
{
	FILE *accountFile = fopen("test.dat", "w+");
	check(accountFile != NULL, "could not open test.dat");

	AccountEvent *tests = calloc(3, sizeof(AccountEvent));
	
	tests[0].Amount = 5;
	memcpy(tests[0].Description, "hello1", 7); // 7 so that null is included.
	tests[0].EventTime = time(NULL);
	tests[0].ID = 0;

	tests[1].Amount = 6;
	memcpy(tests[1].Description, "hello2", 7); // 7 so that null is included.
	tests[1].EventTime = time(NULL);
	tests[1].ID = 1;

	tests[2].Amount = -3;
	memcpy(tests[2].Description, "hello3", 7); // 7 so that null is included.
	tests[2].EventTime = time(NULL);
	tests[2].ID = 2;

	fwrite(tests, sizeof(AccountEvent), 3, accountFile);
	fflush(accountFile);
	fclose(accountFile);
	return 0;

error:
	return 1;
}

int testAccountRead()
{
	FILE *file = fopen("test.dat", "r");
	check(file != NULL, "Couldn't open \"test.dat\".");

	AccountEvent *events = calloc(1, sizeof(AccountEvent));
	size_t i = 0;
	while (fread(&(events[i]), sizeof(AccountEvent), 1, file) == 1)
	{
		printf("Reading at iteration %zu\n", i);
		printf("\tAmount: %lf\n", events[i].Amount);
		printf("\tDescription: %s\n", events[i].Description);
		printf("\tTime: %s", ctime(&(events[i].EventTime)));
		printf("\tId: %zu\n", events[i].ID);

		i++;
		events = realloc(events, (i + 1) * sizeof(AccountEvent));
	}

	return 0;

error:
	return 1;
}

int main(int argc, char *argv[])
{
	printf("Ledger v0.0.0\nCreated by Nathan Constantinides\nSee LICENSE for more information.\n\n");

	check(AccountCreate("test.dat") == 0, "Operation Failed. Exiting.");

	check(testAccountWrite() == 0, "Operation Failed. Exiting.");

	check(testAccountRead() == 0, "Operation Failed. Exiting.");

	return 0;

error:
	return 1;
}