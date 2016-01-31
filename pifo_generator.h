#ifndef GENERATOR
#define GENERATOR

#include "pifo.h"

gboolean setup_files(GString **tex, 
       GString **dvi, GString **png,
       GString **aux, GString **log);

gboolean generate_latex_listing(const GString *listing, 
       const GString *language, GString **filename);

gboolean generate_graphviz_png(const GString *dotcode,
       GString **filename);

gboolean generate_latex_formula(const GString *formula, 
        GString **filename_png);

gboolean exec_latex(const GString *pngfilepath, 
        const GString *texfilepath, const GString *dvifilepath);

gboolean chtempdir(const GString *path);
GString *dispatch_command(const GString *command, const GString *snippet);
GString *fgcolor_as_string(void);
GString *bgcolor_as_string(void);

#define LATEX_MATH_TEMPLATE(FORMULA,FCOLOR,BCOLOR) \
    "\\documentclass[12pt]{article}\\usepackage{color}"  \
    "\\usepackage[dvips]{graphicx}\\usepackage{amsmath}" \
    "\\usepackage{amssymb}\\usepackage[utf8]{inputenc}"  \
    "\\pagestyle{empty}" \
    "\\definecolor{fgcolor}{RGB}" "{" #FCOLOR "}" \
    "\\definecolor{bgcolor}{RGB}" "{" #BCOLOR "}" \
    "\\begin{document}\\pagecolor{bgcolor}\\color{fgcolor}" \
    "\\begin{gather*}" \
        #FORMULA \
    "\\end{gather*}" \
    "\\end{document}"

//7parameters
#define LATEX_LST_TEMPLATE(NUMBERS,TABSIZE,FRAME,LANGUAGE,FCOLOR,BCOLOR,LISTING) \
    "\\documentclass[12pt]{article}" \
    "\\usepackage{color}" \
    "\\usepackage{listings}" \
    "\\definecolor{fgcolor}{RGB}{" #FCOLOR "} " \
    "\\definecolor{bgcolor}{RGB}{" #BCOLOR "} " \
    "\\lstset{numbers=" #NUMBERS ",numberstyle=\\small{"\
    "\\ttfamily{}},stepnumber=1,numbersep=4pt}" \
    "\\lstset{tabsize=" #TABSIZE "}"\
    "\\lstset{breaklines=true,breakatwhitespace=true}"\
    "\\lstset{frame=" #FRAME "}" \
    "\\lstset{language=" #LANGUAGE "}" \
    "\\definecolor{comment}{RGB}{102,0,102}" \
    "\\definecolor{identifier}{RGB}{0,100,100}" \
    "\\definecolor{keyword}{RGB}{0,11,0}" \
    "\\definecolor{string}{RGB}{0,155,0}" \
    "\\lstset{showspaces=false,showstringspaces=false}" \
    "\\lstset{basicstyle=\\ttfamily{}," \
    "    identifierstyle=\\color{identifier}," \
    "    keywordstyle=\\color{keyword}," \
    "    commentstyle=\\color{comment}," \
    "    stringstyle=\\itshape\\color{string}}" \
    "\\begin{document}" \
    "\\pagecolor{bgcolor}\\color{fgcolor} " \
    "\\begin{lstlisting\n}" \
        #LISTING \
    "\n\\end{lstlisting}" \
    "\\end{document}"


#endif
