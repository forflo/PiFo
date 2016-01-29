#ifndef GENERATOR
#define GENERATOR

static gboolean setup_files(GString **tex, 
        GString **dvi, GString **png,
        GString **aux, GString **log);

static gboolean generate_latex_listing(GString *listing, 
        GString *language, GString **filename);

static GString *dispatch_command(GString *command, GString *snippet);
static GString *fgcolor_as_string();
static GString *bgcolor_as_string();

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

#define LATEX_LST_CONFIG(NUMBERS,TABSIZE,FRAME) \
    "\\lstset{numbers=" #NUMBERS ",numberstyle=\\small{"\
    "\\ttfamily{}},stepnumber=1,numbersep=4pt}" \
    "\\lstset{tabsize=" #TABSIZE "}"\
    "\\lstset{breaklines=true,breakatwhitespace=true}"\
    "\\lstset{frame=" #FRAME "}" 

#define LATEX_LST_TEMPLATE(CONFIG,LANGUAGE,FCOLOR,BCOLOR,LISTING) \
    "\\documentclass[12pt]{article}\\usepackage{color}"  \
    "\\usepackage[dvips]{graphicx}\\usepackage{amsmath}" \
    "\\usepackage{amssymb}\\usepackage[utf8]{inputenc}"  \
    "\\usepackage{listings}\\pagestyle{empty}" \
    "\\definecolor{fgcolor}{RGB}" "{" #FCOLOR "}" \
    "\\definecolor{bgcolor}{RGB}" "{" #BCOLOR "}" \
        #CONFIG \
    "\\lstset{language=" #LANGUAGE "}" \
    "\\begin{document}\\pagecolor{bgcolor}\\color{fgcolor}" \
    "\\begin{lstlisting}\n"\
        #LISTING "\n" \
    "\\end{lstlisting}" \
    "\\end{document}"


#endif
