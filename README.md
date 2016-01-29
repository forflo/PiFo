# DESCRIPTION

This is a fork of the pidgin latex plugin. It aims to
refactor and extend it. By now, it is basically a complete
rewrite of the original sources.
The pidgin latex plugin for Pidgin allows you to 
display various latex markup.

You can
* highlight sourcecode
* display formulas
* generate graphs from dot files

# REQUIREMENTS
	- The pidgin header files (needed to compile the plugin)
	- A LaTex-Distribution
	- dvipng [3]
    - latex
    - graphviz

# USAGE
The plugin reads every message you send to your conversation
partner. You can markup some of your text via the following
construct:

    $command{your formula, dot code or sourcecode}

## Graphivz dot code
Display of graphviz dot code

    He look at following graph: $graph{your dot code here}

## Latex formulas
Use the following snippet:

    Hey, i found out that I can create the set of natural
    numbers by inductively describing. I Use the Axiom
    $formula{\frac{}{n}} and the rule $formula{\frac{n}{n+1}}

## Sourcecode hightlighting
Use the following snippet:

    Hey, this is not an iteration in C
    $c{
        int main(void){ return 0; }
    }

# IMPORTANT
This plugin is not compatible with gaim-encryption plugin. 
If you have this plugin, please be sure not having the 2 
working at the same time. (for any other imcompability, 
please contact qjuh@users.sourceforge.net )

# BUILDING and INSTALLATION
To build and install :
	You can compile the plugin using
	$ make
	and install it with
	$ make install
	This will install it in ~/.purple/plugins so that only the user who install it can use it.

To install it for everybody on your computer,
	$ make
	$ su
	# make install PREFIX="/path/to/pidgin" (this command as root user)
	generally /path/to/pidgin is /usr or /usr/local. If you don't know the path then you can find out using
	$ whereis pidgin
	and look for the part before "/lib/pidgin".


# NOTE
To everyone updating from earlier versions than 1.3: you don't need ImageMagick anymore for this plugin to work.
Instead this plugin uses dvipng [3], so please install it, if you don't already have it.

# WEB

* http://www.pidgin.im/
* http://www.latex-project.org/
* http://savannah.nongnu.org/projects/dvipng/

# DEVELOPERS
Current: Benjamin Moll <qjuh@users.sourceforge.net> (contact me for all infomations or suggestions)
Former: Nicolas Schoonbroodt <nicolas@ffsa.be>
Thanks for help or patch these other developer:
	Eric Betts <bettse@onid.orst.edu>
	GRIm@ <thegrima@altern.org>
	Gof <ogoffart@kde. org>
	Nicolai Stange <nic-stange@t-online.de>
	Martin Keßler <martin@moegger.de>
