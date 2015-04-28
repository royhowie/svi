// Roy Howie
// Tue Apr 7 20:03:05 EDT 2015

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char String[256];
typedef enum { false, true } bool;
typedef enum { none, range, text } LineSpecType;
// none:                applies to every line
// text:    /<text>/    applies to every line with <text>
// range:   a,b/        applies to every line in a,b inclusive

typedef union {
    bool none;      // applies to every line
    int range[2];   // applies to lines in range[0],range[1] inclusive
    String text;    // applies to lines with this text
} LineSpecRule;

typedef struct {
    LineSpecType type;  // determines type of range of lines to which this `Edit` applies
    LineSpecRule rule;  // elaborates on `type` to exactly specify line range

    char edit;
    // will be in { A, d, I, O, s };
    // edit operations
    //      A: append to line
    //      d: delete the line specification
    //      I: insert at beginning of line
    //      O: insert text on the line before
    //      s/<old>/<new>/ replaces all occurences of <old> with <new>

    String data;
    // holds everything after the `edit` character
} Edit;

void    applyEdit              (Edit edit, String line);
void    applyEditsToStdIn      (Edit edits[], int length);
bool    canApplyEdit           (Edit edit, String currentLine, int lineNumber);
void    getTextDelimiters      (String data, String replace, String replaceWith);
void    substituteText         (Edit edit, String line);
Edit    makeLineRangeEdit      (String command);
Edit    makeTextEdit           (String command);
Edit    makeEverywhereEdit     (String command);
int     readInEditFile         (FILE * commands, Edit edits[]);
Edit    transformToEditStruct  (String input);

int main (int argc, char * argv[]) {
    FILE * commands;
    int length;
    Edit edits[100];

    if (argc < 2) {
        printf("error. please include a file name. e.g., `./svi editFile < inputFile`\n");
        return -1;
    } else if ((commands = fopen(argv[1], "r")) == NULL) {
        printf("file could not be opened. exiting.\n");
        return -1;
    } else {
        length = readInEditFile(commands, edits);
        applyEditsToStdIn(edits, length);
        return 0;
    }
}

void applyEdit (Edit edit, String line) {
    String copy;
    int length;
    switch (edit.edit) {
        case 'A':
            strcpy(copy, line);
            length = strlen(copy) - 1;
            if (copy[length] == '\n') {
                copy[length] = '\0';
            }
            strcat(copy, edit.data);
            strcpy(line, copy);
            break;
        case 'd':
            line[0] = '\0';
            break;
        case 'I':
            strcpy(copy, edit.data);
            length = strlen(copy) - 1;
            if (copy[length] == '\n') {
                copy[length] = '\0';
            }
            strcat(copy, line);
            strcpy(line, copy);
            break;
        case 'O':
            printf("%s", edit.data);
            break;
        case 's':
            substituteText(edit, line);
            break;
        default:
            printf("malformed edit passed to program. exiting.\n");
            exit(-1);
            break;
    }
}
void applyEditsToStdIn (Edit edits[], int length) {
    String input;
    int currentLine = 0, i;
    while (fgets(input, 256, stdin)) {
        currentLine += 1;
        for (i = 0; i < length; i++) {
            if (canApplyEdit(edits[i], input, currentLine)) {
                applyEdit(edits[i], input);
                // if the edit says to delete this line, immediately break
                // so that other operations cannot be applied 
                if (edits[i].edit == 'd') {
                    break;
                }
            }
        }
        printf("%s", input);
    }
}
bool canApplyEdit (Edit edit, String line, int lineNumber) {
    if (edit.type == range) {
        return edit.rule.range[0] <= lineNumber && lineNumber <= edit.rule.range[1];
    } else if (edit.type == text) {
        return strstr(line, edit.rule.text) != NULL;
    } else {
        return true;
    }
}
void getTextDelimiters (String data, String replace, String replaceWith) {
    String copy;
    char * delimiter = "/";
    char * token;

    // so as to not disturb `edit.data`, which is needed in `void substituteText`
    strcpy(copy, data);
    // split string at first '/' and copy to `replace`
    token = strtok(copy, delimiter);
    strcpy(replace, token);

    // split string at next 
    token = strtok(NULL, delimiter);
    strcpy(replaceWith, token);
}
int readInEditFile (FILE * commands, Edit edits[]) {
    String line;
    int i = 0; 
    while (i < 100 && fgets(line, 256, commands) != NULL) {
        edits[i] = transformToEditStruct(line);
        i += 1;
    }
    // close the file since it is no longer needed
    if (fclose(commands) == EOF) {
        perror("Unable to close file. Exiting program.");
        exit(-1);
    } else {
        return i;
    }
}
Edit makeLineRangeEdit (String command) {
    Edit s;
    String copy;
    char * delimiters = ",/";
    char * delimiter = "/";
    char * token;

    s.type = range;
    strcpy(copy, command);

    // the form of these edits is: a,b/<edit char><rest of text>
    // split at the first occurrence of a character in `delimiters`
    // if edits are properly formed, this will always be the comma after a
    token = strtok(copy, delimiters);

    // so parse `token` for an integer `a`
    s.rule.range[0] = atoi(token);

    // advance forward to the next `token` (which is the '/' after b)
    token = strtok(NULL, delimiters);

    // parse `token` for an integer `b`
    s.rule.range[1] = atoi(token);

    // grab the text after the '/' after `b`
    token = strtok(NULL, delimiter);

    // the first character afterwards will be the `Edit.edit` char
    s.edit = token[0];

    // the rest is `Edit.data`
    strcpy(s.data, token + 1);

    return s;
}
Edit makeTextEdit (String command) {
    Edit s;
    String copy;
    char * delimiter = "/";
    char * token;

    s.type = text;
    strcpy(copy, command);

    // split string at first instance of '/'
    token = strtok(copy, delimiter);
    // and copy this into `Edit.rule.text`
    strcpy(s.rule.text, token);

    // grab everything after the first slash to the null-terminator character
    token = strtok(NULL, "\0");
    // the first character is the edit type
    s.edit = token[0];
    // everything else is `Edit.data`
    strcpy(s.data, token + 1);

    return s;
}
Edit makeEverywhereEdit (String command) {
    Edit s;

    s.type = none;
    s.rule.none = true;
    strcpy(s.data, command + 1);
    s.edit = command[0];

    return s;
}
void substituteText (Edit edit, String line) {
    String copy, replace, replaceWith;
    char * token;
    int i, delta, lengthOfReplace, lineLength;

    getTextDelimiters(edit.data, replace, replaceWith);

    lengthOfReplace = strlen(replace);

    // in case there's garbage in `copy`
    strcpy(copy, "");

    // while line contains the text to be replaced
    while ((token = strstr(line, replace)) != NULL) {
        // let `delta` be the number of characters between the beginning of `line`
        // and the character at which `replace` was found in `line`
        delta = token - line;
        lineLength = strlen(line);

        // copy the `delta` characters of line before `replace` to `copy`
        strncat(copy, line, delta);
        // append `replaceWith` to `copy`
        strcat(copy, replaceWith);

        // shift the entire array forward, from the first character after the end of `replace`
        for (i = 0; i < lineLength - delta - lengthOfReplace + 1; i++) {
            line[i] = line[i + delta + lengthOfReplace];
        }
    }
    // while loop terminates when `replace` is no longer in `line`;
    // copy remaining character of `line` after last occurrence of `replace` to copy
    strcat(copy, line);
    // move `copy` to `line` (since it was passed by reference)
    strcpy(line, copy);
}
Edit transformToEditStruct (String command) {
    if ('0' <= command[0] && command[0] <= '9') {
        return makeLineRangeEdit(command);
    } else if (command[0] == '/') {
        return makeTextEdit(command);
    } else {
        return makeEverywhereEdit(command);
    }
}