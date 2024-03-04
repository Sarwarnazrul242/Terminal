#include "Llibc.h"
#include "posix-calls.h"

// In Lcli.c or wherever it's supposed to be defined
/*
int Ltokenize(char *str, char *tokens[], int maxTokens) {
    int count = 0;
    char *token = strtok(str, " ");
    while (token != NULL && count < maxTokens) {
        tokens[count++] = token;
        token = strtok(NULL, " ");
    }
    return count;
}
*/

//#include "Llibc.h"

// Custom tokenization function
int Ltokenize(char *str, char *tokens[], int maxTokens) {
    int count = 0;
    char *tokenStart = str;
    char *current = str;

    while (*current != '\0' && count < maxTokens) {
        // Skip leading delimiters
        while (*current != '\0' && Lisspace(*current)) {
            current++;
        }

        // If we've reached the end of the string, break out of the loop
        if (*current == '\0') {
            break;
        }

        // Mark the start of the token
        tokenStart = current;

        // Find the end of the token
        while (*current != '\0' && !Lisspace(*current)) {
            current++;
        }

        // Null-terminate the token
        if (*current != '\0') {
            *current = '\0';
            current++;
        }

        // Add the token to the tokens array
        tokens[count++] = tokenStart;
    }

    return count;
}

