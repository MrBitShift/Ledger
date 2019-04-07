#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>

#define check(A, B, ...) if (!(A)) { printf(B "\n", ##__VA_ARGS__); goto error; }
#define minimum(A, B) ((A) < (B)) ? (A) : (B)

#define MAX_STRING 64

#define CREATE_FLAG "create"
#define ADD_FLAG "add"
#define REMOVE_FLAG "remove"
#define CHANGE_FLAG "change"
#define READ_FLAG "read"
#define READ_ALL_FLAG "readall"
#define BALANCE_FLAG "balance"


typedef struct AccountEvent 
{
	double Amount;
	char Description[MAX_STRING];
	time_t EventTime;
	size_t ID;
} AccountEvent;

// reading functions

int PrintEvent(AccountEvent event)
{
	printf("Event ID %zu:\n", event.ID);
	printf("\tAmount: %lf\n", event.Amount);
	printf("\tDescription: \"%s\"\n", event.Description);
	printf("\tDate Made: %s", ctime(&event.EventTime));

	return 0;
}

int AccountRead(char *filename, size_t ID)
{
	FILE *file = NULL;
	check(filename != NULL, "filename can't be null.");

	file = fopen(filename, "r");
	check(file != NULL, "Couldn't open file \"%s\"", filename);

	AccountEvent tmp;
	while (1)
	{
		check(fread(&tmp, sizeof(AccountEvent), 1, file) == 1,
			"Could not find event with ID %zu.", ID);
		if (tmp.ID == ID)
		{
			PrintEvent(tmp);
			break;
		}
	}

	fclose(file);
	return 0;

error:
	fclose(file);
	return 1;
}

int AccountReadAll(char *filename)
{
	FILE *file = NULL;
	check(filename != NULL, "filename can't be null.");

	file = fopen(filename, "r");
	check(file != NULL, "Couldn't open file \"%s\"", filename);

	AccountEvent tmp;
	while (fread(&tmp, sizeof(AccountEvent), 1, file) == 1)
	{
		PrintEvent(tmp);
	}

	fclose(file);
	return 0;

error:
	fclose(file);
	return 1;
}

int AccountBalance(char *filename)
{
	FILE *file = NULL;
	check(filename != NULL, "filename can't be null.");

	file = fopen(filename, "r");
	check(file != NULL, "Couldn't open file \"%s\"", filename);

	double balance = 0;
	AccountEvent tmp;
	while (fread(&tmp, sizeof(AccountEvent), 1, file) == 1)
	{
		balance += tmp.Amount;
	}

	printf("Balance: %lf\n", balance);

	return 0;

error:
	fclose(file);
	return 1;
}

// editing functions
int AccountCreate(char *filename)
{
	FILE *file = NULL;
	check(filename != NULL, "filename can't be null.");

	// first try reading it to see if it exists
	file = fopen(filename, "r");
	check(file == NULL, "The file \"%s\" already exists.", filename);

	file = fopen(filename, "w");
	check(file != NULL, "Could not create file \"%s\". Try running with admin permissions.", filename);
	fclose(file);

	return 0;

error:
	fclose(file);
	return 1;
}

int AddEvent(char *filename, double amount, char *description)
{
	FILE *file = NULL;
	check(filename != NULL, "filename can't be null.");
	check(description != NULL, "description can't be null.");

	// first check that file exists
	file = fopen(filename, "r");
	check(file != NULL, "The file \"%s\" does not exist.", filename);
	fclose(file);

	file = fopen(filename, "a+"); // open for appending and reading
	check(file != NULL, "Can't open file \"%s\"", filename);

	// seek to end and get length
	fseek(file, 0, SEEK_END);
	size_t fileLen = ftell(file);

	// get the last used id
	size_t newId;
	// get the id
	if (fileLen >= sizeof(AccountEvent)) // the file isn't empty so read the last element
	{
		AccountEvent tmp;
		fseek(file, -1 * (long)sizeof(AccountEvent), SEEK_END); // seek to end - one event
		fread(&tmp, sizeof(AccountEvent), 1, file);
		newId = tmp.ID + 1;
	}
	else // the file is empty so newId is 0;
	{
		newId = 0;
	}

	AccountEvent newEvent;
	newEvent.Amount = amount;
	memcpy(newEvent.Description, description, minimum(strlen(description) + 1, MAX_STRING - 1));
	newEvent.Description[MAX_STRING - 1] = '\0';
	newEvent.EventTime = time(NULL);
	newEvent.ID = newId;

	fwrite(&newEvent, sizeof(AccountEvent), 1, file);

	fflush(file);
	fclose(file);

	return 0;

error:
	fflush(file);
	fclose(file);
	return 1;
}

int RemoveEvent(char *filename, size_t eventID)
{
	FILE *file = NULL;
	check(filename != NULL, "filename can't be null.");

	// open file and check exists
	file = fopen(filename, "r+");
	check(file != NULL, "Couldn't open \"%s\"", filename);
	
	AccountEvent tmp;
	while (1)
	{
		check(fread(&tmp, sizeof(AccountEvent), 1, file) == 1, 
			"Did not find event ID %zu in file \"%s\"", eventID, filename);
		if (tmp.ID == eventID) // event found, break
			break;
	}

	// since event found, move all events back one position
	while (fread(&tmp, sizeof(AccountEvent), 1, file) == 1)
	{
		fseek(file, -2 * (long)sizeof(AccountEvent), SEEK_CUR); // move cursor to element to replace
		fwrite(&tmp, sizeof(AccountEvent), 1, file);
		fseek(file, sizeof(AccountEvent), SEEK_CUR); // move cursor to next element to move back
	}
	// seek to end minus one element and truncate
	fseek(file, -1 * (long)sizeof(AccountEvent), SEEK_END);
	_chsize(_fileno(file), ftell(file));

	fflush(file);
	fclose(file);
	return 0;

error:
	fflush(file);
	fclose(file);
	return 1;
}

int ChangeEvent(char *filename, size_t eventID, double amount, char *description)
{
	FILE *file = NULL;
	check(filename != NULL, "filename can't be null.");

	// open file and check exists
	file = fopen(filename, "r+");
	check(file != NULL, "Couldn't open \"%s\"", filename);

	AccountEvent tmp;
	while (1)
	{
		check(fread(&tmp, sizeof(AccountEvent), 1, file) == 1, 
			"Could not find event with ID %zu.", eventID);
		if (tmp.ID == eventID)
		{
			tmp.Amount = amount;
			memcpy(tmp.Description, description, min(strlen(description) - 1, MAX_STRING - 1));
			// now go back 1 element and write new event
			fseek(file, -1 * (long)sizeof(AccountEvent), SEEK_CUR);
			fwrite(&tmp, sizeof(AccountEvent), 1, file);
			break;
		}
	}

	fflush(file);
	fclose(file);
	return 0;

error:
	fflush(file);
	fclose(file);
	return 1;
}

int testAddEvent()
{
	char *testFile = "test.dat";
	check(AddEvent(testFile, 5, "hello1") == 0, "Couldn't add data.");
	check(AddEvent(testFile, 2, "hello2") == 0, "Couldn't add data.");
	check(AddEvent(testFile, -3, "hello3") == 0, "Couldn't add data.");

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
	printf("Ledger v0.1.0\nCreated by Nathan Constantinides.\nSee LICENSE for more information.\n\n");

	check(AccountCreate("test.dat") == 0, "Operation Failed. Exiting.");

	check(testAddEvent() == 0, "Operation Failed. Exiting.");

	check(AccountReadAll("test.dat") == 0, "Operation Failed. Exiting.");

	check(AccountBalance("test.dat") == 0, "Operation Failed. Exiting.");

	check(RemoveEvent("test.dat", 0) == 0, "Operation Failed. Exiting");

	printf("After removing 0:\n");

	check(AccountReadAll("test.dat") == 0, "Operation Failed. Exiting.");

	check(ChangeEvent("test.dat", 1, 100, "changed") == 0, "Operation Failed. Exiting.");

	printf("After changing event 1:\n");

	check(AccountReadAll("test.dat") == 0, "Operation Failed. Exiting.");

	printf("Reading 1:\n");

	check(AccountRead("test.dat", 1) == 0, "Operation Failed. Exiting.");

	return 0;

error:
	return 1;
}