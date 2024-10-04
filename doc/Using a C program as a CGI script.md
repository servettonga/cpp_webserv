# Using a C program as a CGI script

In order to set up a C program as a CGI script, it needs to be turned into a binary executable program. This is often problematic, since people largely work on Windows whereas servers often run some version of UNIX or Linux. The system where you develop your program and the server where it should be installed as a CGI script may have quite different architectures, so that the same executable does not run on both of them.

This may create an unsolvable problem. If you are not allowed to log on the server and you cannot use a binary-compatible system (or a cross-compiler) either, you are out of luck. Many servers, however, allow you log on and use the server in interactive mode, as a “shell user,” and contain a C compiler.

> You need to compile and load your C program on the server (or, in principle, on a system with the same architecture, so that binaries produced for it are executable on the server too).

Normally, you would proceed as follows:

1) Compile and test the C program in normal interactive use.
2) Make any changes that might be needed for use as a CGI script. The program should read its input according to the intended form sub­mis­sion method. Using the default `GET` method, the input is to be read from the environment variable. `QUERY_STRING`. (The program may also read data from files—but these must then reside on the server.) It should generate output on the standard output stream (`stdout`) so that it starts with suitable HTTP headers. Often, the output is in HTML format.
3) Compile and test again. In this testing phase, you might set the environment variable `QUERY_STRING` so that it contains the test data as it will be sent as form data. E.g., if you intend to use a form where a field named foo contains the input data, you can give the command
`setenv QUERY_STRING "foo=42"` (when using the tcsh shell)
or
`QUERY_STRING="foo=42"` (when using the bash shell).
4) Check that the compiled version is in a format that works on the server. This may require a recompilation. You may need to log on into the server computer (using Telnet, SSH, or some other terminal emulator) so that you can use a compiler there.
5) Upload the compiled and loaded program, i.e. the executable binary program (and any data files needed) on the server.
6) Set up a simple HTML document that contains a form for testing the script, etc.

You need to put the executable into a suitable directory and name it according to server-specific conventions. Even the compilation commands needed here might differ from what you are used to on your workstation. For example, if the server runs some flavor of Unix and has the Gnu C compiler available, you would typically use a compilation command like gcc -o mult.cgi mult.c and then move (mv) mult.cgi to a directory with a name like cgi-bin. Instead of gcc, you might need to use cc. You really need to check local instructions for such issues.

The filename extension `.cgi` has no fixed meaning in general. However, there can be server-dependent (and operating system dependent) rules for naming executable files. Typical extensions for executables are `.cgi` and `.exe`.

## The Hello world test
As usual when starting work with some new programming technology, you should probably first make a trivial program work. This avoids fighting with many potential problems at a time and concentrating first on the issues specific to the environment, here CGI.

You could use the following program that just prints Hello world but preceded by HTTP headers as required by the CGI interface. Here the header specifies that the data is plain ASCII text.
```C
#include <stdio.h>
int main(void) {
  printf("Content-Type: text/plain;charset=us-ascii\n\n");
  printf("Hello world\n\n");
  return 0;
}
```

After compiling, loading, and uploading, you should be able to test the script simply by entering the URL in the browser’s address bar. You could also make it the destination of a normal link in an HTML document.

### Sample program: view data stored on a file
Finally, we can write a simple program for viewing the data; it only needs to copy the content of a given text file onto standard output:

```C
#include <stdio.h>
#include <stdlib.h>
#define DATAFILE "../data/data.txt"

int main(void)
{
	FILE *f = fopen(DATAFILE,"r");
	int ch;
	if(f == NULL) {
		printf("%s%c%c\n",
		"Content-Type:text/html;charset=iso-8859-1",13,10);
		printf("<TITLE>Failure</TITLE>\n");
		printf("<P><EM>Unable to open data file, sorry!</EM>");
	} else {
		printf("%s%c%c\n",
		"Content-Type:text/plain;charset=iso-8859-1",13,10);
		while((ch=getc(f)) != EOF)
			putchar(ch);
		fclose(f);
	}
	return 0;
}
```

Notice that this program prints (when successful) the data as plain text, preceded by a header that says this, i.e. has `text/plain` instead of `text/html`.

A form that invokes that program can be very simple, since no input data is needed:

```html
<form action="http://www.example/cgi-bin/viewdata.cgi">
<div><input type="submit" value="View"></div>
</form>
```

### Form for checking submitted data
The content of the text file to which the submissions are stored will be displayed as plain text.

Even though the output is declared to be plain text, Internet Explorer may interpret it partly as containing HTML markup. Thus, if someone enters data that contains such markup, strange things would happen. The `viewdata.c` program takes this into account by writing the `NULL` character (`'\0'`) after each occurrence of the greater-than character `lt`;, so that it will not be taken (even by IE) as starting a tag.

## How to process a simple form
> For forms that use `METHOD="GET"` (as our simple example above uses, since this is the default), CGI specifications say that the data is passed to the script or program in an environment variable called `QUERY_STRING`.

It depends on the scripting or programming language used how a program can access the value of an environment variable. In the C language, you would use the library function `getenv` (defined in the standard library `stdlib`) to access the value as a string. You might then use various techniques to pick up data from the string, convert parts of it to numeric values, etc.

The _output_ from the script or program to “primary output stream” (such as stdin in the C language) is handled in a special way. Effectively, it is directed so that it gets sent back to the browser. Thus, by writing a C program that it writes an HTML document onto its standard output, you will make that document appear on user’s screen as a response to the form submission.

In this case, the source program in C is the following:

```C
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
	char *data;
	long m,n;
	printf("%s%c%c\n",
	"Content-Type:text/html;charset=iso-8859-1",13,10);
	printf("<TITLE>Multiplication results</TITLE>\n");
	printf("<H3>Multiplication results</H3>\n");
	data = getenv("QUERY_STRING");
	if(data == NULL)
		printf("<P>Error! Error in passing data from form to script.");
	else if(sscanf(data,"m=%ld&n=%ld",&m,&n)!=2)
		printf("<P>Error! Invalid data. Data must be numeric.");
	else
		printf("<P>The product of %ld and %ld is %ld.",m,n,m*n);
	return 0;
}
```

> As a disciplined programmer, you have probably noticed that the program makes no check against integer overflow, so it will return bogus results for very large operands. In real life, such checks would be needed, but such considerations would take us too far from our topic.

**Note**: The first `printf` function call prints out data that will be sent by the server as an HTTP header. This is required for several reasons, including the fact that a CGI script can send any data (such as an image or a plain text file) to the browser, not just HTML documents. For HTML documents, you can just use the `printf` function call above as such; however, if your character encoding is different from ISO 8859-1 (ISO Latin 1), which is the most common on the Web, you need to replace iso-8859-1 by the registered name of the encoding (“charset”) you use.

Having compiled this program and saved the executable program under the name `mult.cgi` in your directory for CGI scripts at the server, say `www.example`. This implies that any form (on any web page) with `action="http://www.example.fi/cgi-bin/mult.cgi"` will, when submitted, be processed by that program.

> Consequently, anyone could write a form of his own with the same ACTION attribute and pass whatever data he likes to my program. Therefore, the program needs to be able to handle any data. Generally, you need to check the data before starting to process it.

## Using METHOD="POST"

### The idea of METHOD="POST"

Let us consider next a different processing for form data. Assume that we wish to write a form that takes a line of text as input so that the form data is sent to a CGI script that appends the data to a text file on the server. (That text file could be readable by the author of the form and the script only, or it could be made readable to the world through another script.)

It might seem that the problem is similar to the example considered above; one would just need a different form and a different script (program). In fact, there is a difference. The example above can be regarded as a “pure query” that does not change the “state of the world.” In particular, it is “idempotent,” i.e. the same form data could be submitted as many times as you like without causing any problems (except minor waste of resources). How­ever, our current task needs to cause such changes—a change in the content of a file that is intended to be more or less permanent. Therefore, one should use `METHOD="POST"`. This is explained in more detail in the document Methods `GET` and `POST` in HTML forms - what’s the difference? Here we will take it for granted that METHOD="POST" needs to be used and we will consider the technical implications.

For forms that use `METHOD="POST"`, CGI specifications say that the data is passed to the script or program in the standard input stream (`stdin`), and the length (in bytes, i.e. characters) of the data is passed in an environment variable called `CONTENT_LENGTH`.

### Reading input

Reading from standard input sounds probably simpler than reading from an environment variable, but there are complications. The server is not required to pass the data so that when the CGI script tries to read more data than there is, it would get an end of file indi­ca­tion! That is, if you read e.g. using the getchar function in a C program, it is undefined what happens after reading all the data characters; it is not guaranteed that the function will return `EOF`.

> When reading the input, the program must not try to read more than `CONTENT_LENGTH` characters.

### Sample program: accept and append data

A relatively simple C program for accepting input via CGI and `METHOD="POST"` is the following:

```C
#include <stdio.h>
#include <stdlib.h>
#define MAXLEN 80
#define EXTRA 5
/* 4 for field name "data", 1 for "=" */
#define MAXINPUT MAXLEN+EXTRA+2
/* 1 for added line break, 1 for trailing NUL */
#define DATAFILE "../data/data.txt"

void unencode(char *src, char *last, char *dest)
{
 for(; src != last; src++, dest++)
   if(*src == '+')
     *dest = ' ';
   else if(*src == '%') {
     int code;
     if(sscanf(src+1, "%2x", &code) != 1) code = '?';
     *dest = code;
     src +=2; }     
   else
     *dest = *src;
 *dest = '\n';
 *++dest = '\0';
}

int main(void)
{
	char *lenstr;
	char input[MAXINPUT], data[MAXINPUT];
	long len;
	printf("%s%c%c\n",
	"Content-Type:text/html;charset=iso-8859-1",13,10);
	printf("<TITLE>Response</TITLE>\n");
	lenstr = getenv("CONTENT_LENGTH");
	if(lenstr == NULL || sscanf(lenstr,"%ld",&len)!=1 || len > MAXLEN)
		printf("<P>Error in invocation - wrong FORM probably.");
	else {
		FILE *f;
		fgets(input, len+1, stdin);
		unencode(input+EXTRA, input+len, data);
		f = fopen(DATAFILE, "a");
	}
	if(f == NULL)
		printf("<P>Sorry, cannot store your data.");
	else
		fputs(data, f);
	fclose(f);
	printf("<P>Thank you! Your contribution has been stored.");
	return 0;
}
```

Essentially, the program retrieves the information about the number of characters in the input from value of the `CONTENT_LENGTH` environment variable. Then it unencodes (decodes) the data, since the data arrives in the specifically encoded format that was already men­tioned. The program has been written for a form where the text input field has the name data (actually, just the length of the name matters here). For example, if the user types
`Hello there!`
then the data will be passed to the program encoded as
`data=Hello+there%21`
(with space encoded as + and exclamation mark encoded as `%21`). The unencode routine in the program converts this back to the original format. After that, the data is appended to a file (with a fixed file name), as well as echoed back to the user.

Having compiled the program I have saved it as `collect.cgi` into the directory for CGI scripts. Now a form like the following can be used for data submissions:

```html
<FORM ACTION="http://www.example/cgi-bin/collect.cgi"
 METHOD="POST">
<DIV>Your input (80 chars max.):<BR>
<INPUT NAME="data" SIZE="60" MAXLENGTH="80"><BR>
<INPUT TYPE="SUBMIT" VALUE="Send"></DIV>
</FORM>
```
