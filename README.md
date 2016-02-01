# Description

This is a fork of the pidgin latex plugin. It aims to
refactor and extend it. By now, it is basically a complete
rewrite of the original sources.
The pidgin latex plugin for Pidgin allows you to 
display various latex markup.

You can
* highlight your sourcecode,
* display formulas or
* generate graphs from dot files

# Demo

Let's jump right in!
![Demo](screencast/screencast.webm)

# Requirements
	- The pidgin header files (needed to compile the plugin)
	- A LaTex-Distribution
	- dvipng
    - latex
    - graphviz

# Usage
You can markup some of your text via the following
construct (embedded into your normal conversation)

    \command{your formula, dot code or whatever}

If a command is detected, it'll be dispatched to one of the rendering
backends and the result is displayed nicely enclosed by your 
surrounding text.

Here is a list of markup commands you can currently use.

## Graphivz dot
Display of graphviz dot code

    He look at following graph: \graph{your dot code here}

## Latex maths
Use the following snippet:

    Hey, i found out that I can create the set of natural
    numbers by describing them inductively. I use the Axiom
    \formula{\frac{}{n}} and the rule \formula{\frac{n}{n+1}}

## Sourcecode hightlighting
Use the following snippet for C programs

    Not an iteration in C: \c{
        int main(void){ 
            return 0; 
        }
    }

or use this one for highlighting haskell sources

    I hate programming. Look: \haskell{
        shaves :: Integer -> Integer -> Bool
        1 `shaves` 1 = True
        2 `shaves` 2 = False
        0 `shaves` x = not (x `shaves` x)
        _ `shaves` _ = False
    }

# BUILDING and INSTALLATION
To build and install :

You can compile the plugin using

	$ make

and install it with

	$ make install

This will install it in ~/.purple/plugins so 
that only the user who install it can use it.

To install it for everybody on your computer,

	$ make
	$ su
	# make install PREFIX="/path/to/pidgin" (this command as root user)

generally /path/to/pidgin is /usr or /usr/local. If you don't know the path then you can find out using

	$ whereis pidgin

and look for the part before "/lib/pidgin".

