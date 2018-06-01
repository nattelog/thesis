TEXMAINFILE = presentation.tex
PDFNAME = $(shell basename -s.tex $(TEXMAINFILE))
MKLATEX = latexmk
MKLATEXOPTS = -pdf -pdflatex="xelatex -interaction=nonstopmode" -use-make

all: $(PDFNAME).pdf notes

$(PDFNAME).pdf: $(TEXMAINFILE) *.tex
	$(MKLATEX) $(MKLATEXOPTS) $<

notes:
	$(MKLATEX) $(MKLATEXOPTS) notes.tex

watch:
	while true; do make --silent; sleep 2; done

clean:
	$(MKLATEX) -CA
	rm -f $(PDFNAME).pdf *.{aux,bbl,bcf,blg,cb,fdb_latexmk,fls,lof,log,lot,out,run.xml,rel,synctex.gz,toc}
