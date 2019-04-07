#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <inttypes.h>
#include <Shlwapi.h>

#define check(A, B, ...) if (!(A)) { printf(B "\n", ##__VA_ARGS__); goto error; }
#define minimum(A, B) ((A) < (B)) ? (A) : (B)

#define MAX_STRING 64

#define HELP \
"Ledger v1.0.0\n\
Created by MrBitShift\n\
See LICENSE for more information\n\
\n\
USAGES:\n\
Ledger help\n\
	Displays this help message.\n\
Ledger create <filename>\n\
	Creates a new account in file <filename>\n\
	<filename>:\n\
		The name of the file to create the ledger in.\n\
Ledger add <filename> <amount> <description>\n\
	Adds a new event to a ledger.\n\
	<filename>:\n\
		The name of the file to add the event to.\n\
	<amount>:\n\
		A decimal number indicating amount of money to add to the event (negative values indicate withdrawls)\n\
	<description>:\n\
		A description of the event. (make sure it is surrounded by double quotes)\n\
Ledger remove <filename> <eventID>:\n\
	Removes an event from a ledger.\n\
	<filename>:\n\
		The name of the file to remove the event from.\n\
	<eventID>:\n\
		An integer indicating which event to remove from the file (see Ledger readall to obtain the ID)\n\
Ledger change <filename> <eventID> <amount> <description>:\n\
	Changes the values of an event in a ledger.\n\
	<filename>:\n\
		The name of the file to change the event in.\n\
	<eventID>:\n\
		An integer indicating which event to change in the file (see Ledger readall to obtain the ID)\n\
	<amount>:\n\
		The new amount to assign to the event\n\
	<description>:\n\
		The new description to assign to the event\n\
Ledger read <filename> <eventID>:\n\
	Reads a single event from a ledger.\n\
	<filename>:\n\
		The name of the file to read the event from.\n\
	<eventID>:\n\
		An integer indicating which event to read. (see Ledger readall to obtain the ID)\n\
Ledger readall <filename>:\n\
	Reads all events from a ledger file.\n\
	<filename>:\n\
		The name of the file to read the events from.\n\
Ledger balance <filename>:\n\
	Reads the current balance in a ledger file.\n\
	<filename>:\n\
		The name of the file to read the balance from.\n\
"

#define HELP_FLAG "help"
#define CREATE_FLAG "create"
#define ADD_FLAG "add"
#define REMOVE_FLAG "remove"
#define CHANGE_FLAG "change"
#define READ_FLAG "read"
#define READ_ALL_FLAG "readall"
#define BALANCE_FLAG "balance"

// stores individual transactions with date, amount, and a description of the transaction
typedef struct AccountEvent 
{
	double Amount;
	char Description[MAX_STRING];
	time_t EventTime;
	size_t ID;
} AccountEvent;

// reading functions

// Prints AccountEvent in human readable format
int PrintEvent(AccountEvent event)
{
	printf("Event ID %zu:\n", event.ID);
	printf("\tAmount: %lf\n", event.Amount);
	printf("\tDescription: \"%s\"\n", event.Description);
	printf("\tDate Made: %s", ctime(&event.EventTime));

	return 0;
}

// Prints the value of a transaction with ID found in file in human readable format.
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
	return 1;
}

// Prints all AccountEvents in file in human readable format
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
	return 1;
}

// Prints current balance of account.
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

	fclose(file);
	return 0;

error:
	return 1;
}

// Creates an empty file if it doesn't already exist.
int AccountCreate(char *filename)
{
	FILE *file = NULL;
	check(filename != NULL, "filename can't be null.");

	// check if file exists
	check(PathFileExistsA(filename) == 0, "The file \"%s\" already exists.", filename);

	file = fopen(filename, "w");
	check(file != NULL, "Could not create file \"%s\". Try running with admin permissions.", filename);

	fclose(file);
	return 0;

error:
	return 1;
}

// Appends an AccountEvent to the end of a file with the values provided and current date.
int AddEvent(char *filename, double amount, char *description)
{
	FILE *file = NULL;
	check(filename != NULL, "filename can't be null.");
	check(description != NULL, "description can't be null.");

	// first check that file exists
	check(PathFileExistsA(filename) == 1, "The file \"%s\" does not exist.", filename);

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

	fseek(file, 0, SEEK_END);

	fwrite(&newEvent, sizeof(AccountEvent), 1, file);

	fflush(file);
	fclose(file);

	return 0;

error:
	return 1;
}

// Removes the AccountEvent with ID eventID from a file.
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
	return 1;
}


// changes the values of AccountEvent with the ID eventID in file.
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
	return 1;
}

int main(int argc, char *argv[])
{
	//printf("Ledger v0.1.0. Created by Nathan Constantinides. See LICENSE for more information.\n\n");

	if (argc == 3 && strcmp(argv[1], CREATE_FLAG) == 0)
		// ledger create <filename>
	{
		AccountCreate(argv[2]);
	}
	else if (argc == 5 && strcmp(argv[1], ADD_FLAG) == 0)
		// ledger add <filename> <amount> <description>
	{
		double amount;
		char *lastReadChar;
		amount = strtod(argv[3], &lastReadChar);
		check(*lastReadChar == '\0', "\"amount\" must be a valid number.");
		AddEvent(argv[2], amount, argv[4]);
	}
	else if (argc == 4 && strcmp(argv[1], REMOVE_FLAG) == 0)
		// ledger remove <filename> <eventID>
	{
		size_t id;
		char *lastReadChar;
		id = strtoumax(argv[3], &lastReadChar, 10); // read id in base 10
		check(*lastReadChar == '\0', "\"eventID\" must be a valid number.");
		RemoveEvent(argv[2], id);
	}
	else if (argc == 6 && strcmp(argv[1], CHANGE_FLAG) == 0)
		// ledger change <filename> <eventID> <amount> <description>
	{
		size_t id;
		double amount;
		char *lastReadChar;

		id = strtoumax(argv[3], &lastReadChar, 10);
		check(*lastReadChar == '\0', "\"eventID\" must be a valid number.");

		amount = strtod(argv[4], &lastReadChar);
		check(*lastReadChar == '\0', "\"amount\" must be a valid number.");

		ChangeEvent(argv[2], id, amount, argv[5]);
	}
	else if (argc == 4 && strcmp(argv[1], READ_FLAG) == 0)
		// ledger read <filename> <eventID>
	{
		size_t id;
		char *lastReadChar;

		id = strtoumax(argv[3], &lastReadChar, 10);
		check(*lastReadChar == '\0', "\"eventID\" must be a valid number.");

		AccountRead(argv[2], id);
	}
	else if (argc == 3 && strcmp(argv[1], READ_ALL_FLAG) == 0)
		// ledger readall <filename>
	{
		AccountReadAll(argv[2]);
	}
	else if (argc == 3 && strcmp(argv[1], BALANCE_FLAG) == 0)
		// ledger balance <filename>
	{
		AccountBalance(argv[2]);
	}
	else if (argc == 2 && strcmp(argv[1], HELP_FLAG) == 0)
		// ledger help
	{
		printf(HELP);
	}
	else
	{
		check(0, "Improper usage. See \"Ledger help\"");
	}

	return 0;

error:
	return 1;
}