#svi.c: stream editor assignment

[csc322](https://github.com/royhowie/csc322) final project from spring 2015

##overview
`svi.c` takes, as a command line argument, a file containing a list of edit commands. It modifies the standard input according to these commands. Each line of input, and output after editing, will be no more than 256 characters long.

note: `svi` has many similarities with the UNIX `sed`. In fact, `sed` is much better; the two programs are hardly comparable. This is just an exercise in c programming. (clearly)

##commands
Each line of the `edit file` is a command. A command has several parts.
	
	1. a line range specification
	2. an edit type
	3. and possibly some data

The line range specification can consist of `none`, `range`, or `text`. Looking at line 10:

	typedef enum { none, range, text } LineSpecType;

Here's an easy way to remember how they work:

	// none:                applies to every line
	// text:    /<text>/    applies to every line with <text>
	// range:   a,b/        applies to every line in a,b inclusive

There are several edit types. I have already detailed this in lines 25 to 32 of `svi.c`:

	char edit;
	// will be in { A, d, I, O, s };
	// edit operations
	//      A: append to line
	//      d: delete the line specification
	//      I: insert at beginning of line
	//      O: insert text on the line before
	//      s/<old>/<new>/ replaces all occurences of <old> with <new>

The `String data` is simply the rest of the string.

It is helpful to define a union (15-19):

	typedef union {
	    bool none;      // applies to every line
	    int range[2];   // applies to lines in range[0],range[1] inclusive
	    String text;    // applies to lines with this text
	} LineSpecRule;

to record the details of the line range specification.

We ultimately end up with a struct:

	typedef struct {
	    LineSpecType type;		// what type of line range specification
	    LineSpecRule rule;		// the details of how the line range specification should be applied
	    char edit;				// the type of edit being performed
	    String data;
	} Edit;

##running
	
	gcc svi.c -o svi
	./svi commandFile < inputFile