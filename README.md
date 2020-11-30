# pngcrop
returns an WxH+X+Y geometry suitable for passing to convert -crop for cropping pages from e-rara pdfs

example process from start to end to crop and trim a 6"x9" pdf suitable for delivery to printing press:
```
mkdir 1 2 3
gs -q -sDEVICE=pngalpha -r300 -o 1/%03d.png inputpdf1.pdf
gs -q -sDEVICE=pngalpha -r300 -o 2/%03d.png inputpdf2.pdf
gs -q -sDEVICE=pngalpha -r300 -o 3/%03d.png inputpdf3.pdf

mkdir cropped
let a=1; for i in 1 2 3; do for f in $i/*.png; do convert $f -crop `./pc $f` -page 1800x2700 -resize 1800x2700\! cropped/`printf "%03d.pdf" $a`; let a=$a+1; done; done

gs -dNOPAUSE -dQUIET -dBATCH -sDEVICE=pdfwrite -dPDFSETTINGS=/printer -dCompatibilityLevel=1.4 -r300 -sOutputFile=output.pdf cropped/*.pdf

pdfjam --papersize '{6in,9in}' --twoside --offset '0.2in 0in' --scale 0.93 output.pdf -o fit.pdf
```
