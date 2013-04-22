char* trim(char* string) {
    // bail if NULL
    if (! string)
        return NULL;

    while(isspace(*string))
        string++;

    if(*string != 0) {
        char* end = string + strlen(string) - 1;
        while (end > string && isspace(*end))
            end--;
        *(end+1) = '\0';
    }

    return string;
}

/* remove all spaces from a cstring */
void nl2space(char* string) {
	while (*string != '\0') {
		if (*string == '\n') {
			*string = ' ';
			++string;
		} else {
			++string;
		}
	}
}

int countDelims(char* string, char* delim) {
	size_t count = 0;
	for (char* cursor = delim; *delim; ++delim) {
		while (*string) {
			if (*string == *cursor)
				count++;
			string++;
		}
	}
	return count;
}

/*
 * Tokenize the command string
 */
void splitLine(char* line, char*** result, char* delims) {
	*result = malloc(sizeof(char*) * (countDelims(line, delims) + 2));
    if (*result) {
        size_t idx = 0;
        char* token = strtok(line, delims);

        while (token) {
			(*result)[idx++] = strdup(token);
            token = strtok(0, delims);
        }
		(*result)[idx] = 0;              /* add terminating null */
    }
}

char* glueStrings(char** glued, char** strings, int count) {

	// get the length of the command
	size_t len = 0;
	for (int i = 1; i < count; ++i)
		len += strlen(strings[i]);

	// glue the strings together
	*glued  = malloc(sizeof(char) * len + 1);
	**glued = 0;
	for (int i = 1; i < count; ++i)
		strcat(*glued, strings[i]);
}
