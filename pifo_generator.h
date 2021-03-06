#ifndef GENERATOR
#define GENERATOR

#include "pifo.h"

struct mapping {
    const char *command;
    gboolean (*handler)(const GString *string,
            const GString *command,
            void **returnval);
};

gboolean setup_files(GString **tex,
       GString **dvi, GString **png,
       GString **aux, GString **log);

gboolean generate_svg_png(const GString *svg_code,
        const GString *command, GString **filename_png);

gboolean generate_latex_listing(const GString *listing,
        const GString *language, GString **filename);

gboolean generate_graphviz_png(const GString *dotcode,
        const GString *command,
        GString **filename);

gboolean generate_latex_formula(const GString *formula,
        const GString *command,
        GString **filename_png);

gboolean render_latex(const GString *pngfilepath,
        const GString *texfilepath, const GString *dvifilepath);

gboolean generate_markdown(const GString *markdown_text,
                           const GString *command,
                           GString **filename_png);

gboolean render_markdown (const GString *markdownfilepath,
                          const GString *texfilepath);

gboolean render_latex_pdf_to_png(const GString *pngfilepath, 
        const GString *texfilepath, const GString *epsfilepath,
        const GString *pdffilepath);

gboolean generate_tikz_png(const GString *tikz_code,
        const GString *command,
        GString **filename_png);

gboolean chtempdir(const GString *path);
GString *dispatch_command(const GString *command, const GString *snippet);
GString *fgcolor_as_string(void);
GString *bgcolor_as_string(void);

#define LATEX_MATH_TEMPLATE \
    "\\documentclass[12pt]{article}\\usepackage{color}"  \
    "\\usepackage[dvips]{graphicx}\\usepackage{amsmath}" \
    "\\usepackage{amssymb}\\usepackage[utf8]{inputenc}"  \
    "\\pagestyle{empty}" \
    "\\definecolor{fgcolor}{RGB}" "{%s}" \
    "\\definecolor{bgcolor}{RGB}" "{%s}" \
    "\\begin{document}\\pagecolor{bgcolor}\\color{fgcolor}" \
    "\\begin{gather*}" \
        "%s" \
    "\\end{gather*}" \
    "\\end{document}"

#define LATEX_LST_TEMPLATE \
    "\\documentclass[12pt]{article}" \
    "\\usepackage{color}" \
    "\\usepackage{listings}" \
    "\\definecolor{fgcolor}{RGB}{%s} " \
    "\\definecolor{bgcolor}{RGB}{%s} " \
    "\\lstset{numbers=%s,numberstyle=\\small{"\
    "\\ttfamily{}},stepnumber=1,numbersep=4pt}" \
    "\\lstset{tabsize=%s}"\
    "\\lstset{breaklines=false,breakatwhitespace=false}"\
    "\\lstset{frame=%s}" \
    "\\lstset{language=%s}" \
    "\\definecolor{comment}{RGB}{102,0,102}" \
    "\\definecolor{keyword}{RGB}{0,100,100}" \
    "\\definecolor{identifier}{RGB}{0,11,0}" \
    "\\definecolor{string}{RGB}{0,155,0}" \
    "\\lstset{showspaces=false,showstringspaces=false}" \
    "\\lstset{basicstyle=\\ttfamily{}," \
    "    identifierstyle=\\color{identifier}," \
    "    keywordstyle=\\color{keyword}," \
    "    commentstyle=\\color{comment}," \
    "    stringstyle=\\itshape\\color{string}}" \
    "\\begin{document}" \
    "\\pagenumbering{gobble}" \
    "\\pagecolor{bgcolor}\\color{fgcolor} " \
    "\\begin{lstlisting}\n" \
        "%s" \
    "\n\\end{lstlisting}" \
    "\\end{document}"

#define LATEX_TIKZ_TEMPLATE \
    "\\documentclass{article}" \
    "\\usepackage{color}" \
    "\\usepackage{tikz}" \
    "\\usetikzlibrary{babel,scopes,intersections,calc,bending,positioning,quotes,graphs,fadings,decorations,angles,automata,babel,backgrounds,calc,calendar,chains,circuits,er,external,external,external,fadings,fadings,external,fadings,fit,fixedpointarithmetic,fpu,lindenmayersystems,math,matrix,mindmap,folding,patterns,petri,plothandlers,plotmarks,profiler,shadings,shadows,spy,topaths,through,trees,turtle,datavisualization,intersections,curvilinear}" \
    "\\begin{document}" \
    "\\pagenumbering{gobble}" \
    "\\begin{tikzpicture}" \
    "%s" \
    "\\end{tikzpicture}" \
    "\\end{document}"

#endif
