default: compile clean

OUTPUTNAME=Computer_Graphics_Project_-_Report
MAINFILE=main.tex

# We need to compile the document twice for biber to work, since it relies on the generated output
compile:
	cat $(MAINFILE) | pdflatex -jobname=$(OUTPUTNAME)
	biber $(OUTPUTNAME)
	cat $(MAINFILE) | pdflatex -jobname=$(OUTPUTNAME)

# Remove all auto generated files
clean:
	rm *.aux *.bbl *.bcf *.blg *.log *.run.xml
